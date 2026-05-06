// tests/test_http_full.cpp
#include "http/httpParser.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "========== HTTP 解析器完整测试（请求 + 响应）==========\n" << std::endl;

    // ============================
    // 第一部分：请求解析测试
    // ============================
    std::cout << "========== 请求解析测试 ==========\n" << std::endl;

    // 1. GET 请求：完整字段测试
    std::cout << "--- 1. GET 请求（完整字段） ---" << std::endl;
    {
        blue::http::HttpRequestParser parser;
        parser.lenientMode(true);

        parser.on_MessageBegin([]() -> int {
            std::cout << "  [MessageBegin] 新请求开始" << std::endl;
            return 0;
        });

        parser.on_UrlComplate([](const std::string& url) -> int {
            std::cout << "  [UrlComplete] url: " << url << std::endl;
            return 0;
        });
        
        parser.on_MethodComplate([](const std::string& method) -> int {
            std::cout << "  [MethodComplete] method: " << method << std::endl;
            return 0;
        });

        parser.on_VersionComplate([](const std::string& version) -> int {
            std::cout << "  [VersionComplete] version: " << version << std::endl;
            return 0;
        });

        parser.on_HeaderCompalte([]() -> int {
            std::cout << "  [HeaderComplete] 头部解析完成" << std::endl;
            return 0;
        });

        bool done = false;
        parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
            done = true;
            std::cout << "  [MessageComplete]" << std::endl;
            std::cout << "    Method: " << blue::http::HttpMethodToChars(req->getMethod()) << std::endl;
            std::cout << "    Path: " << req->getPath() << std::endl;
            std::cout << "    Query: " << req->getQuery() << std::endl;
            std::cout << "    Fragment: " << req->getFragment() << std::endl;
            std::cout << "    Version: 1." << (req->getVersion() & 0x0F) << std::endl;
            std::cout << "    KeepAlive: " << (req->isKeepAlive() ? "true" : "false") << std::endl;
            std::cout << "    Host: " << req->getHeader("Host") << std::endl;
            std::cout << "    Accept: " << req->getHeader("Accept") << std::endl;
            std::cout << "    Cookie count: " << req->getCookies().size() << std::endl;
            std::cout << "    Param count: " << req->getParams().size() << std::endl;
            return 0;
        });
        parser.Init();

        const char* request =
            "GET /api/users?id=1&name=hello#section HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "Accept: application/json\r\n"
            "Cookie: token=abc123; theme=dark\r\n"
            "\r\n";

        parser.Execute(request, strlen(request));
        parser.Finalize();
        assert(done);
        std::cout << "  [通过]\n" << std::endl;
    }

    // 2. POST 请求：Body 测试
    std::cout << "--- 2. POST 请求（带 Body） ---" << std::endl;
    {
        blue::http::HttpRequestParser parser;
        parser.lenientMode(true);

        bool done = false;
        parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
            done = true;
            std::cout << "  Method: " << blue::http::HttpMethodToChars(req->getMethod()) << std::endl;
            std::cout << "  Path: " << req->getPath() << std::endl;
            std::cout << "  Body size: " << req->getBody().size() << std::endl;
            std::cout << "  Body: [" << req->getBody() << "]" << std::endl;
            assert(req->getBody() == "hello world 123");
            return 0;
        });
        parser.Init();

        std::string body = "hello world 123";
        std::string request =
            "POST /api/login HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" + body;

        parser.Execute(request.data(), request.size());
        parser.Finalize();
        assert(done);
        std::cout << "  [通过]\n" << std::endl;
    }

    // 3. HTTP/1.0 短连接
    std::cout << "--- 3. HTTP/1.0（短连接） ---" << std::endl;
    {
        blue::http::HttpRequestParser parser;
        parser.lenientMode(true);

        bool done = false;
        parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
            done = true;
            std::cout << "  KeepAlive: " << req->isKeepAlive() << " (expect 0)" << std::endl;
            assert(!req->isKeepAlive());
            return 0;
        });
        parser.Init();

        const char* request = "GET / HTTP/1.0\r\nHost: test.com\r\n\r\n";
        parser.Execute(request, strlen(request));
        parser.Finalize();
        assert(done);
        std::cout << "  [通过]\n" << std::endl;
    }

    // 4. 不注册回调不崩溃
    std::cout << "--- 4. 不注册回调 ---" << std::endl;
    {
        blue::http::HttpRequestParser parser;
        parser.lenientMode(true);
        parser.Init();

        const char* request = "GET / HTTP/1.1\r\nHost: test.com\r\n\r\n";
        parser.Execute(request, strlen(request));
        parser.Finalize();
        std::cout << "  [通过] 无回调不崩溃\n" << std::endl;
    }

    // 5. 错误处理
    std::cout << "--- 5. 错误处理 ---" << std::endl;
    {
        blue::http::HttpRequestParser parser;
        parser.lenientMode(false);
        parser.Init();

        const char* bad_request = "INVALID / HTTP/1.1\r\n\r\n";
        parser.Execute(bad_request, strlen(bad_request));
        std::cout << "  [通过] 错误日志已输出\n" << std::endl;
    }

    // 6. 长连接复用（每次新建 parser）
    std::cout << "--- 6. 长连接复用（每次新建 parser） ---" << std::endl;
    {
        int count = 0;

        // 第一个请求
        blue::http::HttpRequestParser parser1;
        parser1.lenientMode(true);
        parser1.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
            count++;
            std::cout << "  消息 #" << count << " path: " << req->getPath() << std::endl;
            return 0;
        });
        parser1.Init();
        std::string req1 = "GET /first HTTP/1.1\r\nHost: test.com\r\n\r\n";
        parser1.Execute(req1.data(), req1.size());
        parser1.Finalize();

        // 第二个请求（新建 parser）
        blue::http::HttpRequestParser parser2;
        parser2.lenientMode(true);
        parser2.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
            count++;
            std::cout << "  消息 #" << count << " path: " << req->getPath() << std::endl;
            return 0;
        });
        parser2.Init();
        std::string req2 = "GET /second HTTP/1.1\r\nHost: test.com\r\n\r\n";
        parser2.Execute(req2.data(), req2.size());
        parser2.Finalize();

        std::cout << "  Total: " << count << " (expect 2)" << std::endl;
        assert(count == 2);
        std::cout << "  [通过]\n" << std::endl;
    }

    // ============================
    // 第二部分：响应解析测试
    // ============================
    std::cout << "========== 响应解析测试 ==========\n" << std::endl;

    // 7. 200 OK 完整响应
    std::cout << "--- 7. 200 OK 完整响应 ---" << std::endl;
    {
        blue::http::HttpResponseParser parser;
        parser.lenientMode(true);

        parser.on_MessageBegin([]() -> int {
            std::cout << "  [MessageBegin] 响应开始" << std::endl;
            return 0;
        });

        parser.on_StatusComplate([](const std::string& reason) -> int {
            std::cout << "  [StatusComplete] reason: " << reason << std::endl;
            return 0;
        });

        parser.on_HeaderCompalte([]() -> int {
            std::cout << "  [HeaderComplete]" << std::endl;
            return 0;
        });

        bool done = false;
        parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpResponse> resp) -> int {
            done = true;
            std::cout << "  [MessageComplete]" << std::endl;
            std::cout << "    Status: " << static_cast<int>(resp->getStatus()) << std::endl;
            std::cout << "    Reason: " << resp->getReason() << std::endl;
            std::cout << "    Version: 1." << (resp->getVersion() & 0x0F) << std::endl;
            std::cout << "    KeepAlive: " << resp->isKeepAlive() << std::endl;
            std::cout << "    Content-Type: " << resp->getHeader("Content-Type") << std::endl;
            std::cout << "    Body: [" << resp->getBody() << "]" << std::endl;
            return 0;
        });
        parser.Init();

        std::string body = "hello world!";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: keep-alive\r\n"
            "\r\n" + body;

        parser.Execute(response.data(), response.size());
        parser.Finalize();
        assert(done);
        std::cout << "  [通过]\n" << std::endl;
    }

    // 8. 404 Not Found
    std::cout << "--- 8. 404 Not Found ---" << std::endl;
    {
        blue::http::HttpResponseParser parser;
        parser.lenientMode(true);

        bool done = false;
        parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpResponse> resp) -> int {
            done = true;
            std::cout << "  Status: " << static_cast<int>(resp->getStatus()) << std::endl;
            std::cout << "  Reason: " << resp->getReason() << std::endl;
            assert(resp->getStatus() == blue::http::HttpStatus::NOT_FOUND);
            return 0;
        });
        parser.Init();

        const char* response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        parser.Execute(response, strlen(response));
        parser.Finalize();
        assert(done);
        std::cout << "  [通过]\n" << std::endl;
    }

    // 9. 不注册回调不崩溃
    std::cout << "--- 9. 响应无回调 ---" << std::endl;
    {
        blue::http::HttpResponseParser parser;
        parser.lenientMode(true);
        parser.Init();

        const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        parser.Execute(response, strlen(response));
        parser.Finalize();
        std::cout << "  [通过] 无回调不崩溃\n" << std::endl;
    }

    std::cout << "========== 全部测试通过！==========" << std::endl;
    return 0;
}