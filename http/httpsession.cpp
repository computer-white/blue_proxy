#include "blue/log.h"
#include "httpsession.h"
#include "httpParser.h"
#include <string>
namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
        HttpSession::HttpSession(MSocket::MSocketPtr sock, bool owner)
            : SocketStream(sock, owner)
        {
        }
    
        HttpSession::ReturnType HttpSession::recvRequest()
        {
            m_isFinish = false;
            auto parser = std::make_shared<HttpRequestParser>();
            parser->on_MessageComplate([this](blue::http::HttpRequest::HttpRequestPtr ptr) -> int{
                m_isFinish = true;
                return 0;
            });
            parser->Init();
            uint64_t requestbuffersize = HttpRequestParser::GetHttpRequestBufferSize();
            // 无脑vector 管理data
            std::vector<char> vec_data(requestbuffersize);
            auto data = vec_data.data();
            size_t offset = 0;
            do
            {
                ssize_t n = read(data + offset,requestbuffersize - offset);
                if (n == 0) // 客户端主动关闭连接
                {
                    // 连接关闭，尝试finalize
                    parser->Finalize(); // 告诉已经发送完数据了
                    if (m_isFinish) // 监测一个完整的http request发送完毕
                    {
                        return {HttpSession::RecvStatus::OK,parser->getData()};
                    }
                    return {HttpSession::RecvStatus::CLOSE,nullptr};
                }
                if (n < 0)
                {
                    if (errno == EINTR) continue;
                    // hook帮我们hook住了，如果没有数据了最后回来了回到hook重新去读了
                    if (errno == EAGAIN || errno ==EWOULDBLOCK) continue;
                    return {HttpSession::RecvStatus::ERROR,nullptr};
                }
                parser->Execute(data + offset,n);
                offset += n;
                if (parser->getError() != HPE_OK)
                {
                    BLUE_LOG_ERROR(g_logger) << "http request 格式错误";
                    return {HttpSession::RecvStatus::ERROR,nullptr};
                }
                if (m_isFinish)
                {
                    return {HttpSession::RecvStatus::OK,parser->getData()};
                }

                // 数据不够放了
                if (offset >= requestbuffersize)
                {
                    HttpRequestParser::SetRequestBufferSize(offset * 2);    // 热更新
                    size_t newsize = HttpRequestParser::GetHttpRequestBufferSize();

                    // 无脑vector 管理data
                    vec_data.resize(newsize);
                    data = vec_data.data();
                    requestbuffersize = newsize;
                }
            } while(true);

            return {HttpSession::RecvStatus::ERROR,nullptr};
        }

        int HttpSession::sendResponse(HttpResponse::HttpResponsePtr response)
        {
            std::string str = response->toString();
            return writeFixSize(str.c_str(),str.size());
        }

    }
}
