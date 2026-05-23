#include "blue/blue.h"
#include "http/httpconnection.h"
#include "blue/configinit.h"
#include "blue/fiber.h"
#include "blue/iomanager.h"
#include <atomic>
#include <chrono>
#include <vector>

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

struct BenchResult
{
    std::string name;
    int concurrency;
    int total;
    int success;
    int failed;
    long long time_ms;
    double qps;
    double avg_latency;
    double throughput;
};

void benchmark(const std::string &name, const std::string &url,
               int concurrency, int total, std::vector<BenchResult> &results)
{

    std::atomic<int> success{0};
    std::atomic<int> failed{0};
    std::atomic<long long> total_bytes{0};
    std::atomic<long long> total_latency{0};
    std::atomic<int> done{0};

    auto start = std::chrono::steady_clock::now();

    blue::IOManager iom(concurrency, true, "xxx");

    for (int i = 0; i < total; i++)
    {
        iom.schedule([&, url, i]()
                     {
            auto req_start = std::chrono::steady_clock::now();
            auto result = blue::http::HttpConnection::DoGet(url, 15000);
            auto req_end = std::chrono::steady_clock::now();

            if (result->result == 0 && result->response) {
                success.fetch_add(1);
                total_bytes.fetch_add(result->response->getBody().size());
                total_latency.fetch_add(
                    std::chrono::duration_cast<std::chrono::microseconds>(req_end - req_start).count());
            } else {
                failed.fetch_add(1);
            }
            done.fetch_add(1); });
    }

    iom.start();
    iom.stop();
    auto end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    BenchResult r;
    r.name = name;
    r.concurrency = concurrency;
    r.total = total;
    r.success = success.load();
    r.failed = failed.load();
    r.time_ms = total_ms;
    r.qps = success.load() * 1000.0 / std::max(1L, total_ms);
    r.avg_latency = total_latency.load() / std::max(1, success.load()) / 1000.0;
    r.throughput = total_bytes.load() / (std::max(1L, total_ms) / 1000.0) / 1024 / 1024;

    results.push_back(r);
}

int main()
{
    blue::http::IniteConfig();

    std::vector<BenchResult> results;

    BLUE_LOG_INFO(g_logger) << "=== Blue Proxy Stress Test ===";

    // HTTP
    benchmark("HTTP 100并发", "http://www.baidu.com", 100, 1000, results);
    benchmark("HTTP 200并发", "http://www.baidu.com", 200, 2000, results);
    benchmark("HTTP 500并发", "http://www.baidu.com", 500, 5000, results);
    benchmark("HTTP 1000并发", "http://www.baidu.com", 1000, 10000, results);

    // HTTPS
    benchmark("HTTPS 50并发", "https://www.baidu.com", 50, 200, results);
    benchmark("HTTPS 100并发", "https://www.baidu.com", 100, 500, results);
    benchmark("HTTPS 200并发", "https://www.baidu.com", 200, 1000, results);

    // ===== 汇总 =====
    BLUE_LOG_INFO(g_logger) << "";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║                    STRESS TEST SUMMARY                       ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════════════════════════════════════════╣";
    BLUE_LOG_INFO(g_logger) << "║ Name           Concur  Total  Success  Failed   Time     QPS    AvgLat  Thrpt  ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════════════════════════════════════════╣";

    for (auto &r : results)
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "║ %-14s %4d   %5d  %5d   %4d   %5lldms  %6.1f  %5.1fms  %5.2f ║",
                 r.name.c_str(), r.concurrency, r.total, r.success, r.failed,
                 r.time_ms, r.qps, r.avg_latency, r.throughput);
        BLUE_LOG_INFO(g_logger) << buf;
    }

    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════════════════════════════════╝";
    BLUE_LOG_INFO(g_logger) << "=== All Tests Complete ===";

    return 0;
}