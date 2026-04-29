#include <vector>
#include "blue/address.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void test_ipv4()
{
	// auto res = blue::IPAddress::Create("www.baidu.com");
	auto res = blue::IPAddress::Create("127.0.0.8");
    
    if (res)
    {
        BLUE_LOG_INFO(g_logger) << res->toString();
    }
    return;
}   

int main()
{
    test_ipv4();
    return 0;
}