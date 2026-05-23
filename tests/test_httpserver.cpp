#include "http/httpserver.h"
#include "blue/log.h"
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

void test_httpserver()
{
    int val = 1;
    auto httpserverPtr = std::make_shared<blue::http::HttpServer<int>>();
    auto address = blue::Address::LookupAny("0.0.0.0:8020");
    
    while (!httpserverPtr->bind(address))
    {
        sleep(2);
    }
    auto ds = httpserverPtr->getDispatch();

    ds->addServlet("/blue/xxx", [](blue::http::HttpRequest::HttpRequestPtr req,
                                    blue::http::HttpResponse::HttpResponsePtr resp,
                                    blue::http::HttpSession::HttpSessionPtr session) -> int32_t {
        std::string body = "<html>\r\n"
                        "<head>\r\n"
                        "<meta charset='UTF-8'>\r\n"
                        "<title>Blue Servlet</title>\r\n"
                        "<style>\r\n"
                        "body{margin:0;padding:40px;font-family:monospace;background:#f0f0f0;}\r\n"
                        "h1{color:#1677ff;}\r\n"
                        "pre{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1);overflow-x:auto;}\r\n"
                        "</style>\r\n"
                        "</head>\r\n"
                        "<body>\r\n"
                        "<h1>Hello Blue, Pinpoint Servlet</h1>\r\n"
                        "<pre>" + req->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        BLUE_LOG_INFO(g_logger) << "query : " << req->getQuery(); 
        resp->setHeader("Content-Type", "text/html; charset=utf-8");
        resp->setHeader("Content-Length", std::to_string(body.size()));
        resp->setBody(body);
        return 0;
    });

    ds->addBlurServlet("/blue/*", [](blue::http::HttpRequest::HttpRequestPtr req,
                                    blue::http::HttpResponse::HttpResponsePtr resp,
                                    blue::http::HttpSession::HttpSessionPtr session) -> int32_t {
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
                        "<pre>" + req->toString() + "</pre>\r\n"
                        "</body>\r\n"
                        "</html>\r\n";
        
        resp->setHeader("Content-Type", "text/html; charset=utf-8");
        resp->setHeader("Content-Length", std::to_string(body.size()));
        resp->setBody(body);
        return 0;
    });
    httpserverPtr->start();
    
}

int main(int argc,char* argv[])
{
    blue::IOManager iom(2);
    iom.schedule(test_httpserver);
    return 0;
}