#ifndef __BLUE_HOOK_H__
#define __BLUE_HOOK_H__
#include <unistd.h>
// 同步原语异步化

// 将系统调用的sleep()拦截,变为我们自己实现的sleep()执行一个特定时间后提交任务的函数
// 而后调用YieldToHold(),从而让线程让出cpu,实现出看起来是同步阻塞,但实际上是异步非阻塞
namespace blue
{
	bool is_hook_enable();
	void set_hook_enable(bool flag);
}
extern "C" {
	typedef unsigned int (*sleep_func)(unsigned int seconds);
	extern sleep_func sleep_f;

	typedef int (*usleep_func)(useconds_t usec);
	extern usleep_func usleep_f;
}
#endif // __BLUE_HOOK_H
