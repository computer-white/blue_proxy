#include "../blue/log.h"
#include "../blue/util.h"
#include <iostream>
int main()
{
    std::cout << "Running tests..." << std::endl;

    auto logger = std::make_shared<blue::Logger>();
    logger->setlevel(blue::Level::DEBUG);

    auto console_appender = std::make_shared<blue::StdoutLogAppender>();
    auto file_appender = std::make_shared<blue::FileoutLogAppender>("./log.txt");

    logger->addAppender(console_appender);
    logger->addAppender(file_appender);
    // 自定义format
    blue::LogFormatter::LogFormatterPtr fmt = 
    std::make_shared<blue::LogFormatter>("%d%T%p%T%m%n");
    // 设置file_appender的fmt以及lever
    file_appender->setformatter(fmt);
    file_appender->setLevel(blue::Level::ERROR);

    BLUE_LOG_INFO(logger) << "INFO-log";
    BLUE_LOG_ERROR(logger) << "ERROR-log";
    BLUE_LOG_FORMAT_DEBUGE(logger,"DEBUGE-log fmt %s","aa");


    auto l = blue::LoggerMgr::GetInstance()->getLogger("xxx");
    BLUE_LOG_ERROR(l) << "xxx";

    std::cout << "Tests passed!" << std::endl;
    return 0;
}