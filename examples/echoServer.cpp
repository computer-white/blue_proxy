#include "blue/bytearray.h"
#include "blue/tcpServer.h"
#include "blue/iomanager.h"
#include "blue/log.h"
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
class EchoServer : public blue::TcpServer<int>
{
    public:
        using EchoServerPtr = std::shared_ptr<EchoServer>;
        EchoServer(int type);
        void handleClient(blue::MSocket::MSocketPtr sock) override;
    private:
        int m_type = 0;
};

EchoServer::EchoServer(int type)
:TcpServer(),
m_type(type)
{

}

void EchoServer::handleClient(blue::MSocket::MSocketPtr sock)
{
    BLUE_LOG_INFO(g_logger) << "handleClient : " << sock->toString();
    blue::ByteArray::ByteArrayPtr byte = std::make_shared<blue::ByteArray>();
    while (true)
    {
        byte->clear();
        std::vector<iovec> vec;
        byte->getWriteBuffers(vec,1024);
        int rt = sock->recv(&vec[0],vec.size());
        if (rt == 0)
        {
            BLUE_LOG_INFO(g_logger) << "client closed";
            break;
        }
        else if (rt < 0)
        {
            BLUE_LOG_ERROR(g_logger) << "recv error : " << errno 
                                     << " strerror : " << strerror(errno);
            break;
        }
        BLUE_LOG_INFO(g_logger) << "rt : " << rt << " size : " << byte->getSize();
        byte->setSize(byte->getSize() + rt);
        byte->setPosition(0);
        if (m_type == 1)
        {
            BLUE_LOG_INFO(g_logger) << "rt : " << rt << " size : " << byte->getSize();
            BLUE_LOG_INFO(g_logger) << byte->toString();
        }
        else
        {
            BLUE_LOG_INFO(g_logger) << byte->toHexString();
        }
    }
    
}

int type = 1;

void run()
{
    EchoServer::EchoServerPtr echo = std::make_shared<EchoServer>(type);
    auto addr = blue::Address::LookupAny("0.0.0.0:8080");
    while (!echo->bind(addr))
    {
        sleep(2);
    }
    echo->start();
}

int main(int argc,char* argv[])
{
    if (argc < 2)
    {
        BLUE_LOG_INFO(g_logger) << "used as [" << argv[0] << "-t or [" << argv[0] << "-b]";
        return 0;
    }
    BLUE_LOG_INFO(g_logger) << argv[1];
    if (strcmp(argv[1],"-b") == 0)
    {
        type = 2;
    }
    blue::IOManager iom(2);
    iom.schedule(run);
    return 0;
}