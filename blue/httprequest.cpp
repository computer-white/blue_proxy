#include "httprequest.h"
namespace blue
{
    HTTP_CODE HttpRequest::parse_start(std::string &buf, int &check_idx,
                                       int &start_idx, int &end_idx)
    {
        LINE_STATUS linestates = blue::LINE_STATUS::LINE_OK; // 当前行状态
        HTTP_CODE endcode = blue::HTTP_CODE::NO_QUESTION;    // 请求分析结束状态
        CHECK_STATE chec = blue::CHECK_STATE::CHECK_STATE_REQUESTLINE;
        CHECK_STATE &checkstate = chec;
        while ((linestates = parse_line(buf, check_idx, end_idx)) == blue::LINE_STATUS::LINE_OK)
        {
            std::string temp = buf.substr(start_idx);
            start_idx = check_idx;
            switch (checkstate)
            {
            case blue::CHECK_STATE::CHECK_STATE_REQUESTLINE:
            {
                endcode = parse_requestline(temp, checkstate);
                if (endcode == blue::HTTP_CODE::BAD_QUESTION)
                {
                    return blue::HTTP_CODE::BAD_QUESTION;
                }
                break;
            }
            case blue::CHECK_STATE::CHECK_STATE_HEADER:
            {
                endcode = parse_header(temp, checkstate);
                if (endcode == blue::HTTP_CODE::BAD_QUESTION)
                {
                    return blue::HTTP_CODE::BAD_QUESTION;
                }
                else if (endcode == blue::HTTP_CODE::GET_QUESTION)
                {
                    return blue::HTTP_CODE::GET_QUESTION;
                }
                break;
            }
            case blue::CHECK_STATE::CHECK_STATE_BODY:
            {
                int length = temp.size();
                endcode = parse_body(temp, length);
                if (endcode == blue::HTTP_CODE::GET_QUESTION_WITH_BODY)
                {
                    return blue::HTTP_CODE::GET_QUESTION_WITH_BODY;
                }
                break;
            }
            default:
                return blue::HTTP_CODE::INTERNAL_ERROR;
                break;
            }
        }
        if (linestates == blue::LINE_STATUS::LINE_OPEN)
        {
            return blue::HTTP_CODE::NO_QUESTION;
        }
        else
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }
    }

    HTTP_CODE HttpRequest::parse_requestline(std::string &buf, CHECK_STATE &check)
    {
        // GET /api/users?page=1&limit=10 HTTP/1.1\r\n
        std::string requestline = buf.substr(0, buf.find("\r\n"));
        if (requestline.empty())
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }

        // method
        std::string method = requestline.substr(0, requestline.find_first_of(' '));
        if (method.empty())
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }

        // url
        std::string url = requestline.substr(requestline.find_first_of('/'));
        url = url.substr(0, url.find_first_of(' '));
        if (url.empty() || url[0] != '/')
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }

        // version
        std::string version = requestline.substr(requestline.find_last_of('H'));
        if (version.empty())
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }
        m_message.m_method = method;
        m_message.m_url = url;
        m_message.m_version = version;
        check = blue::CHECK_STATE::CHECK_STATE_HEADER;
        return blue::HTTP_CODE::NO_QUESTION;
    }

    HTTP_CODE HttpRequest::parse_header(std::string &buf, CHECK_STATE &check)
    {
        /*
            Host: www.example.com
            Content-Type: application/x-www-form-urlencoded
            Content-Length: 29
            User-Agent: Mozilla/5.0
            Accept: text/html,application/xhtml+xml
            Cookie: sessionid=abc123; user=zhangsan
        */
        // 头部信息结束
        if (buf.size() == 2 && buf[0] == '\r' && buf[1] == '\n')
        {
            if (m_message.m_header.count("Content-Length") > 0)
            {
                int m_content_length = std::stoi(m_message.m_header["Content-Length"]);
                if (m_content_length > 0)
                {
                    m_message.m_Contentlength = m_content_length;
                    auto it = m_message.m_header.find("Content-Type");
                    if (it != m_message.m_header.end())
                    {
                        m_message.m_ContentType = it->second;
                    }
                    check = blue::CHECK_STATE::CHECK_STATE_BODY;
                    return blue::HTTP_CODE::NO_QUESTION;
                }
            }
            return blue::HTTP_CODE::GET_QUESTION;
        }
        std::string key_val = buf.substr(0, buf.find("\r\t"));
        if (key_val.empty())
        {
            return blue::HTTP_CODE::BAD_QUESTION;
        }
        size_t colon_pos = key_val.find_first_of(':');
        if (colon_pos == std::string::npos)
            return blue::HTTP_CODE::BAD_QUESTION;

        std::string key = key_val.substr(0, colon_pos);
        std::string val = key_val.substr(colon_pos + 1);

        // 去除空白字符
        while (!key.empty() && key.back() == ' ')
            key.pop_back();
        while (!val.empty() && val.front() == ' ')
            val.erase(0, 1);
        while (!val.empty() && val.back() == ' ')
            val.pop_back();

        if (key.empty())
            return blue::HTTP_CODE::BAD_QUESTION;
        m_message.m_header[key] = val;

        return blue::HTTP_CODE::NO_QUESTION;
    }

    HTTP_CODE HttpRequest::parse_body(std::string &temp, int length)
    {
        int m_length = m_message.m_Contentlength;
        // length不完整
        if (m_length > length)
        {
            m_message.m_body.append(temp);
            return blue::HTTP_CODE::NO_QUESTION;
        }
        else if (m_length == length) // 完整的length
        {
            m_message.m_body = temp;
            return blue::HTTP_CODE::GET_QUESTION_WITH_BODY;
        }
    }

    LINE_STATUS HttpRequest::parse_line(std::string &buf, int &check_idx, int &end)
    {
        for (; check_idx < end; check_idx++)
        {
            char tem = buf[check_idx];
            if (tem == '\r')
            {
                if ((check_idx + 1) == end)
                {
                    return blue::LINE_STATUS::LINE_OPEN; // 没有读完
                }
                else if (buf[check_idx + 1] == '\n')
                {
                    // 保留\r\n作为每一行的判断
                    check_idx += 2;
                    return blue::LINE_STATUS::LINE_OK;
                }
                return blue::LINE_STATUS::LINE_BAD;
            }
            else if (tem == '\n') // 到这里前面一个必然不是'\r'所以直接返回LINE_BAD
            {
                return blue::LINE_STATUS::LINE_BAD;
            }
        }
        return blue::LINE_STATUS::LINE_OPEN;
    }

}