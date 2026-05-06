#ifndef __BLUE_HTTPREQUEST_H__
#define __BLUE_HTTPREQUEST_H__
#include <string>
#include <unordered_map>
#include <memory>
namespace blue
{
    enum class HTTP_CODE
    {
        NO_QUESTION,           // 请求不完整,还需要继续读取客户信息
        GET_QUESTION,          // 获得完整请求
        BAD_QUESTION,          // 请求格式错误
        INTERNAL_ERROR,        // 内部错误
        GET_QUESTION_WITH_BODY // 获得完整的请求带有body字段
    };
    // 当前状态机的行状态
    enum class LINE_STATUS
    {
        LINE_OK = 0, // 行完成
        LINE_BAD,    // 行出错
        LINE_OPEN    // 行数据不完整
    };
    // 状态机转移的状态
    enum class CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0, // 正在分析请求行
        CHECK_STATE_HEADER,          // 正在分析头部
        CHECK_STATE_BODY             // 正在分析body
    };
    struct HttpMessage
    {
        std::string m_method;                                  // 请求方法
        std::string m_url;                                     // url
        std::string m_version;                                 // version
        std::unordered_map<std::string, std::string> m_header; // header
        std::string m_body;                                    // body
        int m_Contentlength;                                   // 文本长度
        std::string m_ContentType;                             // 文本类型
    };
    class HttpRequest
    {
    public:
        using HttpRequestPtr = std::shared_ptr<HttpRequest>;

    private:
        // 分析请求行
        HTTP_CODE parse_requestline(std::string &temp, CHECK_STATE &check);
        // 分析头部
        HTTP_CODE parse_header(std::string &temp, CHECK_STATE &check);
        // 每一行是否完整
        LINE_STATUS parse_line(std::string &temp, int &check_idx, int &end);
        HTTP_CODE parse_body(std::string &temp, int length);

    public:
        HttpRequest() = default;
        // 开始解析函数
        HTTP_CODE parse_start(std::string &temp,
                              int &check_idx, int &start_idx, int &end_idx);
        // 根据请求执行不同的处理
        
        // get message
        HttpMessage getMessage() const { return m_message; };

    private:
        HttpMessage m_message;
    };
}

#endif