#include <vector>
#include "blue/address.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

// ==================== 测试辅助函数 ====================
void test_lookup(const std::string& host, const std::string& description) {
    BLUE_LOG_INFO(g_logger) << "========== " << description << " ==========";
    BLUE_LOG_INFO(g_logger) << "Input: " << host;
    
    std::vector<blue::Address::AddressPtr> addrs;
    bool val = blue::Address::Lookup(addrs, host);
    
    if (!val) {
        BLUE_LOG_ERROR(g_logger) << "Lookup FAILED for: " << host;
        return;
    }
    
    BLUE_LOG_INFO(g_logger) << "Resolved " << addrs.size() << " address(es):";
    for (size_t i = 0; i < addrs.size(); i++) {
        auto& ptr = addrs[i];
        BLUE_LOG_INFO(g_logger) << "  [" << i << "] " << ptr->toString();
    }
    BLUE_LOG_INFO(g_logger) << "";  // 空行分隔
}

// ==================== DNS 域名解析 ====================
void test_dns_resolution() {
    // 1. 常规域名
    test_lookup("www.baidu.com", "常规域名");
    
    // 2. 多级域名
    test_lookup("api.github.com", "多级域名");
    
    // 3. 根域名
    test_lookup("example.com", "根域名");
    
    // 4. 带端口的域名
    test_lookup("www.baidu.com:80", "域名+端口80");
    test_lookup("www.baidu.com:443", "域名+端口443");
    test_lookup("www.baidu.com:8080", "域名+端口8080");
    
    // 5. 完整 URL 格式
    test_lookup("http://www.baidu.com", "完整HTTP URL");
    test_lookup("https://www.baidu.com", "完整HTTPS URL");
    test_lookup("http://www.baidu.com:8080/index.html", "完整URL+路径");
    
    // 6. 不带协议前缀
    test_lookup("www.baidu.com:8080/api/test", "域名+端口+路径(无协议)");
}

// ==================== IPv4 地址 ====================
void test_ipv4_address() {
    // 1. 纯 IPv4
    test_lookup("192.168.1.1", "纯IPv4地址");
    test_lookup("127.0.0.1", "本地回环IPv4");
    test_lookup("0.0.0.0", "全零IPv4");
    test_lookup("255.255.255.255", "广播地址");
    
    // 2. IPv4 + 端口
    test_lookup("192.168.1.1:8080", "IPv4+端口");
    test_lookup("127.0.0.1:3000", "回环地址+端口");
    
    // 3. IPv4 完整URL
    test_lookup("http://192.168.1.1:8080/index.html", "IPv4完整URL");
    test_lookup("https://127.0.0.1:443/api", "IPv4 HTTPS URL");
    
    // 4. 无端口 IPv4 URL
    test_lookup("http://10.0.0.5", "IPv4 HTTP无端口");
}

// ==================== IPv6 地址 ====================
void test_ipv6_address() {
    // 1. 纯 IPv6（各种压缩格式）
    test_lookup("::1", "IPv6回环地址");
    test_lookup("2001:db8::1", "IPv6文档地址");
    test_lookup("fe80::1234:5678", "IPv6链路本地");
    test_lookup("2001:0db8:0000:0000:0000:ff00:0042:8329", "IPv6完整格式");
    
    // 2. IPv6 + 端口
    test_lookup("[::1]:8080", "IPv6+端口");
    test_lookup("[2001:db8::1]:443", "IPv6+端口443");
    test_lookup("[fe80::1%eth0]:3000", "IPv6作用域+端口");
    
    // 3. IPv6 完整URL
    test_lookup("http://[::1]:8080/index.html", "IPv6 HTTP URL");
    test_lookup("https://[2001:db8::1]:443/api", "IPv6 HTTPS URL");
    test_lookup("http://[fe80::1]:3000/path/to/resource", "IPv6链路本地URL");
    
    // 4. IPv6 无端口（默认端口）
    test_lookup("[::1]", "IPv6无端口");
    test_lookup("http://[::1]/index.html", "IPv6 URL无端口");
    test_lookup("https://[2001:db8::1]", "IPv6 HTTPS无端口");
}

