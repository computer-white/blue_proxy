#include "blue/configinit.h"
#include "blue/iomanager.h"
#include "http/httpserver.h"
#include "blue/log.h"
#include "blue/url.h"
#include "blue/dbmanager.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
void testssl()
{
    blue::SSLSocket::initSSL("/home/blue/c_projects/sylar/certs");
    auto sslhttpserverPtr = std::make_shared<blue::http::HttpServer<int>>();
    auto ssladdress = blue::Address::LookupAny("0.0.0.0:8443");
    if (!ssladdress)
    {
        BLUE_LOG_INFO(g_logger) << "ssladdress error";
        return;
    }
    while (!sslhttpserverPtr->bind(ssladdress))
    {
        sleep(2);
    }
    auto ds = sslhttpserverPtr->getDispatch();
    // 添加模糊匹配路由
    ds->addBlurServlet("/*", [](blue::http::HttpRequest::HttpRequestPtr request, blue::http::HttpResponse::HttpResponsePtr response, blue::http::HttpSession::HttpSessionPtr session) -> int32_t
                       {
            std::string body = "<html>\r\n"
                        "<head>\r\n"
                        "<meta charset='UTF-8'>\r\n"
                        "<title>Blue Blur Servlet</title>\r\n"
                        "<style>\r\n"
                        "body{margin:0;padding:40px;font-family:monospace;background:#f5f5f5;}\r\n"
                        "h1{color:#1677ff;}\r\n"
                        "pre{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);overflow-x:auto;}\r\n"
                        "</style>\r\n"
                        "</head>\r\n"
                        "<body>\r\n"
                        "<h1>Hello Blue, Blur Servlet</h1>\r\n"
                        "<pre>" + request->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        
            response->setHeader("Content-Type", "text/html; charset=utf-8");
            response->setHeader("Content-Length", std::to_string(body.size()));
            response->setBody(body);
            return 0; });
    sslhttpserverPtr->start();
}
void test()
{
    auto httpserverPtr = std::make_shared<blue::http::HttpServer<int>>();
    auto address = blue::Address::LookupAny("0.0.0.0:8020");
    if (!address)
    {
        BLUE_LOG_INFO(g_logger) << "address error";
        return;
    }
    // bind地址并listen
    while (!httpserverPtr->bind(address))
    {
        sleep(2);
    }
    // 获取路由指针
    auto ds = httpserverPtr->getDispatch();
    // 添加模糊匹配路由
    ds->addBlurServlet("/*", [](blue::http::HttpRequest::HttpRequestPtr request, blue::http::HttpResponse::HttpResponsePtr response, blue::http::HttpSession::HttpSessionPtr session) -> int32_t
                       {
            std::string body = "<html>\r\n"
                        "<head>\r\n"
                        "<meta charset='UTF-8'>\r\n"
                        "<title>Blue Blur Servlet</title>\r\n"
                        "<style>\r\n"
                        "body{margin:0;padding:40px;font-family:monospace;background:#f5f5f5;}\r\n"
                        "h1{color:#1677ff;}\r\n"
                        "pre{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);overflow-x:auto;}\r\n"
                        "</style>\r\n"
                        "</head>\r\n"
                        "<body>\r\n"
                        "<h1>Hello Blue, Blur Servlet</h1>\r\n"
                        "<pre>" + request->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        
            response->setHeader("Content-Type", "text/html; charset=utf-8");
            response->setHeader("Content-Length", std::to_string(body.size()));
            response->setBody(body);
            return 0; });

    // 开始
    httpserverPtr->start();
}

int main(int argc, char *argv[])
{
    blue::http::IniteConfig();
    blue::IOManager iom(32,true,"blueproxy");
    iom.schedule(test);
    // iom.schedule(testssl);
    return 0;
}