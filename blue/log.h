#ifndef __BLUE_LOG_H__
#define __BLUE_LOG_H__
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <vector>
#include <type_traits>
#include <map>
#include "util.h"
#include "singleton.h"
#include "mthread.h"
// 宏展开输出
#define BLUE_LOG_LEVER(logger, level)                                                             \
    if (logger->get_level() <= level)                                                             \
    blue::LogEventWrap(blue::LogEvent::LogEventPtr(new blue::LogEvent(logger, level, __FILE__,    \
                                                                      time(0), __LINE__,    \
                                                                      0, blue::GetThreadId(),     \
                                                                      blue::GetFiberID(),         \
                                                                      blue::Mthread::GetName()))) \
        .getstringstream()

#define BLUE_LOG_DEBUGE(logger) BLUE_LOG_LEVER(logger, blue::Level::DEBUG)
#define BLUE_LOG_INFO(logger) BLUE_LOG_LEVER(logger, blue::Level::INFO)
#define BLUE_LOG_WARN(logger) BLUE_LOG_LEVER(logger, blue::Level::WARN)
#define BLUE_LOG_ERROR(logger) BLUE_LOG_LEVER(logger, blue::Level::ERROR)
#define BLUE_LOG_FATAL(logger) BLUE_LOG_LEVER(logger, blue::Level::FATAL)

// format输出
#define BLUE_LOG_FORMAT_LEVEL(logger, fmt, level, ...)                                              \
    if (logger->get_level() <= level)                                                               \
    blue::LogEventWrap(blue::LogEvent::LogEventPtr(new blue::LogEvent(logger, level, __FILE__,      \
                                                                      time(0), __LINE__, 0,  \
                                                                      blue::GetThreadId(),          \
                                                                      blue::GetFiberID(),           \
                                                                      blue::Mthread::GetName())))   \
        .getEvent()                                                                                 \
        ->format(fmt, ##__VA_ARGS__)

#define BLUE_LOG_FORMAT_DEBUGE(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::DEBUG, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_INFO(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::INFO, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_WARN(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::WARN, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_ERROR(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::ERROR, ##__VA_ARGS__)
#define BLUE_LOG_FORMAT_FATAL(logger, fmt, ...) BLUE_LOG_FORMAT_LEVEL(logger, fmt, blue::Level::FATAL, ##__VA_ARGS__)

// 输出LogMessageRoot,默认输出到控制台
#define BLUE_LOG_MASSAGE_ROOT() blue::LoggerMgr::GetInstance()->GetRoot()

// 用name获取Logger,如果不存在那么输出的位置跟BLUE_LOG_MASSAGE_ROOT() 得到的Logger输出位置一样
#define BLUE_LOG_NAME(name) blue::LoggerMgr::GetInstance()->GetLogger(name)

// 名称空间
namespace blue
{

    class Logger;        // 前置声明,日志名称可以由Appender往上传到Formatter
    enum class Level;    // 限定作用域的枚举类的前置声明
    class LoggerManager; // 前置声明Message

    class LogEvent
    {
    public:
        using LogEventPtr = std::shared_ptr<LogEvent>;
        LogEvent() = default;
        LogEvent(std::shared_ptr<Logger> logger_ptr, Level level,
                 const char *file, uint64_t time, uint32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, const std::string &name);

        const char *getFilename() const { return m_file; }
        uint64_t getTime() const { return m_time; }
        uint32_t getLines() const { return m_lines; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadID; }
        uint32_t getFiberId() const { return m_fiberID; }
        const std::string &getThreadName() const { return m_threadname; }
        std::string getContent() const { return m_stringstream.str(); }
        std::shared_ptr<Logger> getLoggerptr() const { return m_logger_ptr; }
        std::stringstream &getstringstream() { return m_stringstream; }
        Level getLevel() const { return m_level; }
        void format(const char *fmt, ...);
        void format(const char *fmt, va_list al);

    private:
        // 32个字节:内存对齐
        const char *m_file = nullptr;         // 文件名
        uint64_t m_time = 0;                  // 时间戳
        uint32_t m_lines = 0;                 // 行号
        uint32_t m_elapse = 0;                // 程序到现在启动时间(毫秒)
        uint32_t m_threadID = 0;              // 线程id
        uint32_t m_fiberID = 0;               // 协程id
        std::stringstream m_stringstream;     // string流
        Level m_level;                        // level
        std::shared_ptr<Logger> m_logger_ptr; // loggerptr
        std::string m_threadname;             // 线程名称
    }; // LogEvent

    // 限定作用域枚举支持比较运算符
    enum class Level
    {
        NOKNOW = 0, // noknow
        DEBUG = 1,  // debug
        INFO = 2,   // info
        WARN = 3,   // warn
        ERROR = 4,  // error
        FATAL = 5,  // fatal
    }; // Lervel

    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::LogEventPtr e);
        ~LogEventWrap();
        LogEvent::LogEventPtr getEvent() const { return m_event_ptr; }
        std::stringstream &getstringstream();

    private:
        LogEvent::LogEventPtr m_event_ptr;
    };

    // 输出的日志格式
    class LogFormatter
    {
    public:
        using LogFormatterPtr = std::shared_ptr<LogFormatter>;
        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event);
        const std::string getPattern() const { return m_pattern; }
        bool GerHasError() const
        {
            return m_HasError.load(std::memory_order_acquire);
        }

    public:
        class FormatterItem
        {
        public:
            using FormatterItemPtr = std::shared_ptr<FormatterItem>;
            virtual ~FormatterItem() = default;
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) = 0;
        };

    private:
        void init(); // 做pattern的解析
    private:
        std::string m_pattern; // 一旦从构造函数加载好不会再去做修改
        std::vector<FormatterItem::FormatterItemPtr> m_items;
        std::atomic<bool> m_HasError = false;
    }; // LogFormatter

    // 日志输出目的地
    class LogAppender
    {
        friend class Logger;

    public:
        using LogAppenderPtr = std::shared_ptr<LogAppender>;
        using MutexType = blue::SpinLockMutex;
        LogAppender() = default;
        virtual ~LogAppender() = default;
        virtual std::string toyamlString() = 0;
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) = 0;
        LogFormatter::LogFormatterPtr get_formatter() const;
        void set_formatter(LogFormatter::LogFormatterPtr formatter);
        Level getLevel() const { return m_level.load(std::memory_order_acquire); }
        void setLevel(Level level) { m_level.store(level, std::memory_order_release); }

        const bool gatHasformatter() const { return m_hasformatter; }

    protected:
        mutable MutexType m_mutex;
        std::atomic<Level> m_level = Level::DEBUG;
        LogFormatter::LogFormatterPtr m_formatter;
        std::atomic<bool> m_hasformatter = false;
    }; // LogAppender

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;

    public:
        using LoggerPtr = std::shared_ptr<Logger>;
        using MutexType = blue::MRWmutex;
        void Log(Level level, LogEvent::LogEventPtr event);
        Logger(const std::string &name = "root");
        void debug(LogEvent::LogEventPtr event);
        void info(LogEvent::LogEventPtr event);
        void warn(LogEvent::LogEventPtr event);
        void error(LogEvent::LogEventPtr event);
        void fatal(LogEvent::LogEventPtr event);

        std::string toyamlString();

        void addAppender(LogAppender::LogAppenderPtr Appender);
        void delAppender(LogAppender::LogAppenderPtr Appender);
        void ClearAppender() noexcept;

        Level get_level() const { return m_level; }
        void set_level(Level val) { m_level = val; }
        const std::string getname() const { return m_name; }
        void SetFormatter(LogFormatter::LogFormatterPtr rhs);
        void SetFormatter(const std::string &rhs);
        LogFormatter::LogFormatterPtr GetFormatter() const;

    private:
        mutable MutexType m_mutex;                          // 互斥变量
        std::string m_name;                                 // 日志名称
        std::list<LogAppender::LogAppenderPtr> m_Appenders; // Appender列表
        std::atomic<Level> m_level;                         // 日志级别
        LogFormatter::LogFormatterPtr m_formatter;          // 输出日志格式
        Logger::LoggerPtr m_root;                           // root
    }; // Logger

    // 输出到控制台的appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        using StdoutLogAppenderPtr = std::shared_ptr<StdoutLogAppender>;
        StdoutLogAppender() = default;
        ~StdoutLogAppender() = default;
        virtual std::string toyamlString() override;
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        std::string m_name = "console"; // 输出目的名称(控制台)
    }; // StdoutLogAppender

    // 输出到文件的appender
    class FileoutLogAppender : public LogAppender
    {
    public:
        ;
        using FileoutLogAppenderPtr = std::shared_ptr<FileoutLogAppender>;
        FileoutLogAppender() = default;
        ~FileoutLogAppender();
        FileoutLogAppender(const std::string &filename);
        void reopen(); // 重新打开
        void clear();  // 关闭
        // 虚函数，实现了一系列将event事件和日志相关信息写入文件的继承方法
        virtual std::string toyamlString() override;
        virtual void log(std::shared_ptr<Logger> logger_ptr, Level level, LogEvent::LogEventPtr event) override;

    private:
        std::string m_filename;               // 文件名
        std::ofstream m_filestream;           // 文件输出流
        std::string m_name = "file";          // 输出目的名称(文件)
        std::atomic<uint64_t> m_lasttime = 0; // 文件最新时间
    }; // FileoutLogAppender

    // logger管理类,生成root(默认name = "root",具有formatter格式,\
