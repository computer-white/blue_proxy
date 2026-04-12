#include "blue/blue.h"
#include <mutex>
blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
static int threads = 10;
void test_fiber() {
    static int count = 5;
    BLUE_LOG_INFO(g_logger) << "test fiber, count=" << count;
    // sleep(1);
    if (--count >= 0) {
        blue::Schedular::GetThis()->schedule(&test_fiber);
    }
}

// 测试 use_caller = true
void test_with_caller() {
    BLUE_LOG_INFO(g_logger) << "=== Test with use_caller=true ===";
    blue::Schedular sc(threads, true, "caller_test");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    BLUE_LOG_INFO(g_logger) << "=== Test with use_caller=true finished ===";
}

// 测试 use_caller = false
void test_without_caller() {
    BLUE_LOG_INFO(g_logger) << "=== Test with use_caller=false ===";
    blue::Schedular sc(threads, false, "no_caller_test");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    BLUE_LOG_INFO(g_logger) << "=== Test with use_caller=false finished ===";
}

int main() {
    BLUE_LOG_INFO(g_logger) << "main";
    test_with_caller();
    // test_without_caller();
    BLUE_LOG_INFO(g_logger) << "over";
    return 0;
}