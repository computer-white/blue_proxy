#ifndef __BLUE_IOMANAGER_H__
#define __BLUE_IOMANAGER_H__
#include "schedular.h"
#include "timer.h"
namespace blue
{
    class IOManager : public Schedular, public TimerManager
    {
        public:
            using IOManagerPtr = std::shared_ptr<IOManager>;
            using MRWmutexType = MRWmutex;

            enum Event
            {
              NONE   =    0x0,
              READ   =    0x1,  // EPOLLIN
              WRITE  =    0x4,  // EPOLLOUT
            };

        private:
            struct FdContext
            {
                using MmutexType = Mmutex;
                struct EventContext
                {
                    Schedular* schedular;       // 事件的schedular
                    Fiber::FiberPtr fiber;      // 事件的协程
                    std::function<void()> cb;   // 事件回调函数
                };

                EventContext& getEventContext(Event event);
                void resetEventContext(EventContext& ec);
                void triggerContext(Event event);
                MmutexType mutex;
                EventContext read;                              // 读事件
                EventContext write;                             // 写事件
                int fd = 0;                                     // 事件关联的句柄
                Event m_events = Event::NONE;                   // 已注册的事件
            };
        public:
            // threads : 线程个数 use_caller : 是否使用主线程执行任务 name : 主线程名称
            IOManager(size_t threads = 1,bool use_caller = true,const std::string& name = "");
            ~IOManager() override;
            
            // 添加任务
            int addEvent(int fd,Event event,std::function<void()> cb = nullptr);
            // 删除任务
            bool delEvent(int fd,Event event);
            // 取消任务
            bool cancelEvent(int fd,Event event);
            // 取消所有任务
            bool cancelAll(int fd);
            // 设置IOManager指针
            static IOManager* GetThis();
        protected:
            // 给调度器传递一个信号，唤醒等待的线程
            void tickle() override;
            // 停止调度器
            bool stopping() override;
            // idle任务函数
            void idle() override;
            // 有定时任务时,唤醒epoll_wait()
            void onTimerInsertedAtFront() override;
            // m_fdContext重新设定大小
            void contextResize(size_t size);
        private:
            MRWmutexType m_mutex;                               // 互斥锁
            int m_epfd = 0;                                     // epoll_create返回的句柄
            int m_ticklefds[2];                                 // 文件描述符,m_ticklefds[0]表示读,[1]表示写
            std::atomic<size_t> m_pendingEventCounts = {0};     // 等待执行的任务计数
            std::vector<FdContext*> m_fdContexts;               
    };
} // namespace blue


#endif
