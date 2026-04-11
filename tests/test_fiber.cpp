#include "blue/blue.h"
blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void run_in_fiber()
{
    BLUE_LOG_INFO(g_logger) << "run in fiber begin";
    blue::Fiber::GetThis()->swapOut();
    BLUE_LOG_INFO(g_logger) << "run in fiber end";
}

void test_fiber()
{
    blue::Fiber::GetThis();
    BLUE_LOG_INFO(g_logger) << "test_fiber begin";
    blue::Fiber::FiberPtr m_fiber = 
    std::make_shared<blue::Fiber>(run_in_fiber);
    m_fiber->swapIn();
    BLUE_LOG_INFO(g_logger) << "test_fiber swapIn after";
    m_fiber->swapIn();
    BLUE_LOG_INFO(g_logger) << "test_fiber end";
}

int main(int argc,char* argv[])
{
    blue::Mthread::SetThreadName("main");
    std::vector<blue::Mthread::MthreadPtr> threads;
    for (size_t i = 0;i < 100; i++)
    {
        blue::Mthread::MthreadPtr curr = 
        std::make_shared<blue::Mthread>(
            &test_fiber,
            "name_" + std::to_string(i)
        );
        threads.push_back(curr);
    }

    for (size_t i = 0;i<threads.size();i++)
    {
        threads[i]->join();
    }
    return 0;
}