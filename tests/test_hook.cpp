#include "blue/hook.h"
#include "blue/iomanager.h"
#include "blue/log.h"
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
void test_sleep()
{
	blue::IOManager iom(1);
	iom.schedule([](){
		sleep(2);
		BLUE_LOG_INFO(g_logger) << "sleep 2";
	});

	iom.schedule([](){
		sleep(3);
		BLUE_LOG_INFO(g_logger) << "sleep 3";
	});
	BLUE_LOG_INFO(g_logger) << "test_sleep";
}
int main(int argv,char* argc[])
{
	test_sleep();
	return 0;
}
