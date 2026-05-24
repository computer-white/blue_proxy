#ifndef BLUE_FIBER_H
#define BLUE_FIBER_H
#include <memory>
#include <atomic>
#include <ucontext.h>
#include <functional>
#include "mthread.h"
#include "context.h"

// 协程库
namespace blue
{
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        using FiberPtr = std::shared_ptr<Fiber>;
        enum class Status
        {
            INIT,  // init
            HOLD,  // hold
            EXEC,  // exec
            TERM,  // term(结束)
            READY, // ready
            EXCEPT // 错误
        };

    private:
        /**
         * @brief 线程的主协程构造函数
         * @param Create 是否创建
         * @param init 协程初始状态
         * @return
         */
        Fiber(bool Create, Status init);

    public:
        /**
         * @brief 协程默认构造函数
         * @return
         */
        Fiber() = default;

        /**
         * @brief 子协程构造函数,主协程去创建新的子协程
         * @param cb 回调函数,即协程执行的函数,要完成任务
         * @param use_caller 是否使用call,back作为协程上下文切换函数,一般用于将主线程作为工作线程是启用
         * @param stacksize 协程保存上下文切换内容需要的栈大小
         * @return
         */
        Fiber(std::function<void()> cb, bool use_caller = false, size_t stacksize = 0);

        /**
         * @brief 协程析构函数(分为析构主协程和子协程)
         * @return
         */
        ~Fiber();

        /**
         * @brief 重置子协程的回调函数,并重置状态
         * @param cb 回调函数,即协程执行的函数,要完成任务
         * @return
         */
        void reset(std::function<void()> cb);

        /**
         * @brief call(线程主协程跟其他协程切换),切换为当前协程执行
         * @return
         */
        void call();

        /**
         * @brief back(线程主协程跟其他协程切换),当前协程切换到后台Hold
         * @return
         */
        void back();

        /**
         * @brief 协程调度器之间的协程互相切换,保留了没有协程调度器的用法
         * @return
         */
        void swapIn();

        /**
         * @brief 协程调度器之间的协程互相切换,保留了没有协程调度器的用法
         * @return
         */
        void swapOut();

        /**
         * @brief 获取协程id
         * @return 协程id
         */
        uint64_t getId() const { return m_id; }

        /**
         * @brief 获取协程状态
         * @return 协程状态
         */
        Status getStatus() const { return m_status.load(std::memory_order_acquire); }

        /**
         * @brief 设置协程状态
         * @param st 需要设置的协程状态
         * @return
         */
        void setStatus(Status st) { m_status.store(st, std::memory_order_release); }

    public:
        /**
         * @brief 创建主协程
         * @return
         */
        static FiberPtr CreateMainFiber();

        /**
         * @brief 设置当前执行的协程
         * @param f 当前要执行的协程指针
         * @return
         */
        static void SetThis(Fiber *f);

        /**
         * @brief 返回当前执行的协程，若没有协程，设置主协程并返回
         * @return 当前执行的协程
         */
        static Fiber::FiberPtr GetThis();

        /**
         * @brief 协程切换到后台,并设为Ready状态
         * @return
         */
        static void YieldToReady();

        /**
         * @brief  协程切换到后台,并设为Hold状态
         * @return
         */
        static void YieldToHold();

        /**
         * @brief  获取协程总数
         * @return 协程总数
         */
        static uint64_t TotalFibers();

        /**
         * @brief  获取协程id
         * @return 协程id
         */
        static uint64_t GetFiberID();

        /**
         * @brief  协程执行每个协程绑定的协程函数的开始函数
         * @return
         */
        static void MainFunc();

        /**
         * @brief  协程执行调度器主协程绑定的协程函数开始函数
         * @return
         */
        static void MainCallFunc();

    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        std::atomic<Status> m_status = Status::INIT;

        Context::ContextPtr m_ctx;
        void *m_stack = nullptr;

        std::function<void()> m_cb;
    };
}

#endif // __BLUE_FIBER_H__