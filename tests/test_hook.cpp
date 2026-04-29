#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "blue/hook.h"
#include "blue/iomanager.h"
#include "blue/log.h"
static blue::Logger::LoggerPtr g_logger = BLUE_LOG_MASSAGE_ROOT();
void test_sleep()
{
	blue::IOManager iom(1);
	iom.schedule([](){
		sleep(2);
		BLUE_LOG_INFO(g_logger) << "sleep 2";
	});

	iom.schedule([](){
		sleep(3);
		BLUE_LOG_INFO(g_logger) << "sleep 3";
	});
	BLUE_LOG_INFO(g_logger) << "test_sleep";
}

void test_socket()
{
	// blue::IOManager* iom = new blue::IOManager(1,true,"test_socket");
	// ipv4 + tcp + 非阻塞
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		BLUE_LOG_ERROR(g_logger) << "socket error";
		return;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET; // ipv4
	addr.sin_port = htons(80);
	// 把点分十进制ipv4地址转为网络字节数据表示的ip地址
	inet_pton(AF_INET,"110.242.70.57",&addr.sin_addr.s_addr);
	int rt = connect(sockfd,(const sockaddr*)&addr,sizeof(addr));
	BLUE_LOG_INFO(g_logger) << "rt : " << rt << "errno : " << strerror(errno);

	int error = 0;
	socklen_t len = sizeof(error);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error != 0) {
		BLUE_LOG_ERROR(g_logger) << "socket error: " << strerror(error);
		close(sockfd);
		return;
	}
	
	const char data[] = "GET / HTTP/1.0\r\n\r\n";
	int ret = send(sockfd, data, strlen(data), 0);
	if (ret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// 缓冲区满，继续等待 WRITE 事件
			return;
		}
		BLUE_LOG_ERROR(g_logger) << "send failed: " << strerror(errno);
		close(sockfd);
		return;
	}
	BLUE_LOG_INFO(g_logger) << "send " << ret << " bytes";
	char buf[4096];
	ret = recv(sockfd, buf, sizeof(buf), 0);
	if (ret > 0) {
		BLUE_LOG_INFO(g_logger) << "recv " << ret << " bytes";
		BLUE_LOG_INFO(g_logger) << buf;
	}
	// iom->addEvent(sockfd, blue::IOManager::Event::WRITE, [fd = sockfd,iom = iom]() {
	// 	// 检查 socket 状态
	// 	int error = 0;
	// 	socklen_t len = sizeof(error);
	// 	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error != 0) {
	// 		BLUE_LOG_ERROR(g_logger) << "socket error: " << strerror(error);
	// 		close(fd);
	// 		return;
	// 	}
		
	// 	const char data[] = "GET / HTTP/1.0\r\n\r\n";
	// 	int ret = send(fd, data, strlen(data), 0);
	// 	if (ret < 0) {
	// 		if (errno == EAGAIN || errno == EWOULDBLOCK) {
	// 			// 缓冲区满，继续等待 WRITE 事件
	// 			return;
	// 		}
	// 		BLUE_LOG_ERROR(g_logger) << "send failed: " << strerror(errno);
	// 		close(fd);
	// 		return;
	// 	}
	// 	BLUE_LOG_INFO(g_logger) << "send " << ret << " bytes";
		
	// 	// 发送成功，注册读事件接收响应
	// 	iom->addEvent(fd, blue::IOManager::Event::READ, [fd]() {
	// 		char buf[4096];
	// 		int ret = recv(fd, buf, sizeof(buf), 0);
	// 		if (ret > 0) {
	// 			BLUE_LOG_INFO(g_logger) << "recv " << ret << " bytes";
	// 			BLUE_LOG_INFO(g_logger) << buf;
	// 		}
	// 	});
	// });

	// delete iom; // delete 后程序才会启动
}
int main(int argv,char* argc[])
{
	// test_sleep();
	blue::IOManager iom(1);
	iom.schedule(test_socket);
	// test_socket();
	return 0;
}
