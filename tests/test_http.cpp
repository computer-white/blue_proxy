#include "llhttp.h"
#include <iostream>
#include <cstring>

int on_message_complete(llhttp_t* parser) {
    std::cout << "HTTP 消息解析完成！" << std::endl;
    return 0;
}

int main() {
    llhttp_t parser;
    llhttp_settings_t settings;
    
    llhttp_settings_init(&settings);
    settings.on_message_complete = on_message_complete;
    
    llhttp_init(&parser, HTTP_REQUEST, &settings);
    
    const char* request = "GET / HTTP/1.1\r\n\r\n";
    enum llhttp_errno err = llhttp_execute(&parser, request, strlen(request));
    
    if (err == HPE_OK) {
        std::cout << "解析成功！" << std::endl;
    } else {
        std::cout << "解析错误: " << llhttp_errno_name(err) << std::endl;
    }
    
    return 0;
}