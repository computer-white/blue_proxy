// tests/test_http_pool_stress.cpp
#include "blue/log.h"
#include "blue/iomanager.h"
#include "http/httpconnection.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void stress_test()
{
    // 创建连接池：最大 10 个连接，存活时间 30 秒
    auto pool = std::make_shared<blue::http::HttpConnectionPool>(
        "httpbin.org", // host
        "",            // vhost
        80,            // port
        30000,         // aliveTime (ms)
        100,           // maxRequest (没用上)
        10             // maxSize
    );

    auto success = std::make_shared<std::atomic<int>>(0);
    auto timeout = std::make_shared<std::atomic<int>>(0);
    auto error = std::make_shared<std::atomic<int>>(0);
    auto total = std::make_shared<std::atomic<int>>(0);

    // 高频率定时任务：每 200ms 发起 3 个并发请求
    blue::IOManager::GetThis()->addTimer(200, [pool, success, timeout, error, total]()
                                         {
        for (int i = 0; i < 3; i++) {
            blue::IOManager::GetThis()->schedule([pool, success, timeout, error, total, i]() {
                total->fetch_add(1);
                int idx = total->load();

                const char* paths[] = {
                    "/gzip",
                    "/stream-bytes/512?chunk_size=64",
                    "/get",
                    "/delay/1"
                };
                const char* path = paths[idx % 4];

                auto result = pool->doGet(path, 3000);
                if (result->result == (int)blue::http::HttpResult::ResultStatus::OK) {
                    success->fetch_add(1);
                    BLUE_LOG_INFO(g_logger) << "#" << idx << " OK " << path
                                            << " body_size=" << result->response->getBody().size();
                } else if (result->result == (int)blue::http::HttpResult::ResultStatus::TIMEOUT) {
                    timeout->fetch_add(1);
                    BLUE_LOG_WARN(g_logger) << "#" << idx << " TIMEOUT " << path;
                } else {
                    error->fetch_add(1);
                    BLUE_LOG_ERROR(g_logger) << "#" << idx << " ERROR " << path
                                             << " code=" << result->result
                                             << " msg=" << result->error;
                }
            });
        } }, true); // 每 200ms，循环执行

    // 每 5 秒打印一次统计
    blue::IOManager::GetThis()->addTimer(5000, [success, timeout, error, total]()
                                         {
        int s = success->load();
        int t = timeout->load();
        int e = error->load();
        int tot = total->load();
        BLUE_LOG_INFO(g_logger) << "========== 统计 ==========";
        BLUE_LOG_INFO(g_logger) << "  总请求: " << tot;
        BLUE_LOG_INFO(g_logger) << "  成功: " << s << " (" << (tot > 0 ? s * 100 / tot : 0) << "%)";
        BLUE_LOG_INFO(g_logger) << "  超时: " << t;
        BLUE_LOG_INFO(g_logger) << "  错误: " << e;
        BLUE_LOG_INFO(g_logger) << "==========================="; }, true); // 每 5 秒

    // 运行 60 秒后停止
    blue::IOManager::GetThis()->addTimer(60000, []()
                                         {
        BLUE_LOG_INFO(g_logger) << "60 秒压力测试结束";
        blue::IOManager::GetThis()->stop(); }, false); // 单次
}

int main(int argc, char *argv[])
{
    blue::IOManager iom(4); // 4 个工作线程
    iom.schedule(stress_test);
    return 0;
}