#include <atomic>
#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "schedular.h"

// 协程库
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};
    static thread_local Fiber *t_currfiber = nullptr;          // 线程局部 线程的当前协程
    static thread_local Fiber::FiberPtr t_mainfiber = nullptr; // 线程局部 线程的主协程

    // 配置(若没有指定栈大小，使用此大小)
    static blue::ConfigVar<uint32_t>::ConfigVarPtr g_stack_size_ptr =
        blue::Config::Lookup<uint32_t>("fiber.stack_size",
                                       1024 * 1024,
                                       "fiber stack size");

    // 内存分配器
    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }
        static void Delloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    using StackAllocatorType = MallocStackAllocator;

    uint64_t Fiber::GetFiberID()
    {
        if (t_currfiber)
        {
            return t_currfiber->getId();
        }
        return 0;
    }

    void Fiber::SetThis(blue::Fiber *f)
    {
        t_currfiber = f;
        return;
    }

    std::shared_ptr<Fiber> Fiber::CreateMainFiber()
    {
        return std::shared_ptr<Fiber>(new Fiber(true, Fiber::Status::EXEC));
    }

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

    void Fiber::YieldToReady()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr->m_status == Status::EXEC);
        curr->m_status.store(Status::READY, std::memory_order_release);
        curr->swapOut();
    }

    void Fiber::YieldToHold()
    {
        Fiber::FiberPtr curr = GetThis();
        BLUE_ASSERT(curr->m_status == Status::EXEC);
        curr->m_status.store(Status::HOLD, std::memory_order_release);
        curr->swapOut();
    }

    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

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
            curr->m_status.store(Status::TERM, std::memory_order_release);
        }
        catch (const std::exception &e)
        {
            curr->m_status.store(Status::EXCEPT, std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber Excetion " << e.what();
        }
        catch (...)
        {
            curr->m_status.store(Status::EXCEPT, std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber exception ";
        }
        auto self = curr.get();
        curr.reset();
        self->back(); // self还被其他shared_ptr管控,这里不需要手动释放
        BLUE_LOG_INFO(g_logger) << "never come back : MainCallFunc swapOut";
    }

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
            curr->m_status.store(Status::TERM, std::memory_order_release);
        }
        catch (const std::exception &e)
        {
            curr->m_status.store(Status::EXCEPT, std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber Excetion " << e.what();
        }
        catch (...)
        {
            curr->m_status.store(Status::EXCEPT, std::memory_order_release);
            BLUE_LOG_ERROR(g_logger) << "Fiber exception ";
        }
        auto self = curr.get();
        curr.reset();
        self->swapOut(); // self还被其他shared_ptr管控,这里不需要手动释放
        BLUE_LOG_INFO(g_logger) << "never come back : MainFunc swapOut";
    }

    Fiber::Fiber(bool Create, Status init)
    {
        SetThis(this);
        m_status.store(init, std::memory_order_release);
        m_ctx = std::make_shared<Context>();
        ++s_fiber_count;
    }

    Fiber::Fiber(std::function<void()> cb, bool use_caller, size_t stacksize) : m_id(++s_fiber_id),
                                                                                m_cb(cb)
    {
        m_status.store(Status::INIT, std::memory_order_release);
        ++s_fiber_count;
        // 非零设置stacksize,为零设置配置中的size
        m_stacksize = stacksize ? stacksize : g_stack_size_ptr->getValue();

        m_stack = StackAllocatorType::Alloc(m_stacksize);
        if (use_caller)
        {
            m_ctx = std::make_shared<Context>(Fiber::MainCallFunc, m_stack, m_stacksize);
        }
        else
        {
            m_ctx = std::make_shared<Context>(Fiber::MainFunc, m_stack, m_stacksize);
        }
    }

    Fiber::~Fiber()
    {
        --s_fiber_count;

        uint64_t this_id = m_id;
        uint64_t curr_id = 0;

        if (m_stack)
        {
            auto status = m_status.load(std::memory_order_acquire);
            ;
            BLUE_ASSERT(status == Status::TERM ||
                        status == Status::EXCEPT ||
                        status == Status::INIT);
            StackAllocatorType::Delloc(m_stack, m_stacksize);
        }
        else
        {
            // 主协程
            BLUE_ASSERT(!m_cb);
            BLUE_ASSERT(m_status.load(std::memory_order_acquire) == Status::EXEC);

            if (t_currfiber == this)
            {
                SetThis(nullptr);
            }
        }
        if (t_currfiber)
        {
            curr_id = t_currfiber->getId();
        }
    }

    void Fiber::reset(std::function<void()> cb)
    {
        BLUE_ASSERT(m_stack);
        auto status = m_status.load(std::memory_order_acquire);
        BLUE_ASSERT(status == Status::INIT ||
                    status == Status::EXCEPT ||
                    status == Status::TERM);
        m_status.store(Status::INIT, std::memory_order_release);
        m_cb = cb;
        m_ctx = std::make_shared<Context>(Fiber::MainFunc, m_stack, m_stacksize);
    }

    void Fiber::call()
    {
        BLUE_ASSERT(m_status.load(std::memory_order_acquire) != Status::EXEC);
        BLUE_ASSERT2(t_mainfiber, "main fiber not exit");
        BLUE_ASSERT2(this, "this not exit");
        SetThis(this);
        m_status.store(Status::EXEC, std::memory_order_release);
        Context::Swap(*t_mainfiber->m_ctx, *m_ctx);
    }

    void Fiber::back()
    {
        BLUE_ASSERT2(t_mainfiber, "main fiber not exit");
        SetThis(t_mainfiber.get());
        if (m_status.load(std::memory_order_acquire) != Status::TERM)
        {
            // 如果已经是 READY，不要改成 HOLD！
            auto status = m_status.load(std::memory_order_acquire);
            if (status != Status::READY)
            {
                m_status.store(Status::HOLD, std::memory_order_release);
            }
        }
        Context::Swap(*m_ctx, *t_mainfiber->m_ctx);
    }

    void Fiber::swapIn()
    {
        // 保留没有调度器用法
        if (blue::Schedular::GetThis() == nullptr && blue::Schedular::GetMainFiber() == nullptr)
        {
            call();
        }
        else
        {
            BLUE_ASSERT(m_status.load(std::memory_order_acquire) != Status::EXEC);
            BLUE_ASSERT2(blue::Schedular::GetMainFiber(), "schedular main fiber not exit");
            BLUE_ASSERT2(this, "this not exit");
            SetThis(this);
            m_status.store(Status::EXEC, std::memory_order_release);
            Context::Swap(*Schedular::GetMainFiber()->m_ctx, *m_ctx);
        }
    }

    void Fiber::swapOut()
    {
        // 保留没有调度器用法
        if (blue::Schedular::GetThis() == nullptr && blue::Schedular::GetMainFiber() == nullptr)
        {
            back();
        }
        else
        {
            BLUE_ASSERT2(blue::Schedular::GetMainFiber(), "GetMainFiber() not exit");
            SetThis(blue::Schedular::GetMainFiber());

            if (m_status.load(std::memory_order_acquire) != Status::TERM)
            {
                // 如果已经是 READY，不要改成 HOLD！
                auto status = m_status.load(std::memory_order_acquire);
                if (status != Status::READY)
                {
                    m_status.store(Status::HOLD, std::memory_order_release);
                }
            }
            Context::Swap(*m_ctx, *Schedular::GetMainFiber()->m_ctx);
        }
    }

}
