// tests/test_http_stress.cpp
#include "blue/log.h"
#include "blue/iomanager.h"
#include "http/httpconnection.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void stress_test() {
    // 用 shared_ptr 管理，避免悬空引用
    auto success = std::make_shared<std::atomic<int>>(0);
    auto timeout = std::make_shared<std::atomic<int>>(0);
    auto error   = std::make_shared<std::atomic<int>>(0);
    auto total   = std::make_shared<std::atomic<int>>(0);

    // 每 200ms 发起 3 个并发请求，每次独立创建连接
    blue::IOManager::GetThis()->addTimer(0, [success, timeout, error, total]() {
        for (int i = 0; i < 3; i++) {
            blue::IOManager::GetThis()->schedule([success, timeout, error, total]() {
                total->fetch_add(1);
                int idx = total->load();

                const char* paths[] = {
                    "/gzip",
                    "/stream-bytes/512?chunk_size=64",
                    "/get",
                    "/delay/1"
                };
                const char* path = paths[idx % 4];

                // 每次独立创建连接，直接调 HttpConnection::DoGet
                auto result = blue::http::HttpConnection::DoGet(
                    std::string("http://httpbin.org") + path, 3000);

                if (!result) {
                    error->fetch_add(1);
                    BLUE_LOG_ERROR(g_logger) << "#" << idx << " NULL_RESULT " << path;
                    return;
                }
                if (result->result == (int)blue::http::HttpResult::ResultStatus::OK) {
                    success->fetch_add(1);
                    size_t body_size = result->response ? result->response->getBody().size() : 0;
                    BLUE_LOG_INFO(g_logger) << "#" << idx << " OK " << path
                                            << " body_size=" << body_size;
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
        }
    }, true);  // 每 200ms，循环执行

    // 每 5 秒打印统计
    blue::IOManager::GetThis()->addTimer(5000, [success, timeout, error, total]() {
        int s = success->load();
        int t = timeout->load();
        int e = error->load();
        int tot = total->load();
        BLUE_LOG_INFO(g_logger) << "========== 统计 ==========";
        BLUE_LOG_INFO(g_logger) << "  总请求: " << tot;
        BLUE_LOG_INFO(g_logger) << "  成功: " << s << " (" << (tot > 0 ? s * 100 / tot : 0) << "%)";
        BLUE_LOG_INFO(g_logger) << "  超时: " << t;
        BLUE_LOG_INFO(g_logger) << "  错误: " << e;
        BLUE_LOG_INFO(g_logger) << "===========================";
    }, true);

    // 运行 30 秒后停止
    blue::IOManager::GetThis()->addTimer(30000, []() {
        BLUE_LOG_INFO(g_logger) << "30 秒压力测试结束";
        exit(0);
    }, false);
}

int main(int argc, char* argv[]) {
    blue::IOManager iom(4);
    iom.schedule(stress_test);
    return 0;
}