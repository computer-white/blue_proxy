#include <vector>
#include <unordered_map>
#include "blue/address.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

// ==================== 测试辅助函数 ====================
void test_get_interface_all(const std::string& description) {
    BLUE_LOG_INFO(g_logger) << "========== " << description << " ==========";
    std::unordered_map<std::string,int> maps;
    std::multimap<std::string, std::pair<std::shared_ptr<blue::Address>, uint32_t>> results;
    bool val = blue::Address::GetInterfaceAddress(results, AF_UNSPEC);
    
    if (!val) {
        BLUE_LOG_ERROR(g_logger) << "GetInterfaceAddress FAILED";
        return;
    }
    
    BLUE_LOG_INFO(g_logger) << "Found " << results.size() << " interface address(es):";
    for (auto& entry : results) {
        BLUE_LOG_INFO(g_logger) << "  [" << entry.first << "] " 
                                << entry.second.first->toString(false) 
                                << "/" << entry.second.second;
    }
    BLUE_LOG_INFO(g_logger) << "";
}

void test_get_interface_by_name(const std::string& interface, int family, const std::string& description) {
    BLUE_LOG_INFO(g_logger) << "========== " << description << " ==========";
    BLUE_LOG_INFO(g_logger) << "Interface: " << interface << ", Family: " << family;
    
    std::vector<std::pair<std::shared_ptr<blue::Address>, uint32_t>> results;
    std::string iface = interface;
    bool val = blue::Address::GetInterfaceAddress(results, iface, family);
    
    if (!val) {
        BLUE_LOG_ERROR(g_logger) << "GetInterfaceAddress FAILED for: " << interface;
        return;
    }
    
    if (results.empty()) {
        BLUE_LOG_INFO(g_logger) << "No matching interface found";
    } else {
        BLUE_LOG_INFO(g_logger) << "Found " << results.size() << " address(es):";
        for (size_t i = 0; i < results.size(); i++) {
            BLUE_LOG_INFO(g_logger) << "  [" << i << "] " 
                                    << results[i].first->toString(false) 
                                    << "/" << results[i].second;
        }
    }
    BLUE_LOG_INFO(g_logger) << "";
}

// ==================== 测试 1：获取所有接口 ====================
void test_get_all_interfaces() {
    BLUE_LOG_INFO(g_logger) << "=== 获取所有接口（AF_UNSPEC）===";
    test_get_interface_all("获取所有接口(IPv4+IPv6)");
}

// ==================== 测试 2：按地址族过滤 ====================
void test_get_interfaces_by_family() {
    BLUE_LOG_INFO(g_logger) << "=== 按地址族过滤 ===";
    
    // 只获取 IPv4
    test_get_interface_by_name("*", AF_INET, "所有IPv4接口");
    
    // 只获取 IPv6
    test_get_interface_by_name("*", AF_INET6, "所有IPv6接口");
}

// ==================== 测试 3：按接口名获取 ====================
void test_get_interface_by_specific_name() {
    BLUE_LOG_INFO(g_logger) << "=== 按接口名获取 ===";
    
    // 常见接口名测试
    test_get_interface_by_name("lo", AF_UNSPEC, "回环接口(lo)");
    test_get_interface_by_name("eth0", AF_UNSPEC, "以太网接口(eth0)");
    test_get_interface_by_name("ens33", AF_UNSPEC, "以太网接口(ens33)");
    test_get_interface_by_name("enp0s3", AF_UNSPEC, "以太网接口(enp0s3)");
    test_get_interface_by_name("wlan0", AF_UNSPEC, "无线接口(wlan0)");
    test_get_interface_by_name("wlx00", AF_UNSPEC, "无线接口(wlx00)");
    test_get_interface_by_name("docker0", AF_UNSPEC, "Docker桥接(docker0)");
    test_get_interface_by_name("tun0", AF_UNSPEC, "VPN隧道(tun0)");
    test_get_interface_by_name("tap0", AF_UNSPEC, "TAP接口(tap0)");
}

// ==================== 测试 4：指定接口 + 地址族 ====================
void test_get_interface_name_and_family() {
    BLUE_LOG_INFO(g_logger) << "=== 指定接口名+地址族 ===";
    
    test_get_interface_by_name("lo", AF_INET, "lo + IPv4");
    test_get_interface_by_name("lo", AF_INET6, "lo + IPv6");
    test_get_interface_by_name("eth0", AF_INET, "eth0 + IPv4");
    test_get_interface_by_name("eth0", AF_INET6, "eth0 + IPv6");
}

// ==================== 测试 5：通配符 * 和空字符串 ====================
void test_get_interface_wildcard() {
    BLUE_LOG_INFO(g_logger) << "=== 通配符和空字符串 ===";
    
    // 空接口名：返回通配地址 0.0.0.0 和 ::
    test_get_interface_by_name("", AF_UNSPEC, "空字符串(通配)");
    test_get_interface_by_name("*", AF_UNSPEC, "*通配符");
    test_get_interface_by_name("", AF_INET, "空字符串+IPv4");
    test_get_interface_by_name("*", AF_INET6, "*+IPv6");
}

