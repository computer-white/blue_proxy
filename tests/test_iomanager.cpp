#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "../blue/blue.h"
#include "../blue/iomanager.h"
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void test_fiber()
{
	BLUE_LOG_INFO(g_logger) << "test fiber";
	return;
}

void test1() 
{
	blue::IOManager im(3,true,"test1");
	im.schedule(&test_fiber);
	int sco = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
	if (sco == -1)
	{
		BLUE_LOG_ERROR(g_logger) << "socket error";
		return;
	}
	// fcntl(sco,F_SETFL,O_NONBLOCK);

	sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET,"110.242.70.57",&addr.sin_addr.s_addr);
	int rt = connect(sco,(const sockaddr*)&addr,sizeof(addr));
	if (rt < 0)
	{
		// 对于服务器来说这个状态处于三次握手阶段的第二次握手
		if (errno == EINPROGRESS) {
			// 连接正在建立中，这是正常情况
			BLUE_LOG_INFO(g_logger) << "connect in progress";
		} else {
			// 真正的错误
			BLUE_LOG_INFO(g_logger) << "connect error: " << strerror(errno);
			return;
		}
	}
	im.addEvent(sco,blue::IOManager::Event::WRITE,[](){
		BLUE_LOG_INFO(g_logger) << "connected";
	});
}

blue::Timer::TimerPtr task;
void test_timer()
{
	// 把主线程也去执行任务就会出错
	blue::IOManager iom(3,true,"test_timer");
	task = iom.addTimer(1000,[](){
		static int n = 0;
		BLUE_LOG_INFO(g_logger) << "hello timer, n : " << n;
		while (++n == 3) {
			// task->cancel();
			BLUE_LOG_INFO(g_logger) << "hello timer, n : " << n;
			task->reset(2000,true);
		}
	},true);
}
int main(int argc,char* argv[])
{
    test1();
	// test_timer();
	return 0;
}
