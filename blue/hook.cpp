#include <dlfcn.h>
#include <stdarg.h>
#include "config.h"
#include "fdmanager.h"
#include "fiber.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"
namespace blue
{
	static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

	static blue::ConfigVar<int>::ConfigVarPtr g_tcp_connect_timeout =
		blue::Config::Lookup<int>("tcp.connect.timeout", 10000u, "tcp connect timeout");

	static thread_local bool t_hook_enable = false;
#define HOOK_FUNC(XX) \
	XX(sleep)         \
	XX(usleep)        \
	XX(nanosleep)     \
	XX(socket)        \
	XX(connect)       \
	XX(accept)        \
	XX(read)          \
	XX(readv)         \
	XX(recvfrom)      \
	XX(recvmsg)       \
	XX(recv)          \
	XX(write)         \
	XX(writev)        \
	XX(send)          \
	XX(sendto)        \
	XX(sendmsg)       \
	XX(close)         \
	XX(fcntl)         \
	XX(ioctl)         \
	XX(getsockopt)    \
	XX(setsockopt)

	// hook初始化
	void hook_initial()
	{
		static bool is_inited = false;
		if (is_inited)
		{
			return;
		}
#define XX(name) name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
		HOOK_FUNC(XX);
#undef XX
	}
	static uint64_t s_connect_timeout = -1;
	// ----main函数前会进行初始化
	struct __HookIniter__
	{
		__HookIniter__()
		{
			hook_initial();
			s_connect_timeout = g_tcp_connect_timeout->getValue();

			g_tcp_connect_timeout->addListener([](const int &old_val, const int &new_val)
											   {
				BLUE_LOG_INFO(g_logger) << "tc connect timeout changed " 
										<<"old_val : " << old_val << " new_val " << new_val;
				s_connect_timeout = new_val; });
		}
	};

	static __HookIniter__ __S_Hook_Initer__;
	// ----main函数前会进行初始化

	// 是否hook
	bool is_hook_enable()
	{
		return t_hook_enable;
	}

	// 设置hook
	void set_hook_enable(bool flag)
	{
		t_hook_enable = flag;
	}

	struct TimerInfo
	{
		std::atomic<int> cancelled = {0};
	};

