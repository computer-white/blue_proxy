#ifndef __BLUE_HTTP_HTTPSESSION_H__
#define __BLUE_HTTP_HTTPSESSION_H__
#include "blue/msocketstream.h"
#include "http.h"
#include "httpParser.h"

namespace blue
{
    namespace http
    {
        class HttpSession : public SocketStream
        {
        public:
            enum class RecvStatus
            {
                OK,    // 解析完成
                ERROR, // 真正的错误
                CLOSE  // 用户主动关闭
            };

        public:
            using HttpSessionPtr = std::shared_ptr<HttpSession>;
            using ReturnType = std::pair<RecvStatus, HttpRequest::HttpRequestPtr>;

        public:
            /**
             * @brief 服务器端与客户端一次连接的抽象
             * @param sock 用客户端sockfd封装的对象
             * @param owner 是否完全交给会话管理客户端sockfd对象,默认 true
             */
            HttpSession(MSocket::MSocketPtr sock, bool owner = true);

            /**
             * @brief 接受客户端请求并解析
             * @return 返回一对值(recvStatus,httpRequestPtr)
             * @note recvStatus返回有 ok,error,close(客户端主动关闭连接)
             */
            ReturnType recvRequest();

            /**
             * @brief 对客户端发送的请求予以响应
             * @param response 响应对象智能指针
             */
            int sendResponse(HttpResponse::HttpResponsePtr response);

        private:
            bool m_isFinish = false;
        };
    }
}

#endif