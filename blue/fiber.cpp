#include <atomic>
#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "schedular.h"
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id {0};
    static std::atomic<uint64_t> s_fiber_count {0};
    static thread_local Fiber* t_currfiber = nullptr;           // 线程局部 线程的当前协程
    static thread_local Fiber::FiberPtr t_mainfiber = nullptr;  // 线程局部 线程的主协程

    // 配置(若没有指定栈大小，使用此大小)
    static blue::ConfigVar<uint32_t>::ConfigVarPtr g_stack_size_ptr = 
    blue::Config::Lookup<uint32_t>("fiber.stack_size",
        1024*1024,
        "fiber stack size");
    
    // 内存分配器
    class MallocStackAllocator
    {
        public:
            static void* Alloc(size_t size)
            {
                return malloc(size);
            }
            static void Delloc(void* vp,size_t size)
            {
                return free(vp);
            }
    };

    using StackAllocatorType = MallocStackAllocator;
    /* static function */

    // 获取协程id
    uint64_t Fiber::GetFiberID()
    {
        if (t_currfiber)
        {
            return t_currfiber->getId();
        }
        return 0;
    }

    // 设置当前执行的协程
    void Fiber::SetThis(blue::Fiber* f)
    {
        t_currfiber = f;
        return;
    }

    // 创建主协程
    std::shared_ptr<Fiber> Fiber::CreateMainFiber()
    {
        return std::shared_ptr<Fiber>(new Fiber(true, Fiber::Status::EXEC));
    }

    // 返回当前执行的协程，若没有协程，设置主协程并返回
    Fiber::FiberPtr Fiber::GetThis()
    {
        if (t_currfiber)
        {
            return t_currfiber->shared_from_this();
        }
        BLUE_ASSERT(t_mainfiber == nullptr);
        // 构造函数设置当前协程(t_fiber)
        Fiber::FiberPtr main_Fiber = CreateMainFiber();
        // main_Fiber设为主协程
        t_mainfiber = main_Fiber;
        return t_currfiber->shared_from_this();
    }

    // 协程切换到后台,并设为Ready状态
    void Fiber::YieldToReady()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr->m_status == Status::EXEC);
        curr->m_status.store(Status::READY,std::memory_order_release);
        curr->swapOut();
    }

    // 协程切换到后台,并设为Hold状态
    void Fiber::YieldToHold()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr->m_status == Status::EXEC);
        curr->m_status.store(Status::HOLD, std::memory_order_release);
        curr->swapOut();
    }

    // 协程总数
    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    // 2026-4-12新增
    // 协程执行调度器主协程协程绑定的协程函数开始函数
    void Fiber::MainCallFunc()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr);
        BLUE_ASSERT(curr != t_mainfiber);
        try
        {
            // 开始执行当前协程上面的任务
            curr->m_cb();
            curr->m_cb = nullptr;
            curr->m_status.store(Status::TERM,std::memory_order_release);
        }
        catch(const std::exception& e)
        {
            curr->m_status.store(Status::EXCEPT,std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber Excetion " << e.what();
        }
        catch(...)
        {
            curr->m_status.store(Status::EXCEPT,std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber exception ";
        }
        auto self = curr.get();
        curr.reset();
        self->back(); // self还被其他shared_ptr管控,这里不需要手动释放
        BLUE_LOG_INFO(g_logger) << "never come back : MainCallFunc swapOut";
    }

    // 协程执行每个协程绑定的协程函数的开始函数
    void Fiber::MainFunc()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr);
        BLUE_ASSERT(curr != t_mainfiber);
        try
        {
            // 开始执行当前协程上面的任务
            curr->m_cb();
            curr->m_cb = nullptr;
            curr->m_status.store(Status::TERM,std::memory_order_release);
        }
        catch(const std::exception& e)
        {
            curr->m_status.store(Status::EXCEPT,std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber Excetion " << e.what();
        }
        catch(...)
        {
            curr->m_status.store(Status::EXCEPT,std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber exception ";
        }
        auto self = curr.get();
        curr.reset();
        self->swapOut(); // self还被其他shared_ptr管控,这里不需要手动释放
        BLUE_LOG_INFO(g_logger) << "never come back : MainFunc swapOut";
    }

    /*-------------------- private begin -------------------*/

    // 线程主协程创建的构造函数(私有)
    Fiber::Fiber(bool Create,Status init)
    {
        SetThis(this);
        m_status.store(init,std::memory_order_release);
        if (getcontext(&m_ctx))
        {
            BLUE_ASSERT2(false,"getcontext error");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = nullptr;
        m_ctx.uc_stack.ss_size = 0;
        ++s_fiber_count;
        BLUE_LOG_INFO(g_logger) << "Fiber::Fiber(主协程构造) curr_id : " 
                                << Fiber::GetFiberID();
    }   

    /* -------------------- private end --------------------*/

    // 主协程去创建新的子协程, cb : 回调函数, stacksize : 协程栈大小
    // use_caller为true表示需要使用使用call,back协程上下文切换函数组
    Fiber::Fiber(std::function<void()> cb,bool use_caller,size_t stacksize):
    m_id(++s_fiber_id),
    m_cb(cb)
    {
        // 关键(否则出现栈溢出,空指针导致swapOut上下文切换出现问题,程序不能正常退出,内存泄漏)
        // 目前来说这句可以省略,现在没有上面的问题了，想来一开始可能还是由于idle_fiber的问题导致
        // 但加上符合逻辑
        // 2026-4-12(修改了swapIn,swapOut的逻辑，不再去使用if,else在swap和call,back之间来回)\
        切换,故而之前所有的协程上下文异常都是这个判断导致的,现在也无需现在也无需写SetThis(this)了)
        // SetThis(this);

        m_status.store(Status::INIT,std::memory_order_release);
        ++s_fiber_count;
        // 非零设置stacksize,为零设置配置中的size
        m_stacksize = stacksize ? stacksize : g_stack_size_ptr->GetValue();

        m_stack = StackAllocatorType::Alloc(m_stacksize);
        if (getcontext(&m_ctx))
        {
            BLUE_ASSERT2(false,"getcontext error");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        if (use_caller)
        {
            // 使用call,back协程上下文切换函数组
            makecontext(&m_ctx,Fiber::MainCallFunc,0);
        }
        else
        {
            // 使用swapIn,swapOut协程上下文切换函数组
            makecontext(&m_ctx,&Fiber::MainFunc,0);
        }
        BLUE_LOG_INFO(g_logger) << "Fiber::Fiber(子协程构造) curr_id : " 
                                << Fiber::GetFiberID();
    }

    // 协程析构函数(分为析构主协程和子协程)
    Fiber::~Fiber()
    {
        --s_fiber_count;

        uint64_t this_id = m_id;
        uint64_t curr_id = 0;
        
        if (m_stack) {
            auto status = m_status.load(std::memory_order_acquire);
            // BLUE_LOG_INFO(g_logger) << "status : " << (int)status;
            BLUE_ASSERT(status == Status::TERM ||
                        status == Status::EXCEPT ||
                        status == Status::INIT);
            StackAllocatorType::Delloc(m_stack, m_stacksize);
        } else {
            // 主协程
            BLUE_ASSERT(!m_cb);
            BLUE_ASSERT(m_status.load(std::memory_order_acquire) == Status::EXEC);
            
            if (t_currfiber == this) {
                SetThis(nullptr);
            }
        }
        if (t_currfiber) {
            curr_id = t_currfiber->getId();
        }
        
        BLUE_LOG_INFO(g_logger) << "Fiber::~Fiber, this_id : " << this_id 
                                << ", curr_id : " << curr_id;
    }

    // 重置子协程的回调函数,并重置状态
    void Fiber::reset(std::function<void()> cb)
    {
        BLUE_ASSERT(m_stack);
        auto status = m_status.load(std::memory_order_acquire);
        BLUE_ASSERT(status == Status::INIT || 
                    status == Status::EXCEPT || 
                    status == Status::TERM);
        
        // 关键(否则出现栈溢出,空指针导致swapOut上下文切换出现问题,程序不能正常退出,内存泄漏)
        // 目前来说这句可以省略,现在没有上面的问题了，想来一开始可能还是由于idle_fiber的问题导致
        // 但加上符合逻辑
        // 2026-4-12(修改了swapIn,swapOut的逻辑，不再去使用if,else在swap和call,back之间来回)\
        切换,故而之前所有的协程上下文异常都是这个判断导致的,现在也无需写SetThis(this)了)
        // SetThis(this);

        m_status.store(Status::INIT,std::memory_order_release);
        m_cb = cb;
        if (getcontext(&m_ctx))
        {
            BLUE_ASSERT2(false,"getcontext error");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx,&Fiber::MainFunc,0);

    }

    // 2026-4-12修改
    // call(线程主协程跟其他协程切换),切换为当前协程执行
    void Fiber::call()
    {
        BLUE_ASSERT(m_status.load(std::memory_order_acquire) != Status::EXEC);
        BLUE_ASSERT2(t_mainfiber,"main fiber not exit");
        BLUE_ASSERT2(this,"this not exit");
        SetThis(this);
        m_status.store(Status::EXEC,std::memory_order_release);
        if (swapcontext(&t_mainfiber->m_ctx,&m_ctx))
        {
            BLUE_ASSERT2(false,"swapcontext error");
        }
    }

    // back(线程主协程跟其他协程切换),当前协程切换到后台Hold
    void Fiber::back()
    {
        BLUE_ASSERT2(t_mainfiber,"main fiber not exit");
        SetThis(t_mainfiber.get());
        if (m_status.load(std::memory_order_acquire) != Status::TERM)
        {
            // 如果已经是 READY，不要改成 HOLD！
            auto status = m_status.load(std::memory_order_acquire);
            if (status != Status::READY) {
                m_status.store(Status::HOLD, std::memory_order_release);
            }
        }
        if (swapcontext(&m_ctx,&t_mainfiber->m_ctx))
        {
            BLUE_ASSERT2(false,"swapcontext error");
        }
    }

    // 协程调度器之间的协程互相切换
    void Fiber::swapIn()
    {
        // 保留没有调度器用法
        if (blue::Schedular::GetThis() == nullptr && blue::Schedular::GetMainFiber() == nullptr)
        {
            call();
        }
        else{
            BLUE_ASSERT(m_status.load(std::memory_order_acquire) != Status::EXEC);
            BLUE_ASSERT2(blue::Schedular::GetMainFiber(),"schedular main fiber not exit");
            BLUE_ASSERT2(this,"this not exit");
            SetThis(this);
            m_status.store(Status::EXEC,std::memory_order_release);
            if (swapcontext(&blue::Schedular::GetMainFiber()->m_ctx,&m_ctx))
            {
                BLUE_ASSERT2(false,"swapcontext error");
            }
        }
    }

    // 协程调度器之间的协程互相切换     
    void Fiber::swapOut()
    {
        // 保留没有调度器用法
        if (blue::Schedular::GetThis() == nullptr && blue::Schedular::GetMainFiber() == nullptr)
        {
            back();
        }
        else
        {
            BLUE_ASSERT2(blue::Schedular::GetMainFiber(),"GetMainFiber() not exit");
            SetThis(blue::Schedular::GetMainFiber());

            if (m_status.load(std::memory_order_acquire) != Status::TERM)
            {
                // 如果已经是 READY，不要改成 HOLD！
                auto status = m_status.load(std::memory_order_acquire);
                if (status != Status::READY) {
                    m_status.store(Status::HOLD, std::memory_order_release);
                }
            }
            if (swapcontext(&m_ctx,&blue::Schedular::GetMainFiber()->m_ctx))
            {
                BLUE_ASSERT2(false,"swapcontext error");
            }
        }
    }
    
}

