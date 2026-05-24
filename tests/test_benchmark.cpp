#include "blue/blue.h"
#include "http/httpconnection.h"
#include "blue/configinit.h"
#include "blue/fiber.h"
#include "blue/iomanager.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <thread>

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

// ======================== 数据结构 ========================

struct BenchResult
{
    std::string name;
    int concurrency;      // 并发 fiber 数
    int worker_threads;   // 工作线程数
    int total_requests;   // 总请求数
    int success;          // 成功数
    int failed;           // 失败数
    long long time_ms;    // 总耗时(ms)
    double qps;           // 每秒请求数
    double avg_latency_ms; // 平均延迟(ms)
    double p50_latency_ms; // P50 延迟
    double p99_latency_ms; // P99 延迟
    double throughput_mbps; // 吞吐量(MB/s)
    double error_rate;     // 错误率(%)
};

// ======================== 延迟百分位计算 ========================

double calc_percentile(std::vector<long long> &latencies, double percentile)
{
    if (latencies.empty())
        return 0;
    std::sort(latencies.begin(), latencies.end());
    size_t idx = static_cast<size_t>(latencies.size() * percentile / 100.0);
    if (idx >= latencies.size())
        idx = latencies.size() - 1;
    return latencies[idx] / 1000.0; // us → ms
}

// ======================== benchmark 核心 ========================

void benchmark(const std::string &name, const std::string &url,
               int fiber_count, int total_requests, int worker_threads,
               std::vector<BenchResult> &results)
{
    BLUE_LOG_INFO(g_logger) << "[" << name << "] Starting...";

    // 线程安全的计数器
    std::atomic<int> success{0};
    std::atomic<int> failed{0};
    std::atomic<long long> total_bytes{0};
    std::atomic<long long> total_latency{0}; // 微秒
    std::atomic<int> done{0};

    // 采集所有延迟用于计算 P50/P99
    std::vector<long long> all_latencies;
    all_latencies.reserve(total_requests);
    std::mutex latency_mutex;

    // 按状态码分类统计
    std::unordered_map<int, std::atomic<int>> status_count;
    std::mutex status_mutex;

    auto start = std::chrono::steady_clock::now();

    // 创建 IOManager，使用固定的工作线程数
    blue::IOManager iom(worker_threads, true, "bench");

    // 提交所有任务
    for (int i = 0; i < total_requests; i++)
    {
        iom.schedule([&, url, i]()
                     {
            auto req_start = std::chrono::steady_clock::now();
            auto result = blue::http::HttpConnection::DoGet(url, 30000); // 30s 超时
            auto req_end = std::chrono::steady_clock::now();

            long long latency_us = std::chrono::duration_cast<std::chrono::microseconds>(req_end - req_start).count();

            if (result->result == 0 && result->response) {
                success.fetch_add(1);
                total_bytes.fetch_add(result->response->getBody().size());
                total_latency.fetch_add(latency_us);

                // 记录延迟用于百分位计算
                {
                    std::lock_guard<std::mutex> lock(latency_mutex);
                    all_latencies.push_back(latency_us);
                }

                // 统计 HTTP 状态码
                int status_code = (int)result->response->getStatus();
                {
                    std::lock_guard<std::mutex> lock(status_mutex);
                    // status_count[status_code] 是 atomic，不需要锁，但 map 插入需要
                    auto it = status_count.find(status_code);
                    if (it == status_count.end()) {
                        status_count[status_code].store(1);
                    } else {
                        it->second.fetch_add(1);
                    }
                }
            } else {
                failed.fetch_add(1);
                int error_code = result->result;
                {
                    std::lock_guard<std::mutex> lock(status_mutex);
                    auto it = status_count.find(error_code);
                    if (it == status_count.end()) {
                        status_count[error_code].store(1);
                    } else {
                        it->second.fetch_add(1);
                    }
                }
            }
            done.fetch_add(1); });
    }

    iom.start();

    // 等待所有请求完成（带超时保护）
    int wait_loops = 0;
    int max_wait_loops = 600; // 最多等 60 秒
    while (done.load() < total_requests && wait_loops < max_wait_loops)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        wait_loops++;
    }

    auto end = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 统计超时未完成
    int unfinished = total_requests - done.load();
    if (unfinished > 0)
    {
        failed.fetch_add(unfinished);
        BLUE_LOG_WARN(g_logger) << "[" << name << "] " << unfinished << " requests timed out";
    }

    iom.stop();

    // ======================== 计算结果 ========================

    BenchResult r;
    r.name = name;
    r.concurrency = fiber_count;
    r.worker_threads = worker_threads;
    r.total_requests = total_requests;
    r.success = success.load();
    r.failed = failed.load() + unfinished;
    r.time_ms = total_ms;
    r.qps = (total_ms > 0) ? (r.success * 1000.0 / total_ms) : 0;
    r.avg_latency_ms = (r.success > 0) ? (total_latency.load() / r.success / 1000.0) : 0;
    r.p50_latency_ms = calc_percentile(all_latencies, 50.0);
    r.p99_latency_ms = calc_percentile(all_latencies, 99.0);
    r.throughput_mbps = (total_ms > 0) ? (total_bytes.load() / (total_ms / 1000.0) / 1024 / 1024) : 0;
    r.error_rate = (r.total_requests > 0) ? (r.failed * 100.0 / r.total_requests) : 0;

    results.push_back(r);

    // ======================== 单轮结果打印 ========================

    std::stringstream ss;
    ss << "\n";
    ss << "┌──── " << name << " ────┐\n";
    ss << "│ Total Requests:  " << std::setw(8) << total_requests << "              │\n";
    ss << "│ Success:         " << std::setw(8) << r.success << " (" << std::fixed << std::setprecision(1) << (100.0 - r.error_rate) << "%)  │\n";
    ss << "│ Failed:          " << std::setw(8) << r.failed << " (" << std::fixed << std::setprecision(1) << r.error_rate << "%)  │\n";
    ss << "│ Duration:        " << std::setw(8) << total_ms << " ms           │\n";
    ss << "│ QPS:             " << std::setw(8) << std::fixed << std::setprecision(1) << r.qps << " req/s     │\n";
    ss << "│ Avg Latency:     " << std::setw(8) << std::fixed << std::setprecision(1) << r.avg_latency_ms << " ms        │\n";
    ss << "│ P50 Latency:     " << std::setw(8) << std::fixed << std::setprecision(1) << r.p50_latency_ms << " ms        │\n";
    ss << "│ P99 Latency:     " << std::setw(8) << std::fixed << std::setprecision(1) << r.p99_latency_ms << " ms        │\n";
    ss << "│ Throughput:      " << std::setw(8) << std::fixed << std::setprecision(2) << r.throughput_mbps << " MB/s     │\n";
    ss << "│ Worker Threads:  " << std::setw(8) << worker_threads << "              │\n";
    ss << "└──────────────────────────────────┘\n";
    BLUE_LOG_INFO(g_logger) << ss.str();
}

