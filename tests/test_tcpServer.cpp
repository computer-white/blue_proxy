#include "blue/tcpServer.h"
#include "blue/iomanager.h"
#include "blue/log.h"

static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
void run()
{
	auto res = blue::Address::LookupAny("0.0.0.0:8080");
	// unlink("/tmp/unix_address");
	// unlink("/tmp/unix_address_123456");
	// auto unixaddr = std::make_shared<blue::UnixAddress>(blue::UnixAddress("/tmp/unix_address_123456"));
	// BLUE_LOG_INFO(g_logger) << res->toString() << " unix : " << unixaddr->toString();

	std::vector<blue::Address::AddressPtr> addss = {res};
	std::vector<blue::Address::AddressPtr> fails;

	auto tcp = std::make_shared<blue::TcpServer<int>>();
	while(!tcp->bind(addss,fails))
	{
		sleep(2);
	};
	tcp->start();
}

void test_tcpserver()
{
	blue::IOManager iom(2,true,"test_tcpserver");
	iom.schedule(run);
}


int main(void)
{
	test_tcpserver();
	return 0;
}
