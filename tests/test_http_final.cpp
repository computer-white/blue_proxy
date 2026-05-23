// tests/test_http_final.cpp
#include "blue/log.h"
#include "blue/iomanager.h"
#include "http/httpconnection.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void stress_test() {
    auto pool = std::make_shared<blue::http::HttpConnectionPool>(
        "www.baidu.com", "", 80, 30000, 100, 10);

    auto success = std::make_shared<std::atomic<int>>(0);
    auto timeout = std::make_shared<std::atomic<int>>(0);
    auto error   = std::make_shared<std::atomic<int>>(0);
    auto total   = std::make_shared<std::atomic<int>>(0);

    // 每 200ms 发起 3 个请求
    blue::IOManager::GetThis()->addTimer(200, [pool, success, timeout, error, total]() {
        for (int i = 0; i < 3; i++) {
            blue::IOManager::GetThis()->schedule([pool, success, timeout, error, total]() {
                total->fetch_add(1);
                int idx = total->load();

                // 百度首页路径
                const char* paths[] = {"/", "/robots.txt", "/favicon.ico"};
                const char* path = paths[idx % 3];

                auto result = pool->doGet(path, 5000);  // 百度响应慢一些，给 5 秒
                if (result->result == (int)blue::http::HttpResult::ResultStatus::OK) {
                    success->fetch_add(1);
                } else if (result->result == (int)blue::http::HttpResult::ResultStatus::TIMEOUT) {
                    timeout->fetch_add(1);
                } else {
                    error->fetch_add(1);
                    BLUE_LOG_ERROR(g_logger) 
                        << "FAIL #" << idx << " " << path
                        << " code=" << result->result
                        << " msg=" << result->error;
                }
            });
        }
    }, true);

    // 每 10 秒打印统计
    blue::IOManager::GetThis()->addTimer(10000, [success, timeout, error, total]() {
        int s = success->load();
        int t = timeout->load();
        int e = error->load();
        int tot = total->load();
        BLUE_LOG_INFO(g_logger) 
            << "统计 | 总请求:" << tot
            << " 成功:" << s 
            << "(" << (tot > 0 ? s * 100 / tot : 0) << "%)"
            << " 超时:" << t
            << " 错误:" << e;
    }, true);

    // 60 秒后退出
    blue::IOManager::GetThis()->addTimer(60000, [success, timeout, error, total]() {
        int s = success->load();
        int t = timeout->load();
        int e = error->load();
        int tot = total->load();
        BLUE_LOG_INFO(g_logger) << "========== 最终结果 ==========";
        BLUE_LOG_INFO(g_logger) << "总请求: " << tot;
        BLUE_LOG_INFO(g_logger) << "成功: " << s << " (" << (tot > 0 ? s * 100 / tot : 0) << "%)";
        BLUE_LOG_INFO(g_logger) << "超时: " << t;
        BLUE_LOG_INFO(g_logger) << "错误: " << e;
        BLUE_LOG_INFO(g_logger) << "==============================";
        _exit(0);
    }, false);
}

int main(int argc, char* argv[]) {
    blue::IOManager iom(4);
    iom.schedule(stress_test);
    return 0;
}