	/*
		On success, read(), readv(), recv(), recvfrom(), recvmsg(), return the number of bytes read;
		write(), writev(), send(), sendto(), sendmsg() return the number of bytes written.
		return -1 if an error occurred
	*/
	template <typename OriginFunc, typename... Args>
	static ssize_t hook_io(int fd, OriginFunc func, const char *hook_func_name,
						   uint32_t event, int timeout_sok_type, Args &&...args)
	{
		if (!blue::is_hook_enable())
		{
			BLUE_LOG_INFO(g_logger) << hook_func_name << " hook failed";
			return func(fd, std::forward<Args>(args)...);
		}
		blue::FdCxt::FdCxtPtr ctx = blue::FdManagerPtr::GetInstance()->get(fd);
		// 不存在
		if (!ctx)
		{
			// BLUE_LOG_INFO(g_logger) << hook_func_name << " don't have cxt";
			return func(fd, std::forward<Args>(args)...);
		}

		// fd已经被关闭
		if (ctx->isClosed())
		{
			BLUE_LOG_ERROR(g_logger) << hook_func_name << "ctx has been closed";
			errno = EBADF;
			return -1;
		}

		// 不是socket或者被用户设置为非阻塞
		if (!ctx->isSocket() || ctx->getUserNonBlock())
		{
			// BLUE_LOG_INFO(g_logger) << hook_func_name << " fd not socket or is nonblock";
			return func(fd, std::forward<Args>(args)...);
		}

		// BLUE_LOG_INFO(g_logger) << hook_func_name << " hook successful";

		uint64_t timeout = ctx->getTimeout(timeout_sok_type);

		std::shared_ptr<TimerInfo> timerinfo = std::make_shared<TimerInfo>();

		while (true)
		{
			ssize_t n = func(fd, std::forward<Args>(args)...);
			// 失败并且errno处于中断(被系统打断),重新尝试
			while (n == -1 && errno == EINTR)
			{
				n = func(fd, std::forward<Args>(args)...);
			}

			// 由于操作是非阻塞IO的,但我们要的数据还没有准备好,所以系统会设置返回-1,并设置errno为EAGAIN(此时将其hook)
			if (n == -1 && errno == EAGAIN)
			{
				/*
					hook就是提交一个定时任务,当执行到定时任务的时候就取消这次因为没有数据而被阻塞(函数被阻塞)的任务
					因为这个任务最多执行这么长时间(timeout)
				*/
				blue::IOManager *iom = blue::IOManager::GetThis();
				blue::Timer::TimerPtr timer;
				std::weak_ptr<TimerInfo> weakTimerInfo(timerinfo);

				// 有自己的定时时间,提交一个带条件定时任务(到点后,就发生超时了直接取消任务就行)
				if (timeout != UINT64_MAX)
				{
					timer = iom->addConditionTimer(timeout, [weakPtr = weakTimerInfo, im = iom, fd_ = fd, event_ = event]()
												   {
							blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
							auto sharPtr = weakPtr.lock();
							if (!sharPtr || sharPtr->cancelled)
							{
								return;
							}
							// 连接超时
							sharPtr->cancelled.store(ETIMEDOUT,std::memory_order_release);
							im->cancelEvent(fd_,(blue::IOManager::Event)event_); }, weakTimerInfo);
				}

				// 提交一次任务给epoll
				int ret = iom->addEvent(fd, (blue::IOManager::Event)event);
				if (ret) // 提交失败就打印信息,取消定时任务
				{
					BLUE_LOG_ERROR(g_logger) << hook_func_name << "addEvent("
											 << fd << ", " << event << ") error";
					if (timer)
					{
						timer->cancel();
					}
					return -1;
				}
				else
				{
					// 提交成功就让出cpu,再次回来后要么epoll唤醒说明有数据可以操作了,要么被定时器唤醒(失败)
					blue::Fiber::YieldToHold();
					// BLUE_LOG_INFO(g_logger) << hook_func_name << "YieldToHold has been come back";
					/*
						cpu让出后,再一次回来了.如果定时器还存在就取消这时设置的定时器,
						如果timerinfo->cancelled有值,说明我们提交的定时任务在我们让出
						cpu这段时间被某个线程拿出来执行了,任务被取消了.所以我们直接返回
						-1(因为hook成功的函数(例如read,write)已经没有要读/写的意义了,
						此次操作失败),否则,就在定时任务还没有被执行的期间再去执行被hook
						成功的函数(可能在YieldToHold的过程中之前导致操作被阻塞的原因不
						存在了,比如read因为缓冲区没有内容而被阻塞)
					*/
					if (timer)
					{
						// BLUE_LOG_INFO(g_logger) << hook_func_name << " YieldToHold After, cancel timer";
						timer->cancel();
					}
					// 被定时器唤醒了
					if (timerinfo->cancelled.load(std::memory_order_acquire))
					{
						errno = timerinfo->cancelled;
						return -1;
					}
					continue; // 再次去执行 (ssize_t n = func(fd,std::forward<Args>(args)...);)这句
				}
			}
			return n;
		}
	}

