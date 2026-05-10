#ifndef __BLUE_HTTP_HTTPSERVER_H__
#define __BLUE_HTTP_HTTPSERVER_H__
#include "httpsession.h"
#include "httpservlet.h"
#include "blue/tcpServer.h"

namespace blue
{
    namespace http
    {
        /* 服务器端 */
        template <typename T>
        class HttpServer : public TcpServer<T>
        {
            public:
                using HttpServerPtr = std::shared_ptr<HttpServer<T>>;
            public:
                /**
                 * @brief http服务构造函数
                 * @param keepAlive 服务器是否选择保持长连接,true表示保持长连接
                 * @param level 协议例如SOL_SOCKET 默认 -1
                 * @param option_name 选项名称 例如SO_REUSEADDR 默认 -1
                 * @param option 选项值 1开启 0禁用 默认 0
                 * @param manager 工作iomanager
                 * @param acceptmanager 处理accpet的iomanager
                 */
                HttpServer(bool keepAlive = false,int level = -1, int option_name = -1, T option = T(), IOManager* manager = IOManager::GetThis(),
                            IOManager* acceptmanager = IOManager::GetThis());

                /**
                 * @brief 获取servlet diapatch的智能指针对象
                 * @return servlet diapatch的智能指针对象
                 */
                std::shared_ptr<ServletDispatch> getDispatch() const { return m_dispatch; }

                /**
                 * @brief 设置新的servlet dispatch
                 * @param dispatch 新的servlet dispatch对象智能指针
                 */
                void setDispatch(std::shared_ptr<ServletDispatch> dispatch) { m_dispatch = dispatch; }
            protected:
                /**
                 * @brief 处理客户端和服务端的请求和响应
                 * @param sock 客户端socket对象智能指针
                 */
                virtual void handleClient(MSocket::MSocketPtr sock) override;
            private:
                bool m_keepAlive;
                std::shared_ptr<ServletDispatch> m_dispatch;
        };
    }
}

#endif