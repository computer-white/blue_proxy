#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include "hook.h"
namespace blue
{
    static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::Event::READ:
            return read;
        case IOManager::Event::WRITE:
            return write;
        default:
            BLUE_ASSERT2(false, "getEventContext");
            break;
        }
    }

    void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext &ec)
    {
        ec.schedular = nullptr;
        ec.fiber.reset();
        ec.cb = nullptr;
    }

    void IOManager::FdContext::triggerContext(IOManager::Event event)
    {
        BLUE_ASSERT(m_events & event);
        m_events = (Event)(m_events & ~event);
        EventContext &ctx = getEventContext(event);
        if (ctx.cb)
        {
            ctx.schedular->schedule(std::move(ctx.cb));
        }
        if (ctx.fiber)
        {
            ctx.schedular->schedule(std::move(ctx.fiber));
        }
        ctx.schedular = nullptr;
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) : Schedular(threads, use_caller, name)
    {
        m_epfd = epoll_create(5000);
        BLUE_ASSERT(m_epfd > 0);
        // pipe,传入文件描述符数组,fd[0]为读打开,fd[1]为写打开,fd[1]的输出是fd[0]的输入
        int rt = pipe(m_ticklefds); // 成功返回0,m_ticklefds[1]的输出是m_ticklefds[0]的输入
        BLUE_ASSERT(rt == 0);
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET; // 边缘触发 EPOLLET
        event.data.fd = m_ticklefds[0]; // 设置文件描述符

        // 设置读文件描述符状态为非阻塞模式(O_NONBLOCK)
        rt = fcntl(m_ticklefds[0], F_SETFL, O_NONBLOCK);
        BLUE_ASSERT(rt == 0);
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_ticklefds[0], &event);
        BLUE_ASSERT(rt == 0);
        contextResize(64);
        Schedular::start();
    }

    IOManager::~IOManager()
    {
        Schedular::stop();
        close(m_epfd);
        close(m_ticklefds[0]);
        close(m_ticklefds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); i++)
        {
            if (m_fdContexts[i] != nullptr)
            {
                delete m_fdContexts[i];
            }
        }
    }

    // success : 0 , error : -1
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        MRWmutexType::ReadlockSco lock1(m_mutex);
        FdContext *fd_ctx = nullptr;
        if (m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock1.unlock();
        }
        else
        {
            lock1.unlock();
            MRWmutexType::WritelockSco lock2(m_mutex);
            contextResize(fd * 2);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);
        if (fd_ctx->m_events & event)
        {
            BLUE_LOG_ERROR(g_logger) << "addEvent assert id : " << fd
                                     << " event : " << event
                                     << " fc->m_events : " << fd_ctx->m_events;
            BLUE_ASSERT(!(fd_ctx->m_events & event));
        }
        int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event e_event;
        e_event.events = EPOLLET | fd_ctx->m_events | event;
        e_event.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &e_event);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << e_event.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return -1;
        }
        ++m_pendingEventCounts;
        fd_ctx->m_events = (Event)(fd_ctx->m_events | event);
        FdContext::EventContext &event_context = fd_ctx->getEventContext(event);
        BLUE_ASSERT(!event_context.schedular &&
                    !event_context.fiber &&
                    !event_context.cb);

        event_context.schedular = Schedular::GetThis();
        if (cb)
        {
            event_context.cb.swap(cb);
        }
        else
        {
            event_context.fiber = Fiber::GetThis();
            BLUE_ASSERT(event_context.fiber->getStatus() == Fiber::Status::EXEC);
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }
        Event new_event = (Event)(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        --m_pendingEventCounts;
        fd_ctx->m_events = new_event;
        FdContext::EventContext &event_context = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(event_context);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }
        Event new_event = (Event)(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        fd_ctx->triggerContext(event);
        --m_pendingEventCounts;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);
        if (!fd_ctx->m_events)
        {
            return false;
        }
        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }
        if (fd_ctx->m_events & Event::READ)
        {
            fd_ctx->triggerContext(Event::READ);
            --m_pendingEventCounts;
        }
        if (fd_ctx->m_events & Event::WRITE)
        {
            fd_ctx->triggerContext(Event::WRITE);
            --m_pendingEventCounts;
        }
        BLUE_ASSERT(fd_ctx->m_events == 0);
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Schedular::GetThis());
    }

    void IOManager::tickle()
    {
        // 没有idle直接退出
        if (!Schedular::hasIdleThreads())
        {
            return;
        }
        int rt = write(m_ticklefds[1], "T", 1);
        BLUE_ASSERT(rt == 1);
    }

    bool IOManager::stopping()
    {
        // BLUE_LOG_INFO(g_logger) << " m_pendingEventCOunts : " << m_pendingEventCounts
        //                         << " hasTimer : " << TimerManager::hasTimer()
        //                         << " stopping : " << Schedular::stopping();
        return m_pendingEventCounts == 0 
        && !TimerManager::hasTimer() 
        && Schedular::stopping();
    }

    void IOManager::idle()
    {
        epoll_event *epevent = new epoll_event[64]();
        std::shared_ptr<epoll_event> shared_event(epevent, [](epoll_event *ptr)
                                                  { delete[] ptr; });
        while (true)
        {
            // uint64_t next_timeout = 0;
            if (stopping())
            {
                BLUE_LOG_INFO(g_logger) << "name : " << getName() << " io idle stopping";
                break;
            }

            int rt = 0;
            do
            {
                static const int MAX_WAIT = 3000;
                uint64_t next_timeout = blue::TimerManager::getNextTime();
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_WAIT ? MAX_WAIT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_WAIT;
                }
                // BLUE_LOG_DEBUGE(g_logger) << "next_timeout : " << next_timeout;
                rt = epoll_wait(m_epfd, epevent, 64, (int)next_timeout);
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                else
                {
                    break;
                }
            } while (true);

            // 从Timer中取出超时任务执行
            std::vector<std::function<void()>> cbs;
            blue::TimerManager::listExpiredCb(cbs);
            if (!cbs.empty())
            {
                BLUE_LOG_DEBUGE(g_logger) << "cbs.size : " << cbs.size();

                // 提交超时任务
                blue::Schedular::schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            for (int i = 0; i < rt; i++)
            {
                epoll_event &event = epevent[i];
                if (event.data.fd == m_ticklefds[0])
                {
                    uint8_t dummy;
                    while (read(m_ticklefds[0], &dummy, 1) == 1)
                        ;
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MmutexType::lockSco lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= EPOLLIN | EPOLLOUT;
                }
                int real_event = Event::NONE;
                if (event.events & EPOLLIN)
                {
                    real_event |= Event::READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_event |= Event::WRITE;
                }
                if ((fd_ctx->m_events & real_event) == Event::NONE)
                {
                    continue;
                }

                int left_events = (fd_ctx->m_events & ~real_event);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                             << "," << op << "," << fd_ctx->fd << "," << event.events << "):"
                                             << rt2 << "(" << errno << "," << strerror(errno) << ")";
                    continue;
                }

                if (real_event & Event::READ)
                {
                    fd_ctx->triggerContext(Event::READ);
                    --m_pendingEventCounts;
                }
                if (real_event & Event::WRITE)
                {
                    fd_ctx->triggerContext(Event::WRITE);
                    --m_pendingEventCounts;
                }
            }
            Fiber::FiberPtr curr = Fiber::GetThis();
            auto curr_ptr = curr.get();
            curr.reset();

            curr_ptr->swapOut();
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < size; i++)
        {
            if (m_fdContexts[i] == nullptr)
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
} // namespace blue
