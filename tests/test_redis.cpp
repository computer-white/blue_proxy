#include "blue/redismanager.h"
#include <iostream>

int main()
{
    auto redis = blue::RedisManager::Create("127.0.0.1",6379,"lld200510");
    redis->set("user:1001","blue",180);
    std::string name = redis->get("user:1001");
    std::cout << "name : " << name << std::endl;
    long long count = redis->incr("rate:127.0.0.1");
    redis->expire("rate:127.0.0.1", 60);  // 60秒窗口
    if (redis->exists("cache:xxx"))
    {
        std::cout << "cache:xxx 存在" << std::endl; 
    }
}