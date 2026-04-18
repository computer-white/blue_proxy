#ifndef __BLUE_IOMANAGER_H__
#define __BLUE_IOMANAGER_H__
#include <unordered_map>
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
            // 句柄及对应的事件
            struct FdContext
            {
                using MmutexType = Mmutex;
                // 事件
                struct EventContext
                {
                    Schedular* schedular;       // 事件的schedular
                    Fiber::FiberPtr fiber;      // 事件的协程
                    std::function<void()> cb;   // 事件回调函数
                };

                // 根据event事件获取对应的任务
                EventContext& getEventContext(Event event);
                // 清空任务
                void resetEventContext(EventContext& ec);
                // 删除m_events中的event,并提交event事件对应的任务
                void triggerContext(Event event);

                MmutexType mutex;
                EventContext read;                              // 读事件
                EventContext write;                             // 写事件
                int fd = 0;                                     // 事件关联的句柄
                Event m_events = Event::NONE;                   // 已注册的事件
            };
        public:
            /*
                IOManager构造函数
                threads : 总线程个数
                use_caller : 是否使用主线程执行任务
                name : 主线程名称
            */
            IOManager(size_t threads = 1,bool use_caller = true,const std::string& name = "");
            ~IOManager() override;
            
            /*
                添加任务 
                fd : 事件的句柄,同一个句柄可以有多个不同的事件
                event : 要添加的事件
                cb : 若为nullptr,使用Fiber::GetThis() 当前协程
                return : success : 0 , error : -1
            */
            int addEvent(int fd,Event event,std::function<void()> cb = nullptr);

            /*
                删除任务
                fd : 多个事件的句柄
                event : 要删除的事件
            */
            bool delEvent(int fd,Event event);

            /*
                取消任务
                fd : 多个事件的句柄
                event : 要取消的事件
            */
            bool cancelEvent(int fd,Event event);

            /*
                取消fd句柄下所有的任务
            */
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
            // m_fdContext设置
            void contextSet(int fd = -1);
        private:
            MRWmutexType m_mutex;                               // 互斥锁
            int m_epfd = 0;                                     // epoll_create返回的句柄,监听的事件集合的句柄
            int m_ticklefds[2];                                 // 文件描述符,m_ticklefds[0]表示读,[1]表示写
            std::atomic<size_t> m_pendingEventCounts = {0};     // 等待执行的任务计数
            // std::vector<FdContext*> m_fdContexts;   
            std::unordered_map<int,FdContext*> m_fdContexts;    // 改用map存储fd文件描述符对应的事件        
    };
} // namespace blue


#endif