// ==================== 特殊场景 ====================
void test_special_cases() {
    // 1. 空字符串
    test_lookup("", "空字符串");
    
    // 2. NULL 或空（获取本机地址）
    test_lookup("0.0.0.0:0", "获取本机所有地址");
    
    // 3. localhost
    test_lookup("localhost", "localhost域名");
    test_lookup("localhost:8080", "localhost+端口");
    
    // 4. 只有端口
    test_lookup(":8080", "仅端口号");
    
    // 5. 特殊域名
    test_lookup("localhost.localdomain", "完整本地域名");
    
    // 6. 不存在的域名（测试错误处理）
    test_lookup("this-domain-does-not-exist-12345.com", "不存在的域名");
    test_lookup("invalid...domain", "格式错误域名");
}

// ==================== 边界值测试 ====================
void test_boundary_cases() {
    // 1. 极限端口号
    test_lookup("www.baidu.com:1", "最小端口1");
    test_lookup("www.baidu.com:65535", "最大端口65535");
    test_lookup("www.baidu.com:0", "端口0");
    test_lookup("www.baidu.com:65536", "越界端口65536");
    test_lookup("www.baidu.com:99999", "越界端口99999");
    
    // 2. 极限长度域名
    test_lookup("a.very.long.domain.name.with.many.subdomains.example.com", "长域名");
    
    // 3. 超长标签
    std::string long_label(64, 'a');
    test_lookup(long_label + ".com", "超63字符标签");
    
    // 4. 带参数查询字符串
    test_lookup("www.baidu.com:8080/search?q=test&page=1", "URL带查询参数");
    test_lookup("http://www.baidu.com:8080/api/data?id=123#section", "URL带Fragment");
    
    // 5. 特殊字符
    test_lookup("www.baidu.com:8080/path with spaces", "路径含空格");
    test_lookup("www.baidu.com:8080/%E4%B8%AD%E6%96%87", "URL编码");
}

// ==================== IPv4/IPv6 混合 ====================
void test_mixed_scenarios() {
    // 1. 相同域名对比
    test_lookup("www.baidu.com", "百度域名IPv4/IPv6混合");
    
    // 2. 双栈域名
    test_lookup("ipv6-test.com", "双栈测试域名");
    test_lookup("google.com", "Google(支持双栈)");
    
    // 3. 纯 IPv6 域名
    test_lookup("ipv6.baidu.com", "百度IPv6");
}

// ==================== 协议端口映射 ====================
void test_protocol_ports() {
    // HTTP 常用端口
    test_lookup("www.example.com:80", "HTTP标准端口");
    test_lookup("www.example.com:8080", "HTTP备用端口");
    
    // HTTPS
    test_lookup("www.example.com:443", "HTTPS标准端口");
    test_lookup("www.example.com:8443", "HTTPS备用端口");
    
    // 其他服务
    test_lookup("localhost:22", "SSH端口");
    test_lookup("localhost:3306", "MySQL端口");
    test_lookup("localhost:6379", "Redis端口");
    test_lookup("localhost:27017", "MongoDB端口");
}

// ==================== 主测试函数 ====================
void run_all_lookup_tests() {
    BLUE_LOG_INFO(g_logger) << "\n\n";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║   Address::Lookup 完整测试套件       ║";
    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════════╝";
    BLUE_LOG_INFO(g_logger) << "\n";
    
    // 按顺序执行所有测试
    test_dns_resolution();
    test_ipv4_address();
    test_ipv6_address();
    test_special_cases();
    test_boundary_cases();
    test_mixed_scenarios();
    test_protocol_ports();
    
    BLUE_LOG_INFO(g_logger) << "════════════ 测试完成 ════════════";
}

// ==================== 选择性测试 ====================
void run_quick_test() {
    // 快速验证核心功能
    test_lookup("www.baidu.com", "快速测试-百度");
    test_lookup("127.0.0.1:8080", "快速测试-本地IPv4");
    test_lookup("[::1]:8080", "快速测试-本地IPv6");
    test_lookup("localhost:3000", "快速测试-localhost");
}

void run_debug_test(const std::string& specific_host) {
    // 调试特定地址
    test_lookup(specific_host, "DEBUG: " + specific_host);
}

int main() {
    // 运行完整测试
    run_all_lookup_tests();
    
    // 或者快速测试
    // run_quick_test();
    
    // 或者调试单个地址
    // run_debug_test("http://[::1]:8080/test");
    
    return 0;
}