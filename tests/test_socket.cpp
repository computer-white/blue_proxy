#include "blue/msocket.h"
#include "blue/log.h"
#include "blue/iomanager.h"
static blue::Logger::LoggerPtr g_logger  = BLUE_LOG_MASSAGE_ROOT();

void test_socket() {
    std::shared_ptr<blue::IPAddress> ipaddr = blue::Address::LookupAnyIpAddress("www.baidu.com");
    if (!ipaddr) {
        BLUE_LOG_ERROR(g_logger) << "get address failed";
        return;
    }
    BLUE_LOG_INFO(g_logger) << "get address : " << ipaddr->toString();

    std::shared_ptr<blue::MSocket> sockfd = blue::MSocket::CreateTcpSocket();
    ipaddr->setPort(80);
    
    if (!sockfd->connect(ipaddr)) {
        BLUE_LOG_ERROR(g_logger) << "connect " << ipaddr->toString() << " failed";
        return;
    }
    BLUE_LOG_INFO(g_logger) << "connect " << ipaddr->toString() << " connected";

    // // 发送请求
    // const char* request = 
    //     "GET / HTTP/1.0\r\n"
    //     "Host: www.baidu.com\r\n"
    //     "Connection: close\r\n"
    //     "\r\n";
    
    // size_t len = strlen(request);
    // size_t sent = 0;
    // while (sent < len) {
    //     ssize_t rt = sockfd->send(request + sent, len - sent, 0);
    //     if (rt <= 0) {
    //         BLUE_LOG_ERROR(g_logger) << "send failed";
    //         return;
    //     }
    //     sent += rt;
    // }
    const char* request = 
    "GET / HTTP/1.0\r\n"
    "Host: www.baidu.com\r\n"
    "Connection: close\r\n"
    "\r\n";

    ssize_t rt = sockfd->send(request, strlen(request), 0);
    if (rt <= 0) {
        BLUE_LOG_ERROR(g_logger) << "send failed";
        return;
    }

    // // 接收响应
    // std::string response;
    // char buf[4096];
    // while (true) {
    //     ssize_t rt = sockfd->recv(buf, sizeof(buf) - 1, 0);
    //     if (rt <= 0) {
    //         break;
    //     }
    //     buf[rt] = '\0';
    //     response += buf;
        
    //     // 简单判断是否接收完
    //     if (response.find("\r\n\r\n") != std::string::npos) {
    //         break;  // 实际还应该根据 Content-Length 判断body是否接收完
    //     }
    // }
    
    char buf[4096];
    rt = sockfd->recv(buf, sizeof(buf) - 1, 0);
    if (rt > 0) {
        buf[rt] = '\0';
        BLUE_LOG_INFO(g_logger) << buf;
    }

    BLUE_LOG_INFO(g_logger) << "bufsize : " << rt;
}

int main(int argc,char* argv[])
{
    blue::IOManager iom(1);
    iom.schedule(test_socket);
    return 0;
}