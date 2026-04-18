#ifndef __BLUE_SCHEDULAR_H__
#define __BLUE_SCHEDULAR_H__
#include <memory>
#include <list>
#include "fiber.h"
#include "mthread.h"
// 协程调度模块:分配协程到对应的线程上
namespace blue
{
    class Schedular
    {
    public:
        using SchedularPtr = std::shared_ptr<Schedular>;
        using MmutexType = blue::Mmutex;
        /*
            Schedular构造函数
            threads : 总线程个数
            use_caller : 是否使用主线程执行任务
            name : 主线程名称
        */
        Schedular(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        virtual ~Schedular();
        // 获取线程名称
        const std::string &getName() const { return m_name; }
        // 获取调度器指针
        static Schedular *GetThis();
        // 设置调度器主协程
        static blue::Fiber *GetMainFiber();

        // 开启调度器
        void start();
        // 关闭调度器
        void stop();

        /*
            with lock
            fof : 任务类型(fiber or func)
            thr : 指定线程执行任务.不写则由系统默认分配
        */
        template <typename FiberOrFunc>
        void schedule(FiberOrFunc &&fof, int thr = -1)
        {
            bool need_tickle = false;
            {
                MmutexType::lockSco lock(m_mutex);
                need_tickle = scheduleNoLock(std::forward<FiberOrFunc>(fof), thr);
            }
            if (need_tickle)
            {
                tickle();
            }
        }

        /*
            with lock
            fof : 任务类型(fiber or func)
            thr : 指定线程执行任务.不写则由系统默认分配
        */
        template <typename InputIterator>
        void schedule(InputIterator begin, InputIterator end, int thr = -1)
        {
            bool need_tickle = false;
            {
                MmutexType::lockSco lock(m_mutex);
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(std::move(*begin), thr) || need_tickle;
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle();
            }
        }

    protected:
        // 给调度器传递一个信号，唤醒等待的线程
        virtual void tickle();
        // 停止调度器
        virtual bool stopping();
        // idle任务函数
        virtual void idle();
        // 设置当前调度器
        void setThis();
        // 调度器运行函数
        void run();
        // 是否还有空闲线程
        bool hasIdleThreads() { return m_idleThreadCounts.load(std::memory_order_acquire) > 0; }

    private:
        // no lock
        template <typename FiberOrFunc>
        bool scheduleNoLock(FiberOrFunc &&fof, int thr)
        {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(std::forward<FiberOrFunc>(fof), thr);
            if (ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        // FiberTask
        struct FiberAndThread
        {
            blue::Fiber::FiberPtr fiber;
            std::function<void()> cb;
            int threadId;
            // 左值拷贝右值移动
            FiberAndThread(const blue::Fiber::FiberPtr &f, int thr)
                : fiber(f), threadId(thr)
            {
            }

            FiberAndThread(blue::Fiber::FiberPtr &&f, int thr)
                : fiber(std::move(f)), threadId(thr)
            {
            }

            FiberAndThread(const std::function<void()> &c, int thr)
                : cb(c), threadId(thr)
            {
            }

            FiberAndThread(std::function<void()> &&c, int thr)
                : cb(std::move(c)), threadId(thr)
            {
            }

            FiberAndThread() : threadId(-1)
            {
            }

            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                threadId = -1;
            }
        };

    private:
        MmutexType m_mutex;                                             // mutex
        std::vector<blue::Mthread::MthreadPtr> m_threads;               // 线程池
        std::list<FiberAndThread> m_fibers;                             // 协程池(任务)
        blue::Fiber::FiberPtr m_mainfiber;                              // 主协程
        std::string m_name;                                             // 线程名称

    protected:
        std::vector<int> m_threadIds;                                   // 线程id
        size_t m_threadCounts = 0;                                      // 线程数量
        std::atomic<size_t> m_activeThreadCounts = {0};                 // 活跃线程数
        std::atomic<size_t> m_idleThreadCounts = {0};                   // 空闲线程数
        int m_mainThreadId = 0;                                         // 主线程id
        std::atomic<bool> m_stopping = true;                            // 是否停止(调度器)
        std::atomic<bool> m_autoStopping = false;                       // 如果没有任务,那么调度器可自动停止
    };
}

#endif // __BLUE_SCHEDULAR_H__
