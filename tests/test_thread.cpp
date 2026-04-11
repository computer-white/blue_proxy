#include "blue/blue.h"
blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
int count = 0;
// blue::MRWmutex s_mutex;
blue::Mmutex s_mutex;

void fun1()
{
    BLUE_LOG_INFO(g_logger) << " name : " << blue::Mthread::GetName()
                            << " this->name : " << blue::Mthread::GetThis()->getName()
                            << " id : " << blue::GetThreadId()
                            << " this->id : " << blue::Mthread::GetThis()->getID();
    for (size_t i = 0;i<1'000'000;i++)
    {
        blue::Mmutex::lockSco lock(s_mutex);
        ++count;
    }
}

void fun2()
{
    while (true)
    {
        BLUE_LOG_INFO(g_logger) << "==========================================";
    }
}

void fun3()
{
    while (true)
    {
        BLUE_LOG_INFO(g_logger) << "******************************************";
    }
}
int main(int argc,char* argv[])
{
    BLUE_LOG_INFO(g_logger) << "mthread test begin";
    // // 加载配置文件
    // YAML::Node root = YAML::LoadFile(
    //     "/home/blue/c_projects/sylar/bin/cof/log2.yml");
    // blue::Config::LoadFromYAML(root);

    std::vector<blue::Mthread::MthreadPtr> threads;
    for (size_t i = 0;i<5;i++)
    {
        blue::Mthread::MthreadPtr new_thread1 = 
        std::make_shared<blue::Mthread>(&fun1,"name_" + std::to_string(i * 2));

        // blue::Mthread::MthreadPtr new_thread2 = 
        // std::make_shared<blue::Mthread>(&fun3,"name_" + std::to_string(i * 2 + 1));
        threads.push_back(new_thread1);
        // threads.push_back(new_thread2);
        
    }
    for (size_t i = 0;i<threads.size();i++)
    {
        threads[i]->join();
    }
    BLUE_LOG_INFO(g_logger) << "mthread test end";
    BLUE_LOG_INFO(g_logger) << count;
    return 0;
}