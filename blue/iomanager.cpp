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
        // 哪个对象调用删除哪个对象的event事件,并提交event任务
        m_events = (Event)(m_events & ~event);
        // 获取event事件对应的任务
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
        // 现代linux对epoll_create的实现,可以忽略size只要传入的大于0就行
        // 只要传一个大于 0 的数就行。内核会根据你实际添加的 fd 数量，
        // 完全动态地去管理红黑树的内存，用多少，分多少
        // 所以这个就相当于是一个动态的epoll管理器
        m_epfd = epoll_create(1);
        BLUE_ASSERT(m_epfd > 0);

        // pipe,传入文件描述符数组,fd[0]为读打开,fd[1]为写打开,fd[1]的输出是fd[0]的输入
        // 创建管道用于读和写
        int rt = pipe(m_ticklefds); // 成功返回0,m_ticklefds[1]的输出是m_ticklefds[0]的输入

        BLUE_ASSERT(rt == 0);

        // 设置默认epoll event
        epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN | EPOLLET; // 边缘触发 EPOLLET
        event.data.fd = m_ticklefds[0];   // 设置文件描述符

        // 设置读文件描述符状态为非阻塞模式(O_NONBLOCK)
        rt = fcntl(m_ticklefds[0], F_SETFL, O_NONBLOCK);
        BLUE_ASSERT(rt == 0);

        // 添加(EPOLL_CTL_ADD)一个监听对象(m_ticklefds[0]),并注册一个fd相关的event事件的就绪回调,
        // 若这个对象(读端)被写入数据,则触发可读事件
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_ticklefds[0], &event);
        BLUE_ASSERT(rt == 0);

        contextResize(64);
        // contextSet(0);
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

        // for (auto it : m_fdContexts)
        // {
        //     delete it.second;
        // }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        MRWmutexType::ReadlockSco lock1(m_mutex);
        FdContext *fd_ctx = nullptr;
        // auto it = m_fdContexts.find(fd);
        // if (it == m_fdContexts.end())
        // {
        //     lock1.unlock();
        //     MRWmutexType::WritelockSco lock(m_mutex);
        //     if (it == m_fdContexts.end())
        //     {
        //         contextSet(fd);
        //         fd_ctx = m_fdContexts[fd];
        //     }
        // }
        // else
        // {
        //     fd_ctx = it->second;
        //     lock1.unlock();
        // }

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
        // 重复提交相同任务
        if (fd_ctx->m_events & event)
        {
            BLUE_LOG_ERROR(g_logger) << "addEvent assert id : " << fd
                                     << " event : " << event
                                     << " fc->m_events : " << fd_ctx->m_events;
            BLUE_ASSERT(!(fd_ctx->m_events & event));
        }
        // 操作 EPOLL_CTL_MOD 修改, EPOLL_CTL_ADD 添加
        int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        Event newevent = (Event)(fd_ctx->m_events | event);
        epoll_event e_event;
        e_event.events = EPOLLET | (unsigned)newevent;
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
        // 将event添加为已注册的事件
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
        // auto it = m_fdContexts.find(fd);
        // if (it == m_fdContexts.end())
        // {
        //     return false;
        // }
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        // FdContext *fd_ctx = it->second;
        lock.unlock();

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }
        Event new_event = (Event)(fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | (unsigned)new_event;
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
        // 删除某些事件后的新的事件
        fd_ctx->m_events = new_event;

        FdContext::EventContext &event_context = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(event_context);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        // auto it = m_fdContexts.find(fd);
        // if (it == m_fdContexts.end())
        // {
        //     return false;
        // }
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        // FdContext *fd_ctx = it->second;
        lock.unlock();

        FdContext::MmutexType::lockSco lock2(fd_ctx->mutex);

        // 没有事件无需取消
        if (!(fd_ctx->m_events & event))
        {
            return false;
        }

        // 删除掉传进来的event后,加入新的监听
        Event new_event = (Event)(fd_ctx->m_events & ~event);

        // 删除或修改
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        // epevent
        epoll_event epevent;
        epevent.events = EPOLLET | (unsigned)new_event;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            BLUE_LOG_ERROR(g_logger) << "epoll_ctl error(" << m_epfd
                                     << "," << op << "," << fd << "," << epevent.events << "):"
                                     << rt << "(" << errno << "," << strerror(errno) << ")";
            return false;
        }

        // triggerContext包含删除fd_ctx的event事件
        fd_ctx->triggerContext(event);
        --m_pendingEventCounts;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        MRWmutexType::ReadlockSco lock(m_mutex);
        // auto it = m_fdContexts.find(fd);
        // if (it == m_fdContexts.end())
        // {
        //     return false;
        // }
        if (m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        // FdContext *fd_ctx = it->second;
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
        int rt = write(m_ticklefds[1], "iomanager::tickle", 17);
        BLUE_ASSERT(rt == 17);
    }

    bool IOManager::stopping()
    {

        return m_pendingEventCounts == 0 && !TimerManager::hasTimer() && Schedular::stopping();
    }

    void IOManager::idle()
    {
        // auto epevent = std::make_unique<epoll_event[]>(64);
        std::array<epoll_event, 64> epevent;

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
                rt = epoll_wait(m_epfd, &epevent[0], 64, (int)next_timeout);
                // 非阻塞IO,返回-1并设置errno = EINTR表示我们要的任务还没有被准备好
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                else
                {
                    break; // 有任务了
                }
            } while (true);

            // 从Timer中取出超时任务执行
            std::vector<std::function<void()>> cbs;
            blue::TimerManager::listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // BLUE_LOG_DEBUGE(g_logger) << "cbs.size : " << cbs.size();

                // 提交超时任务
                blue::Schedular::schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // 处理可以执行的任务(epoll_wait返回的任务数)
            for (int i = 0; i < rt; i++)
            {
                epoll_event &event = epevent[i];
                if (event.data.fd == m_ticklefds[0])
                {
                    // uint32_t dummy;
                    char dummy[18];
                    while (read(m_ticklefds[0], &dummy, 17) == 17)
                    {
                        // BLUE_LOG_INFO(g_logger) << dummy;
                    };
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
                // 没有事件
                if ((fd_ctx->m_events & real_event) == Event::NONE)
                {
                    continue;
                }

                // 把事件拿出来后在epoll里面要么删除要么更改
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

                // 执行任务
                // if (real_event & Event::READ)
                // {
                //     fd_ctx->triggerContext(Event::READ);
                //     --m_pendingEventCounts;
                // }
                // if (real_event & Event::WRITE)
                // {
                //     fd_ctx->triggerContext(Event::WRITE);
                //     --m_pendingEventCounts;
                // }
                int trigger_events = real_event & fd_ctx->m_events;
                if (trigger_events & Event::READ)
                {
                    fd_ctx->triggerContext(Event::READ);
                    --m_pendingEventCounts;
                }
                if (trigger_events & Event::WRITE)
                {
                    fd_ctx->triggerContext(Event::WRITE);
                    --m_pendingEventCounts;
                }
            }
            Fiber::FiberPtr curr = Fiber::GetThis();
            auto curr_ptr = curr.get();
            curr.reset();

            curr_ptr->swapOut(); // idle协程切换到后台
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

    void IOManager::contextSet(int fd)
    {
        m_fdContexts[fd] = new FdContext;
        m_fdContexts[fd]->fd = fd;
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
} // namespace blue