// ==================== 测试 6：错误处理 ====================
void test_get_interface_error_cases() {
    BLUE_LOG_INFO(g_logger) << "=== 错误处理测试 ===";
    
    // 不存在的接口
    test_get_interface_by_name("nonexistent_interface_xyz", AF_UNSPEC, "不存在的接口");
    test_get_interface_by_name("fake0", AF_INET, "不存在的接口(IPv4)");
    
    // 无效的 family（如果有枚举的话）
    test_get_interface_by_name("lo", 999, "无效的family");
}

// ==================== 测试 7：前缀长度验证 ====================
void test_prefix_length_validation() {
    BLUE_LOG_INFO(g_logger) << "=== 前缀长度验证 ===";
    
    std::multimap<std::string, std::pair<std::shared_ptr<blue::Address>, uint32_t>> results;
    bool val = blue::Address::GetInterfaceAddress(results, AF_UNSPEC);
    
    if (!val) {
        BLUE_LOG_ERROR(g_logger) << "GetInterfaceAddress FAILED";
        return;
    }
    
    for (auto& entry : results) {
        uint32_t prefix = entry.second.second;
        std::string addr_type;
        
        // 判断地址类型并验证前缀长度范围
        if (entry.second.first->getFamily() == AF_INET) {
            addr_type = "IPv4";
            if (prefix > 32) {
                BLUE_LOG_ERROR(g_logger) << "INVALID IPv4 prefix length: " << prefix;
            }
        } else if (entry.second.first->getFamily() == AF_INET6) {
            addr_type = "IPv6";
            if (prefix > 128) {
                BLUE_LOG_ERROR(g_logger) << "INVALID IPv6 prefix length: " << prefix;
            }
        }
        
        BLUE_LOG_INFO(g_logger) << "  [" << entry.first << "] " 
                                << addr_type << " "
                                << entry.second.first->toString(false) 
                                << "/" << prefix;
    }
}

// ==================== 测试 8：对比通配地址和实际接口地址 ====================
void test_wildcard_vs_actual() {
    BLUE_LOG_INFO(g_logger) << "=== 通配地址 vs 实际接口地址 ===";
    
    // 获取通配地址
    std::vector<std::pair<std::shared_ptr<blue::Address>, uint32_t>> wildcard_results;
    std::string wildcard = "*";
    blue::Address::GetInterfaceAddress(wildcard_results, wildcard, AF_UNSPEC);
    
    BLUE_LOG_INFO(g_logger) << "Wildcard addresses:";
    for (auto& entry : wildcard_results) {
        BLUE_LOG_INFO(g_logger) << "  " << entry.first->toString(false) 
                                << "/" << entry.second;
    }
    
    // 获取 loopback 地址对比
    std::vector<std::pair<std::shared_ptr<blue::Address>, uint32_t>> lo_results;
    std::string lo = "lo";
    blue::Address::GetInterfaceAddress(lo_results, lo, AF_UNSPEC);
    
    BLUE_LOG_INFO(g_logger) << "Loopback addresses:";
    for (auto& entry : lo_results) {
        BLUE_LOG_INFO(g_logger) << "  " << entry.first->toString(false) 
                                << "/" << entry.second;
    }
}

// ==================== 主测试函数 ====================
void run_all_interface_tests() {
    BLUE_LOG_INFO(g_logger) << "\n\n";
    BLUE_LOG_INFO(g_logger) << "╔══════════════════════════════════════╗";
    BLUE_LOG_INFO(g_logger) << "║  GetInterfaceAddress 完整测试套件   ║";
    BLUE_LOG_INFO(g_logger) << "╚══════════════════════════════════════╝";
    BLUE_LOG_INFO(g_logger) << "\n";
    
    test_get_all_interfaces();
    test_get_interfaces_by_family();
    test_get_interface_by_specific_name();
    test_get_interface_name_and_family();
    test_get_interface_wildcard();
    test_get_interface_error_cases();
    test_prefix_length_validation();
    test_wildcard_vs_actual();
    
    BLUE_LOG_INFO(g_logger) << "════════════ 测试完成 ════════════";
}

// ==================== 快速测试 ====================
void run_quick_interface_test() {
    BLUE_LOG_INFO(g_logger) << "=== 快速接口测试 ===";
    test_get_interface_all("所有接口");
    test_get_interface_by_name("lo", AF_UNSPEC, "回环接口");
    test_get_interface_by_name("*", AF_UNSPEC, "通配地址");
}

int main()
{
    // run_all_interface_tests();
    // run_quick_interface_test();
    test_get_interface_all("xxx");
}