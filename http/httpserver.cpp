#include "blue/log.h"
#include "httpserver.h"
#include <chrono>
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");

        template <typename T>
        HttpServer<T>::HttpServer(bool keepAlive, int level, int option_name, T option, IOManager *manager, IOManager *acceptmanager)
            : TcpServer<T>(level, option_name, option, manager, acceptmanager),
              m_keepAlive(keepAlive)
        {
            m_dispatch.reset(new ServletDispatch());
        }

        template <typename T>
        void HttpServer<T>::handleClient(MSocket::MSocketPtr sock)
        {
            auto session = std::make_shared<HttpSession>(sock);
            bool temkeepAlive = m_keepAlive;
            do
            {
                // 若双方有一个是长连接就长连接
                // 错误由recvRequest处理
                auto [recvstatus, requestPtr] = session->recvRequest();
                if (recvstatus == http::HttpSession::RecvStatus::ERROR || 
                    recvstatus == http::HttpSession::RecvStatus::CLOSE)
                {
                    session->close();
                    return;
                }
                auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(), (requestPtr->isKeepAlive() || temkeepAlive));
                temkeepAlive = (requestPtr->isKeepAlive() || temkeepAlive);
                // auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(), false);
                // const char *responseData =
                //     "<html>\r\n"
                //     "<head>\r\n"
                //     "<meta charset='UTF-8'>\r\n"
                //     "<style>\r\n"
                //     "body{margin:0;height:100vh;background:#1677ff;display:flex;justify-content:center;align-items:center;font-family:sans-serif;}\r\n"
                //     ".box{background:#fff;padding:60px 100px;border-radius:16px;box-shadow:0 8px 30px rgba(0,0,0,0.2);}\r\n"
                //     ".text{font-size:48px;color:#1677ff;font-weight:bold;}\r\n"
                //     "</style>\r\n"
                //     "</head>\r\n"
                //     "<body>\r\n"
                //     "<div class='box'>\r\n"
                //     "<div class='text'>Hello, blue</div>\r\n"
                //     "</div>\r\n"
                //     "</body>\r\n"
                //     "</html>";
                // responsePtr->setBody(responseData);
                // BLUE_LOG_INFO(g_logger) << "request : \n"
                //                         << requestPtr->toString();
                // BLUE_LOG_INFO(g_logger) << "response : \n"
                //                         << responsePtr->toString();
                m_dispatch->handle(requestPtr, responsePtr, session);
                // 自动补全缺失的响应头
                if (responsePtr->getHeader("Content-Type").empty()) {
                    responsePtr->setHeader("Content-Type", "text/html; charset=utf-8");
                }
                if (responsePtr->getHeader("Content-Length").empty()) {
                    responsePtr->setHeader("Content-Length", std::to_string(responsePtr->getBody().size()));
                }
                session->sendResponse(responsePtr);
                BLUE_LOG_INFO(g_logger) << sock->getRemoteAddress()->toString()
                                        << " \"" << http::HttpMethodToChars(requestPtr->getMethod())
                                        << " " << requestPtr->getPath()
                                        << " HTTP/" << requestPtr->versionToStr() << "\" "
                                        << http::HttpStatusToChars(responsePtr->getStatus())
                                        << " " << responsePtr->getBody().size() << "B "
                                        << requestPtr->getHeader("User-Agent");
            } while (temkeepAlive);
            session->close();
            return;
        }

        template class blue::http::HttpServer<int>;
        template class blue::http::HttpServer<timeval>;
    }
}