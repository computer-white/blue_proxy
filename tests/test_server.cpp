#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "blue/log.h"
#include "blue/iomanager.h"
#include "blue/mthread.h"
#include "blue/macro.h"
#include "blue/hook.h"
#include "blue/httprequest.h"

#define BUF_SIZE 1024
blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();

static const char* zbuf[] = {"hello,I get a corret result\n","somthing wrong\n"};

#include "blue/blue.h"
#include <iostream>

void test_http_server(const char* ip,int port) {
    blue::IOManager iom(2, true, "http_server");
    
    // 创建监听socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET,ip,&addr.sin_addr);
    bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 128);
    
    // 添加监听事件
    iom.addEvent(listen_fd, blue::IOManager::READ, [&iom, listen_fd]() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
        
        // 在协程中处理请求
        iom.schedule([client_fd]() {
            char buf[4096];
            int n = read(client_fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                
                // 解析HTTP请求
                blue::HttpRequest::HttpRequestPtr request = std::make_shared<blue::HttpRequest>();
                int check_idx = 0, start_idx = 0, end_idx = n;
                std::string sbuf(buf);
                auto res = request->parse_start(sbuf, check_idx, start_idx, end_idx);
                
                if (res == blue::HTTP_CODE::GET_QUESTION) {
                    std::string body = "<h1>This is a mediocre imitation of a wet server</h1>";
                    std::string response = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: " + std::to_string(body.size()) + "\r\n"
                        "Connection: close\r\n"
                        "\r\n" + body;
                    write(client_fd, response.c_str(), response.size());
                    // 打印出来检查
                    BLUE_LOG_INFO(g_logger) << "Response length: " << response.size();
                    // BLUE_LOG_FORMAT_INFO(g_logger,"Response length ");
                    BLUE_LOG_INFO(g_logger) << "Body size: " << body.size();
                }
                // else if (res == blue::HTTP_CODE::GET_QUESTION_WITH_BODY)
                // {
                //     auto message = request.getMessage();

                // }
            }
            close(client_fd);
        });
    });
    
    iom.start();
    iom.stop();
}

int main(int argc,char* argv[]) {
    if (argc <= 2) {
        BLUE_LOG_ERROR(g_logger) << "need ip + port";
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    test_http_server(ip,port);
    return 0;
}

// int main(int argc,char* argv[])
// {
//     if (argc <= 2)
//     {
//         BLUE_LOG_INFO(g_logger) << "参数过少,ip_address + port";
//         return 1;
//     }
//     const char* ip = argv[1];
//     int port = std::stoi(argv[2]);

//     int listenfd = socket(AF_INET,SOCK_STREAM,0);
//     BLUE_ASSERT(listenfd >= 0);

//     sockaddr_in address;
//     memset(&address,0,sizeof(address));
//     address.sin_family = AF_INET;
//     inet_pton(AF_INET,ip,&address.sin_addr);
//     address.sin_port = htons(port);

//     int ret = bind(listenfd,(struct sockaddr*)&address,sizeof(address));
//     BLUE_ASSERT(ret != -1);

//     ret = listen(listenfd,5);
//     BLUE_ASSERT(ret != -1);

//     while (true)
//     {
//         struct sockaddr_in client;
//         socklen_t client_Length = sizeof(client);
//         int fd = accept(listenfd,(struct sockaddr*)&client,&client_Length);
//         if (fd < 0)
//         {
//             BLUE_LOG_INFO(g_logger) << "accept failed, errno = " << errno << "strerrno = " << strerror(errno);
//             continue;
//         }
//         blue::IOManager iom(2,false,"blue");
//         iom.addEvent(fd,blue::IOManager::Event::READ,[fd_ = fd](){
//             blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
//             int data_read = 0;
//             int end = 0;
//             int check_idx = 0;
//             int start_idx = 0;
//             // blue::HttpRequest::CHECK_STATE checstate = blue::HttpRequest::CHECK_STATE_REQUESTLINE;
//             while (true)
//             {
//                 char buf[BUF_SIZE];
//                 memset(&buf,'\0',sizeof(buf));
//                 int data_read = recv(fd_,buf,BUF_SIZE-1,0);
//                 if (data_read == -1)
//                 {
//                     BLUE_LOG_ERROR(g_logger) << "reading failed\n";
//                     break;
//                 }
//                 else if (data_read == 0)
//                 {
//                     BLUE_LOG_INFO(g_logger) << "remote client has closed the connection\n";
//                     close(fd_);
//                     break;
//                 }
//                 // 请求解析行
//                 end += data_read;
//                 blue::HttpRequest request;
//                 std::string s_buf(buf);
//                 blue::HttpRequest::HTTP_CODE res = request.parse_start(s_buf,check_idx,start_idx,end);
//                 if (res == blue::HttpRequest::NO_QUESTION)
//                 {
//                     continue;
//                 }
//                 else if (res == blue::HttpRequest::GET_QUESTION)
//                 {
//                     send(fd_,zbuf[0],sizeof(zbuf[0]),0);
//                     close(fd_);
//                     break;
//                 }
//                 else
//                 {
//                     send(fd_,zbuf[1],sizeof(zbuf[1]),0);
//                     close(fd_);
//                     break;
//                 }
//             }
//         });
//     }
//     close(listenfd);
//     return 0;

// }