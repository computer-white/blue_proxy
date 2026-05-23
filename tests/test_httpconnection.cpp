#include "blue/log.h"
#include "blue/iomanager.h"
#include "http/httpconnection.h"
#include <map>
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void test_pool()
{
    auto poolPtr = std::make_shared<blue::http::HttpConnectionPool>(
        "httpbin.org","",80,1000 * 30,5,10);
    
    blue::IOManager::GetThis()->addTimer(1000,[pool = poolPtr](){
        auto res = pool->doGet("/",3000);
        BLUE_LOG_INFO(g_logger) << "res : " << res->toString();
    },true);
}

void test()
{
    // 调用httpbin.org测试对响应的gzip解压
    blue::Address::AddressPtr address = blue::Address::LookupAnyIpAddress("httpbin.org");
    if (!address)
    {
        BLUE_LOG_ERROR(g_logger) << "get address error, error : " << errno << " strerror : " << strerror(errno);
        return;
    }
    blue::MSocket::MSocketPtr sock = blue::MSocket::CreateTcp(address);
    bool ret = sock->connect(address);
    if (!ret)
    {
        BLUE_LOG_ERROR(g_logger) << "connect error, address : " << address->toString();
        return;
    }
    auto stream = std::make_shared<blue::SocketStream>(sock);
    auto httpconnection = std::make_shared<blue::http::HttpConnection>(stream);
    auto request = std::make_shared<blue::http::HttpRequest>();
    request->setMethod(blue::http::HttpMethod::GET);
    request->setPath("/gzip");   // 测试gzip ok
    // request->setPath("/stream-bytes/256?chunk_size=32");  // 测试chunk,返回256字节,每块32字节 ok
    // request->setPath("/stream/10");    // 测试chunk ok
    // request->setPath("/drip?numbytes=128&duration=5&delay=1");  // 测试流式处理 ok
    request->setHeader("Host", "httpbin.org");
    httpconnection->sendRequest(request);
    auto [status,response] = httpconnection->recvResponse();
    if (status != blue::http::HttpConnection::RecvStatus::OK)
    {
        BLUE_LOG_ERROR(g_logger) << "response error, request : \n" << request->toString();
        return;
    }
    BLUE_LOG_INFO(g_logger) << "response : \n" << response->toString();
    BLUE_LOG_INFO(g_logger) << "response body size : " << response->getBody().size();

    BLUE_LOG_INFO(g_logger) << "===============================================";
    auto retGet =  blue::http::HttpConnection::DoGet("http://www.baidu.com",300);
    BLUE_LOG_INFO(g_logger) << "result : " << retGet->result
                            << " error : " << retGet->error
                            << " response : " << retGet->response->toString();
    
    BLUE_LOG_INFO(g_logger) << "===============================================";
    test_pool();
}

int main(int argc, char* argv[])
{
    blue::IOManager iom(2);
    iom.schedule(test);
    return 0;
}