#include "schedular.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
    static thread_local Schedular *t_schedularptr = nullptr; // 线程局部 调度器指针
    static thread_local blue::Fiber *t_mainfiber = nullptr;  // 线程局部 调度器主协程

    Schedular::Schedular(size_t threads,
                         bool use_caller,
                         const std::string &name)
        : m_name(name)
    {
        BLUE_ASSERT(threads > 0);

        // true : 表示所有线程(threads)都工作
        if (use_caller)
        {
            // 如果线程没有协程,那么会创建一个主协程,确保当前线程有协程环境
            blue::Fiber::GetThis();

            --threads; // 减去当前线程

            // 当前线程不能有协程调度器
            BLUE_ASSERT(Schedular::GetThis() == nullptr);
            // 设置当前线程调度器指针
            t_schedularptr = this;
            // 设置调度器的主协程(线程主协程的子协程)
            m_mainfiber.reset(new blue::Fiber(std::bind(&Schedular::run, this), use_caller));
            // 设置线程名称
            blue::Mthread::SetThreadName(m_name);
            // 设置调度器的静态主协程
            t_mainfiber = m_mainfiber.get();
            // 设置调度器主线程id
            m_mainThreadId = blue::GetThreadId();
            // 把线程id push到 vector
            m_threadIds.push_back(m_mainThreadId);
        }
        else
        {
            m_mainThreadId = -1;
        }
        m_threadCounts = threads;
    }

    Schedular::~Schedular()
    {
        BLUE_ASSERT(m_stopping.load(std::memory_order_acquire));
        if (GetThis() == this)
        {
            t_schedularptr = nullptr;
        }
    }

    /*-------------------- static begin ----------------------*/
    Schedular *Schedular::GetThis()
    {
        return t_schedularptr;
    }

    blue::Fiber *Schedular::GetMainFiber()
    {
        return t_mainfiber;
    }
    /*--------------------- static end ------------------------*/

    void Schedular::start()
    {
        // 已经启动了(原子性)
        if (!m_stopping.load(std::memory_order_acquire))
        {
            return;
        }
        MmutexType::lockSco lock(m_mutex);
        if (!m_stopping.load(std::memory_order_relaxed))
        {
            return;
        }
        m_stopping.store(false, std::memory_order_release);
        BLUE_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCounts);
        for (size_t i = 0; i < m_threadCounts; i++)
        {
            m_threads[i].reset(new blue::Mthread(std::bind(&Schedular::run, this),
                                                 "sub_" + m_name + "_" + std::to_string(i + 1)));
            m_threadIds.push_back(m_threads[i]->getID());
        }

        lock.unlock();

        // 放到stop启动,支持开启后提交任务
        // if (m_mainfiber)
        // {
        //     m_mainfiber->call();
        //     BLUE_LOG_INFO(g_logger) << "call out m_mainfiber id : " <<
        //     m_mainfiber->GetFiberID() << " status : " << (int)m_mainfiber->getStatus();
        // }
    }

    void Schedular::stop()
    {
        BLUE_LOG_INFO(g_logger) << "stop";

        m_autoStopping.store(true, std::memory_order_release);
        m_stopping.store(true, std::memory_order_release);

        // 如果总共只有一个工作线程(即主线程)
        if (m_mainfiber &&
            m_threadCounts == 0 &&
            (m_mainfiber->getStatus() == blue::Fiber::Status::INIT ||
             m_mainfiber->getStatus() == blue::Fiber::Status::TERM))
        {
            BLUE_LOG_INFO(g_logger) << "fiber id : " << blue::Fiber::GetFiberID() << " stopping ";
            if (stopping())
            {
                return;
            }
        }

        if (m_mainThreadId != -1)
        {
            BLUE_ASSERT(GetThis() == this);
        }
        else
        {
            BLUE_ASSERT(GetThis() != this);
        }

        // 其他工作线程tickle
        for (size_t i = 0; i < m_threadCounts; i++)
        {
            tickle();
        }

        // 调度器的主协程(线程主协程的子协程)tickle
        // 如果有主调度器协程，也需要唤醒
        if (m_mainfiber)
        {
            tickle();
            // // 只在有任务且未停止时才切换(2026-4-12修改了swapIn,swapOut的逻辑，不再去使用if,else在swap和call,back之间来回)\
            切换,故而之前所有的协程上下文异常都是这个判断导致的,现在也无需这样写了
            // if (!m_fibers.empty() && !stopping()) {
            //     BLUE_LOG_INFO(g_logger) << "stop: switching to main fiber";
            //     m_mainfiber->call();
            // }
            if (!stopping())
            {
                BLUE_LOG_INFO(g_logger) << "stop: switching to main fiber";
                m_mainfiber->call();
            }
        }

        // 等待所有工作线程结束
        std::vector<blue::Mthread::MthreadPtr> thr;
        {
            MmutexType::lockSco lock(m_mutex);
            thr.swap(m_threads);
        }
        for (auto &it : thr)
        {
            it->join();
        }

        BLUE_LOG_INFO(g_logger) << "stop completed";
    }

    void Schedular::setThis()
    {
        t_schedularptr = this;
    }

    void Schedular::run()
    {
        BLUE_LOG_INFO(g_logger) << "run";

        // 设置hook,同步原语异步化
        blue::set_hook_enable(true);

        setThis();

        // 不是主线程(将调度器的主协程设为当前线程的执行协程\
        这样后面协程切换的时候要么就是各自线程的调度器的主协程(t_mainfiber)跟调度器下面的执行协程\
        进行切换,要么就是线程的主协程(Fiber::t_mainfiber)跟线程的调度器的主协程切换即t_mainfiebr)
        if (blue::GetThreadId() != m_mainThreadId)
        {
            // t_mainfiber具有线程局部性,每个线程各持有一份，不会互相影响
            t_mainfiber = blue::Fiber::GetThis().get();
        }

        // 休闲fiber
        blue::Fiber::FiberPtr idle_fiber = std::make_shared<blue::Fiber>(std::bind(&Schedular::idle, this));

        blue::Fiber::FiberPtr cb_fiber; // 回调fiber

        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool has_task = false;
            {
                MmutexType::lockSco lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    // 既不是主线程的任务，也不是当前进来的线程的任务
                    if (it->threadId != -1 && it->threadId != blue::GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }
                    BLUE_ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getStatus() == blue::Fiber::Status::EXEC)
                    {
                        ++it;
                        continue;
                    }
                    ft = *it;
                    m_fibers.erase(it);
                    ++m_activeThreadCounts;
                    has_task = true;
                    // BLUE_LOG_INFO(g_logger) << "++active, now=" << m_activeThreadCounts.load();
                    break;
                }
            }
            if (tickle_me)
            {
                tickle();
            }
            if (ft.fiber && ft.fiber->getStatus() != blue::Fiber::Status::TERM &&
                ft.fiber->getStatus() != blue::Fiber::Status::EXCEPT)
            {
                ft.fiber->swapIn();
                m_activeThreadCounts.fetch_sub(1, std::memory_order_acq_rel);
                // BLUE_LOG_INFO(g_logger) << "--active (cb), now=" << m_activeThreadCounts.load();
                if (ft.fiber->getStatus() == blue::Fiber::Status::READY)
                {
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getStatus() != blue::Fiber::Status::TERM &&
                         ft.fiber->getStatus() != blue::Fiber::Status::EXCEPT)
                {
                    ft.fiber->setStatus(blue::Fiber::Status::HOLD);
                }
                // ft.reset();
            }
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new blue::Fiber(ft.cb));
                }
                // ft.reset();

                cb_fiber->swapIn();
                m_activeThreadCounts.fetch_sub(1, std::memory_order_acq_rel);
                // BLUE_LOG_INFO(g_logger) << "--active (cb), now=" << m_activeThreadCounts.load();
                if (cb_fiber->getStatus() == blue::Fiber::Status::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getStatus() == blue::Fiber::Status::EXCEPT ||
                         cb_fiber->getStatus() == blue::Fiber::Status::TERM)
                {
                    cb_fiber->reset(nullptr);
                }
                else // != EXCEPT and != TERM
                {
                    cb_fiber->setStatus(blue::Fiber::Status::HOLD);
                    cb_fiber.reset();
                }
            }
            else
            {
                if (has_task)
                {
                    m_activeThreadCounts.fetch_sub(1, std::memory_order_acq_rel);
                    continue;
                }
                if (idle_fiber->getStatus() == blue::Fiber::Status::TERM)
                {
                    BLUE_LOG_INFO(g_logger) << "idle fiber term , id : "
                                            << idle_fiber->getId();
                    break;
                }
                m_idleThreadCounts.fetch_add(1, std::memory_order_acq_rel);
                idle_fiber->swapIn();
                m_idleThreadCounts.fetch_sub(1, std::memory_order_acq_rel);
                if (idle_fiber->getStatus() != blue::Fiber::Status::TERM &&
                    idle_fiber->getStatus() != blue::Fiber::Status::EXCEPT)
                {
                    idle_fiber->setStatus(blue::Fiber::Status::HOLD);
                }
            }
        }
    }

    // 给调度器传递一个信号，唤醒等待的线程
    void Schedular::tickle()
    {
        BLUE_LOG_INFO(g_logger) << "tickle";
    }

    // 是否停止
    bool Schedular::stopping()
    {
        bool no_fibers = false;
        {
            MmutexType::lockSco lock(m_mutex);
            no_fibers = m_fibers.empty();
        }
        bool result = m_autoStopping.load(std::memory_order_acquire) &&
                      m_stopping.load(std::memory_order_acquire) &&
                      no_fibers &&
                      m_activeThreadCounts.load(std::memory_order_acquire) == 0;

        // static thread_local bool last_result = false;
        // if (last_result != result) {
        //     BLUE_LOG_INFO(g_logger) << "stopping changed: " << last_result << " -> " << result
        //                             << ", active=" << m_activeThreadCounts.load()
        //                             << ", empty=" << no_fibers;
        //     last_result = result;
        // }

        return result;
    }

    // 休闲任务
    void Schedular::idle()
    {
        BLUE_LOG_INFO(g_logger) << "idle fiber started";
        while (!stopping())
        {
            blue::Fiber::YieldToHold();
        }
    }
}