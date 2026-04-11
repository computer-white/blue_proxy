#include "blue/blue.h"
#include <assert.h>
blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
void test_assert()
{
    // BLUE_LOG_INFO(g_logger) << blue::BacktraceToString(10);

    BLUE_ASSERT2(0 == 1,"error");
}
int main(int argc,char* argv[])
{
    test_assert();
    return 0;
}