通过logger的Appender方法来给没有formatter的Appender设置formatter)
    class LoggerManager
    {
    public:
        using MutexType = blue::MRWmutex;
        // 构造
        LoggerManager();

        // 按照名称获取logger
        Logger::LoggerPtr GetLogger(const std::string &name); // 通过名字查找获取logger

        // 获取root
        Logger::LoggerPtr GetRoot() const { return m_root; } // 获取root

        // 初始化
        void init();

        std::string toyamlString();

    private:
        mutable MutexType m_mutex;
        std::map<std::string, Logger::LoggerPtr> m_logger;
        Logger::LoggerPtr m_root;
    };

    using LoggerMgr = blue::SingleTon<LoggerManager>;

    // 为枚举类型写一个枚举类模板
    template <typename Enum>
    class EnumTraits
    {
        static_assert(std::is_enum_v<Enum>, "必须为枚举类型");
    };
    template <>
    class EnumTraits<Level>
    {
    public:
        // 两种静态方法,支持序列化和反序列化,在读取yaml文件时配置起来方便

        // 从level -> string
        static std::string getlevelstring(Level level)
        {
            switch (level)
            {
#define getstring(name) \
    case Level::name:   \
        return #name;
                getstring(DEBUG);
                getstring(INFO);
                getstring(WARN);
                getstring(ERROR);
                getstring(FATAL);
#undef getstring
            }
            return "NOKNOW";
        }

        // 从string -> level
        static blue::Level getstringlevel(const std::string &val)
        {
#define getlever(name, str)          \
    if (val == #str || val == #name) \
        return Level::name;
            getlever(DEBUG, debug);
            getlever(INFO, info);
            getlever(WARN, warn);
            getlever(ERROR, error);
            getlever(FATAL, fatal);
#undef getlever
            return Level::NOKNOW;
        }

        // 对<<进行重载,支持直接输出enum class
        friend std::ostream &operator<<(std::ostream &os, Level level);
    };
}

#endif // __BLUE_LOG_H__