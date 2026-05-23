#include <boost/coroutine2/all.hpp>
#include <iostream>

using Coro = boost::coroutines2::coroutine<int>;

// 协程函数：作为数据的"生产者"
void producer(Coro::push_type& yield) {
    // push_type 的 operator() 向消费者提供数据并让出控制权
    yield(1);
    std::cout << "produced 1" << std::endl;
    yield(2);
    std::cout << "produced 2" << std::endl;
}

int main() {
    // 主函数作为"消费者"，使用 pull_type 来拉取数据
    Coro::pull_type source(producer);
    
    // 使用 get() 获取数据，operator() 恢复协程
    while (source) { // source 在协程有效时为 true
        std::cout << "consumed " << source.get() << std::endl;
        source(); // 请求下一个数据
    }
    return 0;
}