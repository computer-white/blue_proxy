// tests/test_http_full.cpp
#include "http/httpParser.h"
#include <iostream>
#include <cassert>
int main() {
    // std::cout << "========== HTTP 解析器完整测试（请求 + 响应）==========\n" << std::endl;
    // // ============================
    // // 第一部分：请求解析测试
    // // ============================
    // std::cout << "========== 请求解析测试 ==========\n" << std::endl;

    // // 1. GET 请求：完整字段测试
    // std::cout << "--- 1. GET 请求（完整字段） ---" << std::endl;
    // {
    //     blue::http::HttpRequestParser parser;
    //     parser.lenientMode(true);

    //     parser.on_MessageBegin([]() -> int {
    //         std::cout << "  [MessageBegin] 新请求开始" << std::endl;
    //         return 0;
    //     });

    //     parser.on_UrlComplate([](const std::string& url) -> int {
    //         std::cout << "  [UrlComplete] url: " << url << std::endl;
    //         return 0;
    //     });
        
    //     parser.on_MethodComplate([](const std::string& method) -> int {
    //         std::cout << "  [MethodComplete] method: " << method << std::endl;
    //         return 0;
    //     });

    //     parser.on_VersionComplate([](const std::string& version) -> int {
    //         std::cout << "  [VersionComplete] version: " << version << std::endl;
    //         return 0;
    //     });

    //     parser.on_HeaderCompalte([]() -> int {
    //         std::cout << "  [HeaderComplete] 头部解析完成" << std::endl;
    //         return 0;
    //     });

    //     bool done = false;
    //     parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
    //         done = true;
    //         std::cout << "  [MessageComplete]" << std::endl;
    //         std::cout << "    Method: " << blue::http::HttpMethodToChars(req->getMethod()) << std::endl;
    //         std::cout << "    Path: " << req->getPath() << std::endl;
    //         std::cout << "    Query: " << req->getQuery() << std::endl;
    //         std::cout << "    Fragment: " << req->getFragment() << std::endl;
    //         std::cout << "    Version: 1." << (req->getVersion() & 0x0F) << std::endl;
    //         std::cout << "    KeepAlive: " << (req->isKeepAlive() ? "true" : "false") << std::endl;
    //         std::cout << "    Host: " << req->getHeader("Host") << std::endl;
    //         std::cout << "    Accept: " << req->getHeader("Accept") << std::endl;
    //         std::cout << "    Cookie count: " << req->getCookies().size() << std::endl;
    //         std::cout << "    Param count: " << req->getParams().size() << std::endl;
    //         return 0;
    //     });
    //     parser.Init();

    //     const char* request =
    //         "GET /api/users?id=1&name=hello#section HTTP/1.1\r\n"
    //         "Host: www.example.com\r\n"
    //         "Accept: application/json\r\n"
    //         "Cookie: token=abc123; theme=dark\r\n"
    //         "\r\n";

    //     parser.Execute(request, strlen(request));
    //     parser.Finalize();
    //     assert(done);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";

    // }

    // // 2. POST 请求：Body 测试
    // std::cout << "--- 2. POST 请求（带 Body） ---" << std::endl;
    // {
    //     blue::http::HttpRequestParser parser;
    //     parser.lenientMode(true);

    //     bool done = false;
    //     parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
    //         done = true;
    //         std::cout << "  Method: " << blue::http::HttpMethodToChars(req->getMethod()) << std::endl;
    //         std::cout << "  Path: " << req->getPath() << std::endl;
    //         std::cout << "  Body size: " << req->getBody().size() << std::endl;
    //         std::cout << "  Body: [" << req->getBody() << "]" << std::endl;
    //         assert(req->getBody() == "hello world 123");
    //         return 0;
    //     });
    //     parser.Init();

    //     std::string body = "hello world 123";
    //     std::string request =
    //         "POST /api/login HTTP/1.1\r\n"
    //         "Host: www.example.com\r\n"
    //         "Content-Type: text/plain\r\n"
    //         "Content-Length: " + std::to_string(body.size()) + "\r\n"
    //         "\r\n" + body;

    //     parser.Execute(request.data(), request.size());
    //     parser.Finalize();
    //     assert(done);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 3. HTTP/1.0 短连接
    // std::cout << "--- 3. HTTP/1.0（短连接） ---" << std::endl;
    // {
    //     blue::http::HttpRequestParser parser;
    //     parser.lenientMode(true);

    //     bool done = false;
    //     parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
    //         done = true;
    //         std::cout << "  KeepAlive: " << req->isKeepAlive() << " (expect 0)" << std::endl;
    //         assert(!req->isKeepAlive());
    //         return 0;
    //     });
    //     parser.Init();

    //     const char* request = "GET / HTTP/1.0\r\nHost: test.com\r\n\r\n";
    //     parser.Execute(request, strlen(request));
    //     parser.Finalize();
    //     assert(done);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 4. 不注册回调不崩溃
    // std::cout << "--- 4. 不注册回调 ---" << std::endl;
    // {
    //     blue::http::HttpRequestParser parser;
    //     parser.lenientMode(true);
    //     parser.Init();

    //     const char* request = "GET / HTTP/1.1\r\nHost: test.com\r\n\r\n";
    //     parser.Execute(request, strlen(request));
    //     parser.Finalize();
    //     std::cout << "  [通过] 无回调不崩溃\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 5. 错误处理
    // std::cout << "--- 5. 错误处理 ---" << std::endl;
    // {
    //     blue::http::HttpRequestParser parser;
    //     parser.lenientMode(false);
    //     parser.Init();

    //     const char* bad_request = "INVALID / HTTP/1.1\r\n\r\n";
    //     parser.Execute(bad_request, strlen(bad_request));
    //     std::cout << "  [通过] 错误日志已输出\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 6. 长连接复用（每次新建 parser）
    // std::cout << "--- 6. 长连接复用（每次新建 parser） ---" << std::endl;
    // {
    //     int count = 0;

    //     // 第一个请求
    //     blue::http::HttpRequestParser parser1;
    //     parser1.lenientMode(true);
    //     parser1.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
    //         count++;
    //         std::cout << "  消息 #" << count << " path: " << req->getPath() << std::endl;
    //         return 0;
    //     });
    //     parser1.Init();
    //     std::string req1 = "GET /first HTTP/1.1\r\nHost: test.com\r\n\r\n";
    //     parser1.Execute(req1.data(), req1.size());
    //     parser1.Finalize();

    //     // 第二个请求（新建 parser）
    //     blue::http::HttpRequestParser parser2;
    //     parser2.lenientMode(true);
    //     parser2.on_MessageComplate([&](std::shared_ptr<blue::http::HttpRequest> req) -> int {
    //         count++;
    //         std::cout << "  消息 #" << count << " path: " << req->getPath() << std::endl;
    //         return 0;
    //     });
    //     parser2.Init();
    //     std::string req2 = "GET /second HTTP/1.1\r\nHost: test.com\r\n\r\n";
    //     parser2.Execute(req2.data(), req2.size());
    //     parser2.Finalize();

    //     std::cout << "  Total: " << count << " (expect 2)" << std::endl;
    //     assert(count == 2);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser2.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // ============================
    // // 第二部分：响应解析测试
    // // ============================
    // std::cout << "========== 响应解析测试 ==========\n" << std::endl;

    // // 7. 200 OK 完整响应
    // std::cout << "--- 7. 200 OK 完整响应 ---" << std::endl;
    // {
    //     blue::http::HttpResponseParser parser;
    //     parser.lenientMode(true);

    //     parser.on_MessageBegin([]() -> int {
    //         std::cout << "  [MessageBegin] 响应开始" << std::endl;
    //         return 0;
    //     });

    //     parser.on_StatusComplate([](const std::string& reason) -> int {
    //         std::cout << "  [StatusComplete] reason: " << reason << std::endl;
    //         return 0;
    //     });

    //     parser.on_HeaderCompalte([]() -> int {
    //         std::cout << "  [HeaderComplete]" << std::endl;
    //         return 0;
    //     });

    //     bool done = false;
    //     parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpResponse> resp) -> int {
    //         done = true;
    //         std::cout << "  [MessageComplete]" << std::endl;
    //         std::cout << "    Status: " << static_cast<int>(resp->getStatus()) << std::endl;
    //         std::cout << "    Reason: " << resp->getReason() << std::endl;
    //         std::cout << "    Version: 1." << (resp->getVersion() & 0x0F) << std::endl;
    //         std::cout << "    KeepAlive: " << resp->isKeepAlive() << std::endl;
    //         std::cout << "    Content-Type: " << resp->getHeader("Content-Type") << std::endl;
    //         std::cout << "    Body: [" << resp->getBody() << "]" << std::endl;
    //         return 0;
    //     });
    //     parser.Init();

    //     std::string body = "hello world!";
    //     std::string response =
    //         "HTTP/1.1 200 OK\r\n"
    //         "Content-Type: text/plain\r\n"
    //         "Content-Length: " + std::to_string(body.size()) + "\r\n"
    //         "Connection: keep-alive\r\n"
    //         "\r\n" + body;

    //     parser.Execute(response.data(), response.size());
    //     parser.Finalize();
    //     assert(done);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 8. 404 Not Found
    // std::cout << "--- 8. 404 Not Found ---" << std::endl;
    // {
    //     blue::http::HttpResponseParser parser;
    //     parser.lenientMode(true);

    //     bool done = false;
    //     parser.on_MessageComplate([&](std::shared_ptr<blue::http::HttpResponse> resp) -> int {
    //         done = true;
    //         std::cout << "  Status: " << static_cast<int>(resp->getStatus()) << std::endl;
    //         std::cout << "  Reason: " << resp->getReason() << std::endl;
    //         assert(resp->getStatus() == blue::http::HttpStatus::NOT_FOUND);
    //         return 0;
    //     });
    //     parser.Init();

    //     const char* response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    //     parser.Execute(response, strlen(response));
    //     parser.Finalize();
    //     assert(done);
    //     std::cout << "  [通过]\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // // 9. 不注册回调不崩溃
    // std::cout << "--- 9. 响应无回调 ---" << std::endl;
    // {
    //     blue::http::HttpResponseParser parser;
    //     parser.lenientMode(true);
    //     parser.Init();

    //     const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    //     parser.Execute(response, strlen(response));
    //     parser.Finalize();
    //     std::cout << "  [通过] 无回调不崩溃\n" << std::endl;
    //     std::cout << "--------- to string start ----------\n";
    //     std::cout << parser.getData()->toString() << std::endl;
    //     std::cout << "--------- to string end ----------\n";
    // }

    // std::cout << "========== 全部测试通过！==========" << std::endl;
    #include <string>

// 完整 HTTP 响应字符串（C++）
const std::string http_response = 
"HTTP/1.0 200 OK\r\n"
"Accept-Ranges: bytes\r\n"
"Cache-Control: no-cache\r\n"
"Content-Length: 29533\r\n"
"Content-Type: text/html\r\n"
"Date: Wed, 06 May 2026 09:31:40 GMT\r\n"
"P3p: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
"P3p: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
"Pragma: no-cache\r\n"
"Server: BWS/1.1\r\n"
"Set-Cookie: BAIDUID=75DB74537198EBD3884CAC3157356E4D:FG=1; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: BIDUPSID=75DB74537198EBD3884CAC3157356E4D; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: PSTM=1778059900; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
"Set-Cookie: BAIDUID=75DB74537198EBD3A40633B03E50985F:FG=1; max-age=31536000; expires=Thu, 06-May-27 09:31:40 GMT; domain=.baidu.com; path=/; version=1; comment=bd\r\n"
"Tr_id: pr_0xc750e8e3000441a8\r\n"
"Traceid: 177805990034652920427457098364012415952\r\n"
"Vary: Accept-Encoding\r\n"
"X-Ua-Compatible: IE=Edge,chrome=1\r\n"
"X-Xss-Protection: 1;mode=block\r\n"
"\r\n"
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
"    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge,chrome=1\" />\n"
"    <meta content=\"always\" name=\"referrer\" />\n"
"    <meta\n"
"        name=\"description\"\n"
"        content=\"全球领先的中文搜索引擎、致力于让网民更便捷地获取信息，找到所求。百度超过千亿的中文网页数据库，可以瞬间找到相关的搜索结果。\"\n"
"    />\n"
"    <link rel=\"shortcut icon\" href=\"//www.baidu.com/favicon.ico\" type=\"image/x-icon\" />\n"
"    <link\n"
"        rel=\"search\"\n"
"        type=\"application/opensearchdescription+xml\"\n"
"        href=\"//www.baidu.com/content-search.xml\"\n"
"        title=\"百度搜索\"\n"
"    />\n"
"    <title>百度一下，你就知道</title>\n"
"    <style type=\"text/css\">\n"
"        body {\n"
"            margin: 0;\n"
"            padding: 0;\n"
"            text-align: center;\n"
"            background: #fff;\n"
"            height: 100%;\n"
"        }\n"
"\n"
"        html {\n"
"            overflow-y: auto;\n"
"            color: #000;\n"
"            overflow: -moz-scrollbars;\n"
"            height: 100%;\n"
"        }\n"
"\n"
"        body, input {\n"
"            font-size: 12px;\n"
"            font-family: \"PingFang SC\", Arial, \"Microsoft YaHei\", sans-serif;\n"
"        }\n"
"\n"
"        a {\n"
"            text-decoration: none;\n"
"        }\n"
"\n"
"        a:hover {\n"
"            text-decoration: underline;\n"
"        }\n"
"\n"
"        img {\n"
"            border: 0;\n"
"            -ms-interpolation-mode: bicubic;\n"
"        }\n"
"\n"
"        input {\n"
"            font-size: 100%;\n"
"            border: 0;\n"
"        }\n"
"\n"
"        body, form {\n"
"            position: relative;\n"
"            z-index: 0;\n"
"        }\n"
"\n"
"        #wrapper {\n"
"            height: 100%;\n"
"        }\n"
"\n"
"        #head .s-ps-islite {\n"
"            _padding-bottom: 370px;\n"
"        }\n"
"\n"
"        #head_wrapper.s-ps-islite {\n"
"            padding-bottom: 370px;\n"
"        }\n"
"\n"
"        #head_wrapper.s-ps-islite .s_form {\n"
"            position: relative;\n"
"            z-index: 1;\n"
"        }\n"
"\n"
"        #head_wrapper.s-ps-islite .fm {\n"
"            position: absolute;\n"
"            bottom: 0;\n"
"        }\n"
"\n"
"        #head_wrapper.s-ps-islite .s-p-top {\n"
"            position: absolute;\n"
"            bottom: 40px;\n"
"            width: 100%;\n"
"            height: 181px;\n"
"        }\n"
"\n"
"        #head_wrapper.s-ps-islite #s_lg_img {\n"
"            position: static;\n"
"            margin: 33px auto 0 auto;\n"
"            left: 50%;\n"
"        }\n"
"\n"
"        #form {\n"
"            z-index: 1;\n"
"        }\n"
"\n"
"        .s_form_wrapper {\n"
"            height: 100%;\n"
"        }\n"
"\n"
"        #lh {\n"
"            margin: 16px 0 5px;\n"
"            word-spacing: 3px;\n"
"        }\n"
"\n"
"        .c-font-normal {\n"
"            font: 13px/23px Arial, sans-serif;\n"
"        }\n"
"\n"
"        .c-color-t {\n"
"            color: #222;\n"
"        }\n"
"\n"
"        .c-btn,\n"
"        .c-btn:visited {\n"
"            color: #333 !important;\n"
"        }\n"
"\n"
"        .c-btn {\n"
"            display: inline-block;\n"
"            overflow: hidden;\n"
"            font-family: inherit;\n"
"            font-weight: 400;\n"
"            text-align: center;\n"
"            vertical-align: middle;\n"
"            outline: 0;\n"
"            border: 0;\n"
"            height: 30px;\n"
"            width: 80px;\n"
"            line-height: 30px;\n"
"            font-size: 13px;\n"
"            border-radius: 6px;\n"
"            padding: 0;\n"
"            background-color: #f5f5f6;\n"
"            *zoom: 1;\n"
"            cursor: pointer;\n"
"        }\n"
"\n"
"        .c-btn:hover {\n"
"            background-color: #315efb;\n"
"            color: #fff !important;\n"
"        }\n"
"\n"
"        a.c-btn {\n"
"            text-decoration: none;\n"
"        }\n"
"        .c-btn-mini {\n"
"            height: 24px;\n"
"            width: 48px;\n"
"            line-height: 24px;\n"
"        }\n"
"\n"
"        .c-btn-primary,\n"
"        .c-btn-primary:visited {\n"
"            color: #fff !important;\n"
"        }\n"
"\n"
"        .c-btn-primary {\n"
"            background-color: #4e6ef2;\n"
"        }\n"
"\n"
"        .c-btn-primary:hover {\n"
"            background-color: #315efb;\n"
"        }\n"
"\n"
"        a:active {\n"
"            color: #f60;\n"
"        }\n"
"\n"
"        #wrapper {\n"
"            position: relative;\n"
"            min-height: 100%;\n"
"        }\n"
"\n"
"        #head {\n"
"            padding-bottom: 100px;\n"
"            text-align: center;\n"
"            *z-index: 1;\n"
"        }\n"
"\n"
"        #wrapper {\n"
"            min-width: 1250px;\n"
"            height: 100%;\n"
"            min-height: 600px;\n"
"        }\n"
"\n"
"        #head {\n"
"            position: relative;\n"
"            padding-bottom: 0;\n"
"            height: 100%;\n"
"            min-height: 600px;\n"
"        }\n"
"\n"
"        .s_form_wrapper {\n"
"            height: 100%;\n"
"        }\n"
"\n"
"        .quickdelete-wrap {\n"
"            position: relative;\n"
"        }\n"
"\n"
"        .tools {\n"
"            position: absolute;\n"
"            right: -75px;\n"
"        }\n"
"\n"
"        .s-isindex-wrap {\n"
"            position: relative;\n"
"        }\n"
"\n"
"        #head_wrapper.head_wrapper {\n"
"            width: auto;\n"
"        }\n"
"\n"
"        #head_wrapper {\n"
"            position: relative;\n"
"            height: 30%;\n"
"            min-height: 314px;\n"
"            max-height: 510px;\n"
"            width: 1000px;\n"
"            margin: 0 auto;\n"
"        }\n"
"\n"
"        #head_wrapper .s-p-top {\n"
"            height: 60%;\n"
"            min-height: 185px;\n"
"            max-height: 310px;\n"
"            position: relative;\n"
"            z-index: 0;\n"
"            text-align: center;\n"
"        }\n"
"\n"
"        #head_wrapper input {\n"
"            outline: 0;\n"
"            -webkit-appearance: none;\n"
"        }\n"
"\n"
"        #head_wrapper .s_btn_wr,\n"
"        #head_wrapper .s_ipt_wr {\n"
"            display: inline-block;\n"
"            *display: inline;\n"
"            zoom: 1;\n"
"            background: 0 0;\n"
"            vertical-align: top;\n"
"            *vertical-align: middle;\n"
"        }\n"
"\n"
"        #head_wrapper .s_ipt_wr {\n"
"            position: relative;\n"
"            width: 546px;\n"
"        }\n"
"\n"
"        #head_wrapper .s_btn_wr {\n"
"            width: 108px;\n"
"            height: 44px;\n"
"            position: relative;\n"
"            z-index: 2;\n"
"        }\n"
"\n"
"        #head_wrapper .s_ipt_wr:hover #kw {\n"
"            border-color: #a7aab5;\n"
"        }\n"
"\n"
"        #head_wrapper #kw {\n"
"            width: 512px;\n"
"            height: 16px;\n"
"            padding: 12px 16px;\n"
"            font-size: 16px;\n"
"            margin: 0;\n"
"            vertical-align: top;\n"
"            outline: 0;\n"
"            box-shadow: none;\n"
"            border-radius: 10px 0 0 10px;\n"
"            border: 2px solid #c4c7ce;\n"
"            background: #fff;\n"
"            color: #222;\n"
"            overflow: hidden;\n"
"            box-sizing: content-box;\n"
"        }\n"
"\n"
"        #head_wrapper #kw:focus {\n"
"            border-color: #4e6ef2 !important;\n"
"            opacity: 1;\n"
"            filter: alpha(opacity=100)\\9\n"
"        }\n"
"\n"
"        #head_wrapper .s_form {\n"
"            width: 654px;\n"
"            height: 100%;\n"
"            margin: 0 auto;\n"
"            text-align: left;\n"
"            z-index: 100;\n"
"        }\n"
"\n"
"        #head_wrapper .s_btn {\n"
"            cursor: pointer;\n"
"            width: 108px;\n"
"            height: 44px;\n"
"            line-height: 45px;\n"
"            line-height: 44px\\9;\n"
"            padding: 0;\n"
"            background: 0 0;\n"
"            background-color: #4e6ef2;\n"
"            border-radius: 0 10px 10px 0;\n"
"            font-size: 17px;\n"
"            color: #fff;\n"
"            box-shadow: none;\n"
"            font-weight: 400;\n"
"            border: none;\n"
"            outline: 0;\n"
"        }\n"
"\n"
"        #head_wrapper .s_btn:hover {\n"
"            background-color: #4662d9;\n"
"        }\n"
"\n"
"        #head_wrapper .s_btn:active {\n"
"            background-color: #4662d9;\n"
"        }\n"
"\n"
"        #head_wrapper .quickdelete-wrap {\n"
"            position: relative;\n"
"        }\n"
"\n"
"        #s_top_wrap {\n"
"            position: absolute;\n"
"            z-index: 99;\n"
"            min-width: 1000px;\n"
"            width: 100%;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper.s-hotsearch-wrapper {\n"
"            margin: 45px auto 0;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-title {\n"
"            height: 24px;\n"
"            width: 100%;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-title .title-text {\n"
"            float: left;\n"
"            user-select: none;\n"
"            color: #222;\n"
"            font: 14px / 24px Arial, sans-serif;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content {\n"
"            text-align: left;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .hotsearch-item .title-content-top-icon {\n"
"            display: none;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper\n"
"            .s-hotsearch-content\n"
"            .hotsearch-item[data-index='0']\n"
"            .title-content-index {\n"
"            display: none;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper\n"
"            .s-hotsearch-content\n"
"            .hotsearch-item[data-index='0']\n"
"            .title-content-top-icon {\n"
"            display: inline-block;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .hotsearch-item {\n"
"            width: 306px;\n"
"            float: left;\n"
"            height: 36px;\n"
"            line-height: 36px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .hotsearch-item.odd {\n"
"            margin-right: 20px;\n"
"            clear: both;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .hotsearch-item.even {\n"
"            margin-left: 20px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content {\n"
"            display: block;\n"
"            float: left;\n"
"            height: 36px;\n"
"            line-height: 36px;\n"
"            font-size: 14px;\n"
"            width: 100%;\n"
"            color: #222;\n"
"            text-decoration: none;\n"
"            overflow: hidden;\n"
"            text-overflow: ellipsis;\n"
"            white-space: nowrap;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content:hover {\n"
"            color: #315efb;\n"
"            text-decoration: underline;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .tag-width {\n"
"            max-width: 282px;\n"
"            width: auto;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content-top-icon {\n"
"            transform: rotate(180deg);\n"
"            height: 18px;\n"
"            width: 18px;\n"
"            display: inline-block;\n"
"            vertical-align: middle;\n"
"            line-height: 18px;\n"
"            position: relative;\n"
"            top: -2px;\n"
"            left: -3px;\n"
"            font-size: 18px;\n"
"            color: #f63051;\n"
"            margin-right: 8px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content-index {\n"
"            margin-right: 4px;\n"
"            width: 22px;\n"
"            font-family: Arial, sans-serif;\n"
"            font-size: 18px;\n"
"            line-height: 18px;\n"
"            position: relative;\n"
"            top: 1px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content-title {\n"
"            font-size: 16px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .s-hotsearch-content .title-content-mark {\n"
"            margin-left: 6px;\n"
"            position: relative;\n"
"            top: -2px;\n"
"            display: inline-block;\n"
"            padding: 0 2px;\n"
"            text-align: center;\n"
"            vertical-align: middle;\n"
"            font-style: normal;\n"
"            color: #fff;\n"
"            overflow: hidden;\n"
"            line-height: 16px;\n"
"            height: 16px;\n"
"            font-size: 12px;\n"
"            border-radius: 4px;\n"
"            font-weight: 200;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .c-text-hot {\n"
"            background-color: #f60;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .c-index-single {\n"
"            display: inline-block;\n"
"            background: 0 0;\n"
"            color: #9195a3;\n"
"            letter-spacing: -1px;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .c-index-single-hot1 {\n"
"            color: #fe2d46;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .c-index-single-hot2 {\n"
"            color: #f60;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper .c-index-single-hot3 {\n"
"            color: #faa90e;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper blockquote,\n"
"        body,\n"
"        button,\n"
"        dd,\n"
"        dl,\n"
"        dt,\n"
"        fieldset,\n"
"        form,\n"
"        h1,\n"
"        h2,\n"
"        h3,\n"
"        h4,\n"
"        h5,\n"
"        h6,\n"
"        hr,\n"
"        input,\n"
"        legend,\n"
"        li,\n"
"        ol,\n"
"        p,\n"
"        pre,\n"
"        td,\n"
"        textarea,\n"
"        th,\n"
"        ul {\n"
"            margin: 0;\n"
"            padding: 0;\n"
"        }\n"
"\n"
"        #s-hotsearch-wrapper form,\n"
"        li,\n"
"        p,\n"
"        ul {\n"
"            list-style: none;\n"
"        }\n"
"\n"
"        .c-icon {\n"
"            font-family: cIconfont !important;\n"
"            font-style: normal;\n"
"            -webkit-font-smoothing: antialiased;\n"
"        }\n"
"\n"
"        .s-top-left {\n"
"            position: absolute;\n"
"            left: 0;\n"
"            top: 0;\n"
"            z-index: 100;\n"
"            height: 60px;\n"
"            padding-left: 24px;\n"
"        }\n"
"\n"
"        .s-top-left .mnav {\n"
"            margin-right: 31px;\n"
"            margin-top: 19px;\n"
"            display: inline-block;\n"
"            position: relative;\n"
"        }\n"
"\n"
"        .s-top-left .mnav:hover .s-bri,\n"
"        .s-top-left a:hover {\n"
"            color: #315efb;\n"
"            text-decoration: none;\n"
"        }\n"
"\n"
"        .s-top-left .s-top-more-btn {\n"
"            padding-bottom: 19px;\n"
"        }\n"
"\n"
"        .s-top-left .s-top-more-btn:hover .s-top-more {\n"
"            display: block;\n"
"        }\n"
"\n"
"        .s-top-right {\n"
"            position: absolute;\n"
"            right: 0;\n"
"            top: 0;\n"
"            z-index: 100;\n"
"            height: 60px;\n"
"            padding-right: 24px;\n"
"        }\n"
"\n"
"        .s-top-right .s-top-right-text {\n"
"            margin-left: 32px;\n"
"            margin-top: 19px;\n"
"            display: inline-block;\n"
"            position: relative;\n"
"            vertical-align: top;\n"
"            cursor: pointer;\n"
"        }\n"
"\n"
"        .s-top-right .s-top-right-text:hover {\n"
"            color: #315efb;\n"
"        }\n"
"\n"
"        .s-top-right .s-top-login-btn {\n"
"            display: inline-block;\n"
"            margin-top: 18px;\n"
"            margin-left: 32px;\n"
"            font-size: 13px;\n"
"        }\n"
"\n"
"        .s-top-right a:hover {\n"
"            text-decoration: none;\n"
"        }\n"
"\n"
"        #bottom_layer {\n"
"            width: 100%;\n"
"            position: fixed;\n"
"            z-index: 302;\n"
"            bottom: 0;\n"
"            left: 0;\n"
"            height: 39px;\n"
"            padding-top: 1px;\n"
"            overflow: hidden;\n"
"            zoom: 1;\n"
"            margin: 0;\n"
"            line-height: 39px;\n"
"            background: #fff;\n"
"        }\n"
"\n"
"        #bottom_layer .lh {\n"
"            display: inline;\n"
"            margin-right: 20px;\n"
"        }\n"
"\n"
"        #bottom_layer .lh:last-child {\n"
"            margin-left: -2px;\n"
"            margin-right: 0;\n"
"        }\n"
"\n"
"        #bottom_layer .lh.activity {\n"
"            font-weight: 700;\n"
"            text-decoration: underline;\n"
"        }\n"
"\n"
"        #bottom_layer a {\n"
"            font-size: 12px;\n"
"            text-decoration: none;\n"
"        }\n"
"\n"
"        #bottom_layer .text-color {\n"
"            color: #bbb;\n"
"        }\n"
"\n"
"        #bottom_layer a:hover {\n"
"            color: #222;\n"
"        }\n"
"\n"
"        #bottom_layer .s-bottom-layer-content {\n"
"            text-align: center;\n"
"        }\n"
"\n"
"        @font-face {\n"
"            font-family: cIconfont;\n"
"            src: url('https://pss.bdstatic.com/static/superman/font/iconfont-cdfecb8456.eot');\n"
"            src: url('https://pss.bdstatic.com/static/superman/font/iconfont-cdfecb8456.eot?#iefix')\n"
"                    format('embedded-opentype'),\n"
"                url('https://pss.bdstatic.com/static/superman/font/iconfont-fa013548a9.woff2')\n"
"                    format('woff2'),\n"
"                url('https://pss.bdstatic.com/static/superman/font/iconfont-840387fb42.woff')\n"
"                    format('woff'),\n"
"                url('https://pss.bdstatic.com/static/superman/font/iconfont-4530e108b6.ttf')\n"
"                    format('truetype'),\n"
"                url('https://pss.bdstatic.com/static/superman/font/iconfont-74fcdd51ab.svg#iconfont')\n"
"                    format('svg');\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div id=\"wrapper\" class=\"wrapper_new\">\n"
"        <div id=\"head\">\n"
"            <div id=\"s-top-left\" class=\"s-top-left s-isindex-wrap\">\n"
"                <a href=\"//news.baidu.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">新闻</a>\n"
"                <a href=\"//www.hao123.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">hao123</a>\n"
"                <a href=\"//map.baidu.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">地图</a>\n"
"                <a href=\"//live.baidu.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">直播</a>\n"
"                <a href=\"//haokan.baidu.com/?sfrom=baidu-top\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">视频</a>\n"
"                <a href=\"//tieba.baidu.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">贴吧</a>\n"
"                <a href=\"//xueshu.baidu.com/\" target=\"_blank\" class=\"mnav c-font-normal c-color-t\">学术</a>\n"
"                <div class=\"mnav s-top-more-btn\">\n"
"                    <a href=\"//www.baidu.com/more/\" name=\"tj_briicon\" class=\"s-bri c-font-normal c-color-t\" target=\"_blank\">更多</a>\n"
"                </div>\n"
"            </div>\n"
"            <div id=\"u1\" class=\"s-top-right s-isindex-wrap\">\n"
"                <a\n"
"                    class=\"s-top-login-btn c-btn c-btn-primary c-btn-mini lb\"\n"
"                    style=\"position:relative;overflow: visible;\"\n"
"                    name=\"tj_login\"\n"
"                    href=\"//www.baidu.com/bdorz/login.gif?login&amp;tpl=mn&amp;u=http%3A%2F%2Fwww.baidu.com%2f%3fbdorz_come%3d1\"\n"
"                >登录</a>\n"
"            </div>\n"
"            <div id=\"head_wrapper\" class=\"head_wrapper s-isindex-wrap s-ps-islite\">\n"
"                <div class=\"s_form\">\n"
"                    <div class=\"s_form_wrapper\">\n"
"                        <div id=\"lg\" class=\"s-p-top\">\n"
"                            <img\n"
"                                hidefocus=\"true\"\n"
"                                id=\"s_lg_img\"\n"
"                                class=\"index-logo-src\"\n"
"                                src=\"//www.baidu.com/img/flexible/logo/pc/index.png\"\n"
"                                width=\"270\"\n"
"                                height=\"129\"\n"
"                                usemap=\"#mp\"\n"
"                            />\n"
"                            <map name=\"mp\">\n"
"                                <area\n"
"                                    style=\"outline: none\"\n"
"                                    hidefocus=\"true\"\n"
"                                    shape=\"rect\"\n"
"                                    coords=\"0,0,270,129\"\n"
"                                    href=\"//www.baidu.com/s?wd=%E7%99%BE%E5%BA%A6%E7%83%AD%E6%90%9C&amp;sa=ire_dl_gh_logo_texing&amp;rsv_dl=igh_logo_pcs\"\n"
"                                    target=\"_blank\"\n"
"                                    title=\"点击一下，了解更多\"\n"
"                                />\n"
"                            </map>\n"
"                        </div>\n"
"                        <a href=\"//www.baidu.com/\" id=\"result_logo\"></a>\n"
"                        <form id=\"form\" name=\"f\" action=\"//www.baidu.com/s\" class=\"fm\">\n"
"                            <input type=\"hidden\" name=\"ie\" value=\"utf-8\" />\n"
"                            <input type=\"hidden\" name=\"f\" value=\"8\" />\n"
"                            <input type=\"hidden\" name=\"rsv_bp\" value=\"1\" />\n"
"                            <input type=\"hidden\" name=\"rsv_idx\" value=\"1\" />\n"
"                            <input type=\"hidden\" name=\"ch\" value=\"\" />\n"
"                            <input type=\"hidden\" name=\"tn\" value=\"baidu\" />\n"
"                            <input type=\"hidden\" name=\"bar\" value=\"\" />\n"
"                            <span class=\"s_ipt_wr quickdelete-wrap\">\n"
"                                <input\n"
"                                    id=\"kw\"\n"
"                                    name=\"wd\"\n"
"                                    class=\"s_ipt\"\n"
"                                    value=\"\"\n"
"                                    maxlength=\"255\"\n"
"                                    autocomplete=\"off\"\n"
"                                /></span\n"
"                            ><span class=\"s_btn_wr\">\n"
"                                <input\n"
"                                    type=\"submit\"\n"
"                                    id=\"su\"\n"
"                                    value=\"百度一下\"\n"
"                                    class=\"bg s_btn\"\n"
"                                />\n"
"                            </span>\n"
"                            <input type=\"hidden\" name=\"rn\" value=\"\" />\n"
"                            <input type=\"hidden\" name=\"fenlei\" value=\"256\" />\n"
"                            <input type=\"hidden\" name=\"oq\" value=\"\" />\n"
"                            <input type=\"hidden\" name=\"rsv_pq\" value=\"b9ff093e0000e419\" />\n"
"                            <input\n"
"                                type=\"hidden\"\n"
"                                name=\"rsv_t\"\n"
"                                value=\"3635FYbdbC8tlWmudZmYaUnaucNe+RzTzNEGqg/JuniQU10WL5mtMQehIrU\"\n"
"                            />\n"
"                            <input type=\"hidden\" name=\"rqlang\" value=\"cn\" />\n"
"                            <input type=\"hidden\" name=\"rsv_enter\" value=\"1\" />\n"
"                            <input type=\"hidden\" name=\"rsv_dl\" value=\"ib\" />\n"
"                        </form>\n"
"                    </div>\n"
"                    <div id=\"s-hotsearch-wrapper\" class=\"s-hotsearch-wrapper\">\n"
"                        <div class=\"s-hotsearch-title\">\n"
"                            <a\n"
"                                href=\"https://top.baidu.com/board?platform=pc&amp;sa=pcindex_entry\"\n"
"                                target=\"_blank\"\n"
"                            >\n"
"                                <div\n"
"                                    class=\"title-text c-font-medium c-color-t\"\n"
"                                    aria-label=\"百度热搜\"\n"
"                                >\n"
"                                    <i class=\"c-icon\"></i>\n"
"                                    <i class=\"c-icon arrow\"></i>\n"
"                                </div>\n"
"                            </a>\n"
"                        </div>\n"
"                        <ul class=\"s-hotsearch-content\" id=\"hotsearch-content-wrapper\">\n"
"                            \n"
"<li class=\"hotsearch-item odd\" data-index=\"0\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E6%80%BB%E4%B9%A6%E8%AE%B0%E7%9A%84%E4%B8%80%E5%91%A8&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot0\">0</span><span class=\"title-content-title\">总书记的一周</span></a></li>\n"
"<li class=\"hotsearch-item even\" data-index=\"5\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E5%BE%AE%E4%BF%A1%E6%96%B0%E5%8A%9F%E8%83%BD%E4%B8%8A%E7%BA%BF+%E5%8F%AF%E4%B8%80%E9%94%AE%E5%88%A0%E9%99%A4%E5%8D%95%E5%90%91%E5%A5%BD%E5%8F%8B&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot5\">5</span><span class=\"title-content-title\">微信新功能上线 可一键删除单向好友</span></a><span class=\"title-content-mark c-text-hot\">热</span></li>\n"
"<li class=\"hotsearch-item odd\" data-index=\"1\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E4%B9%A0%E8%BF%91%E5%B9%B3%E5%87%BA%E5%B8%AD%E6%B0%91%E8%90%A5%E4%BC%81%E4%B8%9A%E5%BA%A7%E8%B0%88%E4%BC%9A%E5%B9%B6%E8%AE%B2%E8%AF%9D&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot1\">1</span><span class=\"title-content-title\">习近平出席民营企业座谈会并讲话</span></a></li>\n"
"<li class=\"hotsearch-item even\" data-index=\"6\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E8%BF%99%E4%B8%8B%E9%81%87%E5%88%B0%E7%9C%9F%E7%9F%B3%E7%9F%B6%E5%A8%98%E5%A8%98%E4%BA%86&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot6\">6</span><span class=\"title-content-title\">这下遇到真石矶娘娘了</span></a></li>\n"
"<li class=\"hotsearch-item odd\" data-index=\"2\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E3%80%8A%E5%93%AA%E5%90%922%E3%80%8B%E8%B6%85%E3%80%8A%E7%8B%AE%E5%AD%90%E7%8E%8B%E3%80%8B%E8%BF%9B%E5%85%A8%E7%90%83%E7%A5%A8%E6%88%BF%E5%89%8D10&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot2\">2</span><span class=\"title-content-title\">《哪吒2》超《狮子王》进全球票房前10</span></a><span class=\"title-content-mark c-text-hot\">热</span></li>\n"
"<li class=\"hotsearch-item even\" data-index=\"7\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E4%BA%8E%E6%AD%A3%E5%8F%91%E9%95%BF%E6%96%87%E8%87%B4%E6%AD%89&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot7\">7</span><span class=\"title-content-title\">于正发长文致歉</span></a></li>\n"
"<li class=\"hotsearch-item odd\" data-index=\"3\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E2%80%9C%E6%9D%91%E8%B6%85%E2%80%9D%E6%8C%81%E7%BB%AD%E5%8A%A9%E6%8E%A8%E4%B9%A1%E6%9D%91%E6%8C%AF%E5%85%B4&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot3\">3</span><span class=\"title-content-title\">“村超”持续助推乡村振兴</span></a></li>\n"
"<li class=\"hotsearch-item even\" data-index=\"8\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E5%B0%BC%E6%A0%BC%E4%B9%B0%E6%8F%90%EF%BC%9A%E5%88%AB%E6%89%BE%E4%BA%86%E7%BB%93%E7%95%8C%E5%85%BD%E5%9C%A8%E7%A6%BE%E6%9C%A8&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot8\">8</span><span class=\"title-content-title\">尼格买提：别找了结界兽在禾木</span></a></li>\n"
"<li class=\"hotsearch-item odd\" data-index=\"4\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E3%80%8A%E5%93%AA%E5%90%922%E3%80%8B%E6%80%BB%E7%A5%A8%E6%88%BF%E7%AA%81%E7%A0%B4120%E4%BA%BF&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot4\">4</span><span class=\"title-content-title\">《哪吒2》总票房突破120亿</span></a><span class=\"title-content-mark c-text-hot\">热</span></li>\n"
"<li class=\"hotsearch-item even\" data-index=\"9\"><a class=\"title-content tag-width\" href=\"https://www.baidu.com/s?wd=%E5%A4%A7%E5%AD%A6%E7%94%9F60%E7%B1%B3%E6%A0%8F%E8%B7%91%E5%87%BA7.53%E9%85%B7%E4%BC%BC%E5%88%98%E7%BF%94&sa=ipc_home_hotword_jt&rsv_dl=ipc_home_hotword_jt&from=super&cl=3&tn=baidutop10&fr=top1000&rsv_idx=2&hisfilter=1\" target=\"_blank\"><i class=\"c-icon title-content-top-icon\"></i><span class=\"title-content-index c-index-single c-index-single-hot9\">9</span><span class=\"title-content-title\">大学生60米栏跑出7.53酷似刘翔</span></a></li>\n"
"                        </ul>\n"
"                    </div>\n"
"                </div>\n"
"            </div>\n"
"\n"
"            <div id=\"bottom_layer\" class=\"s-bottom-layer s-isindex-wrap\">\n"
"                <div class=\"s-bottom-layer-content\">\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//home.baidu.com/\" target=\"_blank\">关于百度</a></p>\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//ir.baidu.com/\" target=\"_blank\">About Baidu</a></p>\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//www.baidu.com/duty\" target=\"_blank\">使用百度前必读</a>\n"
"                    </p>\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//help.baidu.com/\" target=\"_blank\">帮助中心</a></p>\n"
"                    <p class=\"lh\"><a\n"
"                            class=\"text-color\"\n"
"                            href=\"//www.beian.gov.cn/portal/registerSystemInfo?recordcode=11000002000001\"\n"
"                            target=\"_blank\">京公网安备11000002000001号</a></p>\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//beian.miit.gov.cn/\"\n"
"                            target=\"_blank\">京ICP证030173号</a></p>\n"
"                    <p class=\"lh\"><span id=\"year\" class=\"text-color\"><noscript>© Baidu</noscript></span></p>\n"
"                    <p class=\"lh\"><span class=\"text-color\">互联网药品信息服务资格证书 (京)-经营性-2017-0020</span></p>\n"
"                    <p class=\"lh\"><a class=\"text-color\" href=\"//www.baidu.com/licence/\"\n"
"                            target=\"_blank\">信息网络传播视听节目许可证 0110516</a></p>\n"
"                </div>\n"
"            </div>\n"
"        </div>\n"
"    </div>\n"
"    <script type=\"text/javascript\">\n"
"        var date = new Date();\n"
"        var year = date.getFullYear();\n"
"        document.getElementById('year').innerText = '©' + year + ' Baidu ';\n"
"    </script>\n"
"</body>\n"
"</html>";
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
            std::cout << "    KeepAlive: " << (resp->isKeepAlive() ? "keep-alive" : "close") << std::endl;
            std::cout << "    Content-Type: " << resp->getHeader("Content-Type") << std::endl;
            // std::cout << "    Body: [" << resp->getBody() << "]" << std::endl;
            return 0;
        });
        parser.Init();
        parser.Execute(http_response);
        // if (parser.getData()->getBody())
        // parser.Finalize();
        std::cout << "  [通过]\n" << std::endl;
        std::cout << "--------- to string start ----------\n";
        std::cout << parser.getData()->toString() << std::endl;
        std::cout << "--------- to string end ----------\n";
        
    return 0;
}