// ======================== 导出报告 ========================

void export_report(const std::vector<BenchResult> &results, const std::string &filename)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
    {
        BLUE_LOG_ERROR(g_logger) << "Failed to export report to " << filename;
        return;
    }

    // CSV 格式，方便导入 Excel
    ofs << "Name,Concurrency,WorkerThreads,Total,Success,Failed,ErrorRate(%),"
        << "Duration(ms),QPS,AvgLatency(ms),P50Latency(ms),P99Latency(ms),Throughput(MB/s)\n";

    for (auto &r : results)
    {
        ofs << r.name << ","
            << r.concurrency << ","
            << r.worker_threads << ","
            << r.total_requests << ","
            << r.success << ","
            << r.failed << ","
            << std::fixed << std::setprecision(1) << r.error_rate << ","
            << r.time_ms << ","
            << std::fixed << std::setprecision(1) << r.qps << ","
            << r.avg_latency_ms << ","
            << r.p50_latency_ms << ","
            << r.p99_latency_ms << ","
            << std::fixed << std::setprecision(2) << r.throughput_mbps << "\n";
    }

    ofs.close();
    BLUE_LOG_INFO(g_logger) << "Report exported to " << filename;
}

// ======================== main ========================

int main()
{
    blue::http::IniteConfig();

    // 获取 CPU 核心数，用于设置合理的线程数
    unsigned int cpu_cores = std::thread::hardware_concurrency();
    int worker_threads = std::min(cpu_cores * 2, 16u); // 工作线程数 = CPU 核心数 × 2，上限 16
    BLUE_LOG_INFO(g_logger) << "CPU Cores: " << cpu_cores << ", Worker Threads: " << worker_threads;

    std::vector<BenchResult> results;

    BLUE_LOG_INFO(g_logger) << "";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║              BLUE HTTP CLIENT STRESS TEST                       ║";
    BLUE_LOG_INFO(g_logger) << "║              Target: www.baidu.com                              ║";
    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════════════════════════════════════╝";

    // ======================== HTTP 测试 ========================
    // 格式: benchmark(名称, URL, 并发fiber数, 总请求数, 工作线程数, results)

    BLUE_LOG_INFO(g_logger) << "\n========== HTTP Benchmark ==========";
    benchmark("HTTP-100", "http://www.baidu.com", 100, 1000, worker_threads, results);
    benchmark("HTTP-200", "http://www.baidu.com", 200, 2000, worker_threads, results);
    benchmark("HTTP-500", "http://www.baidu.com", 500, 5000, worker_threads, results);
    benchmark("HTTP-1000", "http://www.baidu.com", 1000, 10000, worker_threads, results);

    // ======================== HTTPS 测试 ========================
    BLUE_LOG_INFO(g_logger) << "\n========== HTTPS Benchmark ==========";
    benchmark("HTTPS-50", "https://www.baidu.com", 50, 200, worker_threads, results);
    benchmark("HTTPS-100", "https://www.baidu.com", 100, 500, worker_threads, results);
    benchmark("HTTPS-200", "https://www.baidu.com", 200, 1000, worker_threads, results);

    // ======================== 汇总表 ========================
    BLUE_LOG_INFO(g_logger) << "";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║                                          STRESS TEST SUMMARY                                               ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════╣";
    BLUE_LOG_INFO(g_logger) << "║ " << std::left << std::setw(14) << "Name"
                            << std::right << std::setw(7) << "Fiber"
                            << std::setw(7) << "Thread"
                            << std::setw(7) << "Total"
                            << std::setw(8) << "Success"
                            << std::setw(7) << "Failed"
                            << std::setw(7) << "Err%"
                            << std::setw(9) << "Time(ms)"
                            << std::setw(9) << "QPS"
                            << std::setw(9) << "Avg(ms)"
                            << std::setw(9) << "P50(ms)"
                            << std::setw(9) << "P99(ms)"
                            << std::setw(10) << "TP(MB/s)"
                            << " ║";
    BLUE_LOG_INFO(g_logger) << "╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════╣";

    for (auto &r : results)
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "║ %-14s %6d %6d %7d %8d %7d %6.1f%% %8lld %8.1f %8.1f %8.1f %8.1f %9.2f ║",
                 r.name.c_str(),
                 r.concurrency,
                 r.worker_threads,
                 r.total_requests,
                 r.success,
                 r.failed,
                 r.error_rate,
                 r.time_ms,
                 r.qps,
                 r.avg_latency_ms,
                 r.p50_latency_ms,
                 r.p99_latency_ms,
                 r.throughput_mbps);
        BLUE_LOG_INFO(g_logger) << buf;
    }

    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝";

    // ======================== 性能分析 ========================
    BLUE_LOG_INFO(g_logger) << "\n========== Performance Analysis ==========";

    // 找最佳 QPS
    auto best = std::max_element(results.begin(), results.end(),
                                 [](const BenchResult &a, const BenchResult &b)
                                 { return a.qps < b.qps; });
    if (best != results.end())
    {
        BLUE_LOG_INFO(g_logger) << "🏆 Best QPS: " << best->name << " (" << std::fixed << std::setprecision(1) << best->qps << " req/s)";
    }

    // 找最低延迟
    auto lowest_latency = std::min_element(results.begin(), results.end(),
                                           [](const BenchResult &a, const BenchResult &b)
                                           { return a.avg_latency_ms < b.avg_latency_ms; });
    if (lowest_latency != results.end())
    {
        BLUE_LOG_INFO(g_logger) << "⚡ Lowest Latency: " << lowest_latency->name << " (" << std::fixed << std::setprecision(1) << lowest_latency->avg_latency_ms << " ms)";
    }

    BLUE_LOG_INFO(g_logger) << "\n=== All Tests Complete ===";

    // 导出 CSV 报告
    export_report(results, "benchmark_report.csv");

    return 0;
}