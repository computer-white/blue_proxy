#include <dlfcn.h>
#include "hook.h"
#include "fiber.h"
#include "log.h"
#include "iomanager.h"
namespace blue
{
	static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
	static thread_local bool t_hook_enable = false;
	#define HOOK_FUNC(XX)\
		XX(sleep)\
		XX(usleep)
	void hook_initial() 
	{
		static bool is_inited = false;
		if (is_inited)
		{
			return;
		}
	#define XX(name) name##_f = (name##_func)dlsym(RTLD_NEXT,#name);
		HOOK_FUNC(XX);
	#undef XX
	}

	struct _HookIniter
	{
		_HookIniter() 
		{
			hook_initial();
		}
	};

	static _HookIniter s_hook_initer;

	bool is_hook_enable()
	{
		return t_hook_enable;
	}
	void set_hook_enable(bool flag)
	{
		t_hook_enable = flag;
	}

	extern "C" {
	#define XX(name) name##_func name##_f = nullptr;
		HOOK_FUNC(XX);
	#undef XX
		unsigned int sleep(unsigned int seconds)
		{
			if (!blue::t_hook_enable)
			{
				BLUE_LOG_INFO(g_logger) << "sleep_f(seconds)";
				return sleep_f(seconds);
			}
			blue::Fiber::FiberPtr fiber = blue::Fiber::GetThis();
			blue::IOManager* iom = blue::IOManager::GetThis();
			if (!iom || !fiber) 
			{
            	return sleep_f(seconds);
        	}
			BLUE_LOG_INFO(g_logger) << "hook successfuly";
			iom->addTimer(seconds*1000,[f = fiber,im = iom](){
				im->schedule(f);
				// BLUE_LOG_INFO(g_logger) << "hook's addTimer";
			});
			blue::Fiber::YieldToHold();
			return 0;
		}

		int usleep(useconds_t usec)
		{
			if (!blue::t_hook_enable)
			{
				BLUE_LOG_INFO(g_logger) << "usleep_f(seconds)";
				return usleep_f(usec);
			}
			blue::Fiber::FiberPtr fiber = blue::Fiber::GetThis();
			blue::IOManager* iom = blue::IOManager::GetThis();
			if (!iom || !fiber) 
			{
            	return usleep_f(usec);
        	}
			BLUE_LOG_INFO(g_logger) << "hook successfuly";
			iom->addTimer((usec + 999)/1000,[im = iom,f = fiber]() {
				im->schedule(f);
			});
			blue::Fiber::YieldToHold();
			return 0;
		}

	}
}