	int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t len, uint64_t timeout)
	{
		if (!blue::is_hook_enable())
		{
			// BLUE_LOG_INFO(g_logger) << "connect hook failed";
			return connect_f(sockfd, addr, len);
		}
		blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(sockfd);
		if (!cxt || cxt->isClosed())
		{
			BLUE_LOG_INFO(g_logger) << "connect not cxt or isclosed";
			errno = EBADF;
			return -1;
		}
		if (!cxt->isSocket())
		{
			// BLUE_LOG_INFO(g_logger) << "connect not is socket";
			return connect_f(sockfd, addr, len);
		}
		// 用户自己设置了nonblock直接返回
		if (cxt->getUserNonBlock())
		{
			// BLUE_LOG_INFO(g_logger) << "nonblock has been set up";
			return connect_f(sockfd, addr, len);
		}
		// BLUE_LOG_INFO(g_logger) << "connect hook successful";
		int n = connect_f(sockfd, addr, len);
		if (n == 0)
		{
			return 0;
		}
		// 连接成功(n != -1)，或者连接失败(n == -1 && errno != EINPROGRESS)
		// errno = EINPROGRESS表示对非阻塞socket调用connect,并且连接没有立即建立
		else if (n != -1 || errno != EINPROGRESS)
		{
			return n;
		}
		// 到达这里就是n == -1 && errno = EINPROGRESS 表示对非阻塞socket调用connect,并且连接没有立即建立
		// 我们就可以给epoll注册一个connect的事件,最后利用getsockopt的SO_ERROR获取状态,如果socket上的错误码为0表示成功建立连接
		blue::IOManager *iom = blue::IOManager::GetThis();
		blue::Timer::TimerPtr timer;
		std::shared_ptr<TimerInfo> timerinfo = std::make_shared<TimerInfo>();
		std::weak_ptr<TimerInfo> weak_timerinfo(timerinfo);
		if (timeout != UINT64_MAX)
		{
			timer = iom->addConditionTimer(timeout, [im = iom, weak = weak_timerinfo, fd = sockfd]()
										   {
					blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
					auto sharPtr = weak.lock();
					if (!sharPtr || sharPtr->cancelled)
					{
						return;
					}
					// BLUE_LOG_INFO(g_logger) << "connect event will be canceled";
					sharPtr->cancelled.store(ETIMEDOUT,std::memory_order_release);
					im->cancelEvent(fd,blue::IOManager::Event::WRITE); }, weak_timerinfo);
		}
		// 提交任务给epoll
		int ret = iom->addEvent(sockfd, blue::IOManager::Event::WRITE);
		if (ret) // 提交失败就打印信息,取消定时任务
		{
			BLUE_LOG_ERROR(g_logger) << "connect addEvent("
									 << sockfd << ", blue::IOManager::Event::WRITE" << ") error";
			if (timer)
			{
				timer->cancel();
			}
			return -1;
		}
		else
		{
			// 提交成功就让出cpu,再次回来后要么epoll提醒成功connext,要么被定时器唤醒(失败)
			blue::Fiber::YieldToHold();
			// BLUE_LOG_INFO(g_logger) << "connect with timeout YieldToHold has been come back";
			/*
				cpu让出后,再一次回来了.如果定时器还存在就取消这时设置的定时器,
				如果timerinfo->cancelled有值,说明我们提交的定时任务在我们让出
				cpu这段时间被某个线程拿出来执行了,任务被取消了.所以我们直接返回
				-1
			*/
			if (timer)
			{
				// BLUE_LOG_INFO(g_logger) << "connect YieldToHold After, cancel timer";
				timer->cancel();
			}
			if (timerinfo->cancelled.load(std::memory_order_acquire))
			{
				BLUE_LOG_INFO(g_logger) << "connect event has been canceled";
				errno = timerinfo->cancelled;
				return -1;
			}
		}
		int error;
		socklen_t optlen = sizeof(errno);
		// 到这里调用getsockopt返回并清除被挂起的socket error,如果设置error为0,表示tcp三次握手真正成功(连接成功建立)
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &optlen) == -1)
		{
			return -1;
		}
		// error = 0,表示tcp三次握手成功
		if (!error)
		{
			return 0;
		}
		errno = error;
		return -1;
	}
	extern "C"
	{
#define XX(name) name##_func name##_f = nullptr;
		HOOK_FUNC(XX);
#undef XX

		// hook sleep
		unsigned int sleep(unsigned int seconds)
		{
			if (!blue::is_hook_enable())
			{
				// BLUE_LOG_INFO(g_logger) << "sleep_f(seconds)";
				return sleep_f(seconds);
			}
			blue::Fiber::FiberPtr fiber = blue::Fiber::GetThis();
			blue::IOManager *iom = blue::IOManager::GetThis();
			if (!iom || !fiber)
			{
				return sleep_f(seconds);
			}
			// BLUE_LOG_INFO(g_logger) << "sleep hook successfuly";
			iom->addTimer(seconds * 1000, [f = fiber, im = iom]()
						  { im->schedule(f); });
			blue::Fiber::YieldToHold();
			return 0;
		}

		// hook usleep
		int usleep(useconds_t usec)
		{
			if (!blue::is_hook_enable())
			{
				// BLUE_LOG_INFO(g_logger) << "usleep_f(seconds)";
				return usleep_f(usec);
			}
			blue::Fiber::FiberPtr fiber = blue::Fiber::GetThis();
			blue::IOManager *iom = blue::IOManager::GetThis();
			if (!iom || !fiber)
			{
				return usleep_f(usec);
			}
			// BLUE_LOG_INFO(g_logger) << "usleep hook successfuly";
			iom->addTimer((usec + 999) / 1000, [im = iom, f = fiber]()
						  { im->schedule(f); });
			blue::Fiber::YieldToHold();
			return 0;
		}

		// hook nanosleep
		int nanosleep(const struct timespec *req, struct timespec *rem)
		{
			if (!blue::is_hook_enable())
			{
				// BLUE_LOG_INFO(g_logger) << "nanosleep_f(seconds)";
				return nanosleep_f(req, rem);
			}
			uint64_t time_out = req->tv_sec * 1000 + (req->tv_nsec + 999999) / 1000000;
			blue::Fiber::FiberPtr fiber = blue::Fiber::GetThis();
			blue::IOManager *iom = blue::IOManager::GetThis();
			if (!iom || !fiber)
			{
				return nanosleep_f(req, rem);
			}
			// BLUE_LOG_INFO(g_logger) << "nanosleep hook successfuly";
			iom->addTimer(time_out, [im = iom, f = fiber]()
						  { im->schedule(f); });
			blue::Fiber::YieldToHold();
			return 0;
		}

		// hook socket相关
		int socket(int domain, int type, int protocol)
		{
			if (!blue::is_hook_enable())
			{
				return socket_f(domain, type, protocol);
			}
			// BLUE_LOG_INFO(g_logger) << "socket hook successfuly";
			int fd = socket_f(domain, type, protocol);
			if (fd == -1)
			{
				BLUE_LOG_ERROR(g_logger) << "socket failed";
				return fd;
			}
			// 这里会添加(没有就创建)
			// BLUE_LOG_INFO(g_logger) << "socket successful";
			blue::FdManagerPtr::GetInstance()->get(fd, true);
			return fd;
		}

		int connect(int sockfd, const struct sockaddr *addr, socklen_t len)
		{
			return connect_with_timeout(sockfd, addr, len, blue::s_connect_timeout);
		}

		int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
		{
			int fd = hook_io(sockfd, accept_f, "accept",
							 blue::IOManager::Event::READ, SO_RCVTIMEO,
							 addr, addrlen);
			if (fd >= 0)
			{
				blue::FdManagerPtr::GetInstance()->get(fd, true);
			}
			return fd;
		}

		// hook read相关
		ssize_t read(int fd, void *buf, size_t count)
		{
			return hook_io(fd, read_f, "read",
						   blue::IOManager::Event::READ, SO_RCVTIMEO,
						   buf, count);
		}

		// 分散读
		ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
		{
			return hook_io(fd, readv_f, "readv",
						   blue::IOManager::Event::READ, SO_RCVTIMEO,
						   iov, iovcnt);
		}

		ssize_t recv(int sockfd, void *buf, size_t len, int flags)
		{
			return hook_io(sockfd, recv_f, "recv",
						   blue::IOManager::Event::READ, SO_RCVTIMEO,
						   buf, len, flags);
		}

		ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
						 struct sockaddr *src_addr, socklen_t *addrlen)
		{
			return hook_io(sockfd, recvfrom_f, "recvfrom",
						   blue::IOManager::Event::READ, SO_RCVTIMEO,
						   buf, len, flags, src_addr, addrlen);
		}

		ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
		{
			return hook_io(sockfd, recvmsg_f, "recvmsg",
						   blue::IOManager::Event::READ, SO_RCVTIMEO,
						   msg, flags);
		}

		// hook write相关
		ssize_t write(int fd, const void *buf, size_t count)
		{
			return hook_io(fd, write_f, "write",
						   blue::IOManager::Event::WRITE, SO_SNDTIMEO,
						   buf, count);
		}

		// 聚集写
		ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
		{
			return hook_io(fd, writev_f, "writev",
						   blue::IOManager::Event::WRITE, SO_SNDTIMEO,
						   iov, iovcnt);
		}

		ssize_t send(int sockfd, const void *buf, size_t len, int flags)
		{
			return hook_io(sockfd, send_f, "send",
						   blue::IOManager::Event::WRITE, SO_SNDTIMEO,
						   buf, len, flags);
		}

		ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
					   const struct sockaddr *dest_addr, socklen_t addrlen)
		{
			return hook_io(sockfd, sendto_f, "sendto",
						   blue::IOManager::Event::WRITE, SO_SNDTIMEO,
						   buf, len, flags, dest_addr, addrlen);
		}

		ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
		{
			return hook_io(sockfd, sendmsg_f, "sendmsg",
						   blue::IOManager::Event::WRITE, SO_SNDTIMEO,
						   msg, flags);
		}

		// hook close相关
		int close(int fd)
		{
			if (!blue::is_hook_enable())
			{
				return close_f(fd);
			}
			// BLUE_LOG_INFO(g_logger) << "close hook successfuly";
			blue::FdCxt::FdCxtPtr ctx = blue::FdManagerPtr::GetInstance()->get(fd);
			if (ctx)
			{
				auto iom = blue::IOManager::GetThis();
				if (iom)
				{
					iom->cancelAll(fd);
				}
				// 删除fd相关的事件
				blue::FdManagerPtr::GetInstance()->del(fd);
			}
			return close_f(fd);
		}

		// hook set/get socket option 相关
		int fcntl(int fd, int cmd, ... /* arg */)
		{
			va_list va;
			va_start(va, cmd);
			switch (cmd)
			{
			case F_SETFL:
			{
				int arg = va_arg(va, int);
				va_end(va);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return fcntl_f(fd, cmd, arg);
				}
				// 设置用户nonblock
				cxt->setUserNonBlock(arg & O_NONBLOCK);
				if (cxt->getSysNonBlock())
				{
					arg |= O_NONBLOCK;
				}
				else
				{
					arg &= ~O_NONBLOCK;
				}
				return fcntl_f(fd, cmd, arg);
			}
			break;
			case F_GETFL:
			{
				va_end(va);
				int ans = fcntl_f(fd, cmd);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return ans;
				}
				if (cxt->getUserNonBlock())
				{
					return ans | O_NONBLOCK;
				}
				else
				{
					return ans & ~O_NONBLOCK;
				}
			}
			break;
			case F_DUPFD:
			case F_DUPFD_CLOEXEC:
			case F_SETFD:
			case F_SETOWN:
			case F_SETSIG:
			case F_SETLEASE:
			case F_NOTIFY:
			case F_SETPIPE_SZ:
			{
				int arg = va_arg(va, int);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;

			case F_GETFD:
			case F_GETOWN:
			case F_GETSIG:
			case F_GETLEASE:
			case F_GETPIPE_SZ:
			{
				va_end(va);
				return fcntl_f(fd, cmd);
			}
			break;

			case F_SETLK:
			case F_SETLKW:
			case F_GETLK:
			{
				struct flock *arg = va_arg(va, struct flock *);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;
			case F_GETOWN_EX:
			case F_SETOWN_EX:
			{
				struct f_owner_ex *arg = va_arg(va, struct f_owner_ex *);
				va_end(va);
				return fcntl_f(fd, cmd, arg);
			}
			break;
			default:
				va_end(va);
				return fcntl_f(fd, cmd);
			}
		}

		int ioctl(int fd, unsigned long request, ...)
		{
			va_list va;
			va_start(va, request);
			void *arg = va_arg(va, void *);
			va_end(va);
			if (request == FIONBIO)
			{
				bool user_nonblock = (*((int *)(arg)) != 0);
				blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(fd);
				if (!cxt || cxt->isClosed() || !cxt->isSocket())
				{
					return ioctl_f(fd, request, arg);
				}
				cxt->setUserNonBlock(user_nonblock);
			}
			return ioctl_f(fd, request, arg);
		}

		int getsockopt(int sockfd, int level, int optname,
					   void *optval, socklen_t *optlen)
		{
			return getsockopt_f(sockfd, level, optname, optval, optlen);
		}

		int setsockopt(int sockfd, int level, int optname,
					   const void *optval, socklen_t optlen)
		{
			if (!blue::is_hook_enable())
			{
				return setsockopt_f(sockfd, level, optname, optval, optlen);
			}
			// BLUE_LOG_INFO(g_logger) << "setsockopt hook successfuly";
			if (level == SOL_SOCKET)
			{
				if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
				{
					blue::FdCxt::FdCxtPtr cxt = blue::FdManagerPtr::GetInstance()->get(sockfd);
					if (cxt)
					{
						struct timeval *tv = (struct timeval *)optval;
						cxt->setTimeout(optname, tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000);
					}
				}
			}
			return setsockopt_f(sockfd, level, optname, optval, optlen);
		}
	}
#undef HOOK_FUNC
}
