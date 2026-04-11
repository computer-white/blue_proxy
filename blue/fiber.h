#ifndef __BLUE_FIBER_H__
#define __BLUE_FIBER_H__
#include <memory>
#include <atomic>
#include <ucontext.h> // 协程
#include <functional>
#include "mthread.h"
namespace blue
{
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        public:
            using FiberPtr = std::shared_ptr<Fiber>;
            enum class Status
            {
                INIT,       // init
                HOLD,       // hold
                EXEC,       // exec
                TERM,       // term(结束)
                READY,      // ready
                EXCEPT      // 错误
            };


        private:
            Fiber(bool Create,Status init);
            // call(调度器的主协程和调度器下面的协程的切换),作为swapIn的辅助函数
            void call();
            // back(调度器的主协程和调度器下面的协程的切换),作为swapOut的辅助函数
            void back();
        public:
            Fiber() = default;
            Fiber(std::function<void()> cb,size_t stacksize = 0);
            ~Fiber();
            // 重置协程函数,并重置状态
            void reset(std::function<void()> cb);
            // 切换到当前协程执行
            void swapIn();
            // 当前协程切换到后台       
            void swapOut();
            // 获取协程id
            uint64_t getId() const { return m_id; } 
            // 获取协程状态
            Status getStatus() const { return m_status.load(std::memory_order_acquire); }
            // 设置协程状态
            void setStatus(Status st) { m_status.store(st,std::memory_order_release); } 
        public:
            // 创建主协程
            static FiberPtr CreateMainFiber();
            // 设置当前执行的协程
            static void SetThis(Fiber* f);
            // 返回当前执行的协程
            static Fiber::FiberPtr GetThis();
            // 协程切换到后台,并设为Ready状态
            static void YieldToReady();
            // 协程切换到后台,并设为Hold状态
            static void YieldToHold();
            // 协程总数
            static uint64_t TotalFibers();
            // 获取id
            static uint64_t GetFiberID();
            static void MainFunc();
        private:
            uint64_t m_id = 0;                                          
            uint32_t m_stacksize = 0;
            std::atomic<Status> m_status = Status::INIT;

            ucontext_t m_ctx;
            void* m_stack = nullptr;

            std::function<void()> m_cb;
    }; 
}

#endif // __BLUE_FIBER_H__