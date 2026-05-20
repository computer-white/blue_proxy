#include <regex>
#include <fcntl.h>
#include "blue/log.h"
#include "httpserver.h"
#include "httpconnection.h"
#ifdef USE_GUMBO
#include <gumbo.h>
#endif

namespace blue
{
    namespace http
    {
        static blue::Logger::LoggerPtr g_logger = BLUE_LOG_NAME("system");
            
        // static std::map<std::string, HttpConnectionPool::HttpConnectionPoolPtr> s_poolCache;
        // static Mmutex s_poolMutex;  
        /*
            // 需要处理的标签和属性
            struct ResourceRewrite {
                GumboTag tag;
                const char* attribute;
            };

            ResourceRewrite rewrites[] = {
                {GUMBO_TAG_LINK,   "href"},      // CSS
                {GUMBO_TAG_SCRIPT, "src"},       // JavaScript
                {GUMBO_TAG_IMG,    "src"},       // 图片
                {GUMBO_TAG_IMG,    "srcset"},    // 响应式图片
                {GUMBO_TAG_SOURCE, "src"},       // <source> 标签
                {GUMBO_TAG_SOURCE, "srcset"},    // <source> 的 srcset
                {GUMBO_TAG_IFRAME, "src"},       // 内嵌框架
                {GUMBO_TAG_EMBED,  "src"},       // 嵌入内容
                {GUMBO_TAG_OBJECT, "data"},      // 对象数据
                {GUMBO_TAG_VIDEO,  "src"},       // 视频
                {GUMBO_TAG_AUDIO,  "src"},       // 音频
                {GUMBO_TAG_INPUT,  "src"},       // <input type="image">
            };


            ┌─────────────────────────────────────────────────────────┐
            │                    需要修改的内容                         │
            ├──────────────────┬──────────────────────────────────────┤
            │  HTML 元素/属性   │  示例                                │
            ├──────────────────┼──────────────────────────────────────┤
            │ <a href="">       │  链接跳转                            │
            │ <link href="">    │  CSS 样式表                          │
            │ <script src="">   │  JavaScript                         │
            │ <img src="">      │  图片                                │
            │ <img srcset="">   │  响应式图片                          │
            │ <source src="">   │  媒体资源                            │
            │ <iframe src="">   │  内嵌页面                            │
            │ <embed src="">    │  嵌入内容                            │
            │ <object data="">  │  对象数据                            │
            │ <video src="">    │  视频                                │
            │ <audio src="">    │  音频                                │
            │ <form action="">  │  表单提交                            │
            │ <input src="">    │  image 类型的 input                  │
            │ <base href="">    │  基础 URL（整个页面的相对路径基准）   │
            ├──────────────────┼──────────────────────────────────────┤
            │ CSS 中            │  url()、@import                      │
            │ 内联样式/脚本      │  style=""、onclick="" 中的 URL       │
            │ HTTP 响应头       │  Location、Refresh、CSP              │
            └──────────────────┴──────────────────────────────────────┘
        */

        // CSS URL 重写函数
        static std::string _process_css(const std::string& css,
                                        const std::string& target,
                                        const std::string& proxy_path) 
        {
            std::string result = css;
            
            // 重写 url("//xxx") 和 url(//xxx)
            std::regex url_re(R"(url\(\s*["']?(//[^"')]+)["']?\s*\))");
            std::sregex_iterator begin(css.begin(), css.end(), url_re);
            std::sregex_iterator end;
            
            size_t last_pos = 0;
            for (auto it = begin; it != end; ++it)
            {
                const std::smatch& match = *it;
                std::string url = match[1].str();
                
                // 跳过 data: 等
                if (url.find("data:") == 0)
                {
                    continue;
                }
                
                std::string new_url = rewrite_url(url, target, proxy_path);
                
                // 输出匹配之前的部分
                result.append(css, last_pos, match.position() - last_pos);
                // 输出替换后的 url()
                result += "url(\"" + new_url + "\")";
                
                last_pos = match.position() + match.length();
            }
            
            // 输出剩余部分
            result.append(css, last_pos, css.length() - last_pos);
            
            return result;
        }

        // 修改cookie的domain
        static std::string fix_cookie_domain(const std::string& set_cookie) 
        {
            std::string result = set_cookie;
            
            // 1. 删除 domain=xxx 部分
            std::regex domain_re(R"(;\s*domain=[^;]*)", std::regex::icase);
            result = std::regex_replace(result, domain_re, "");
            
            // 2. 可选：设置 domain=localhost
            result += "; domain=localhost";
            
            // 3. 删除 Secure 标记（因为你是 http，不是 https）
            std::regex secure_re(R"(;\s*secure\b)", std::regex::icase);
            result = std::regex_replace(result, secure_re, "");
            
            return result;
        }

        // URL 解析辅助函数：从 target 参数提取域名
        static std::string extract_host_from_target(const std::string &target)
        {
            // target 格式: http://www.baidu.com 或 https://example.com/path
            auto urlptr = blue::Url::CreateUrl(target);
            return urlptr->getHost();
        }
        
        // 路径规格化
        static std::string normalize_path(const std::string& path) 
        {
            std::vector<std::string> parts;
            std::stringstream ss(path);
            std::string part;
            
            while (std::getline(ss, part, '/')) 
            {
                if (part.empty() || part == ".") 
                {
                    continue;
                } 
                else if (part == "..") 
                {
                    if (!parts.empty()) parts.pop_back();
                } 
                else 
                {
                    parts.push_back(part);
                }
            }
            
            std::string result;
            for (auto& p : parts) 
            {
                result += "/" + p;
            }
            return result.empty() ? "/" : result;
        }

        // URL 重写函数
        static std::string rewrite_url(const std::string &url,
                                       const std::string &target,
                                       const std::string &proxy_path)
        {
            if (url.empty())
                return url;

            // 不处理特殊协议和锚点
            if (url.find("javascript:") == 0 ||
                url.find("data:") == 0 ||
                url.find("mailto:") == 0 ||
                url.find("tel:") == 0 ||
                url.find("#") == 0)
            {
                return url;
            }

            auto target_url = blue::Url::CreateUrl(target);
            if (!target_url)
                return url;

            // 情况1：绝对 URL（http:// 或 https://）
            if (url.find("http://") != std::string::npos 
            || url.find("https://") != std::string::npos)
            {
                return proxy_path + "/" + url;
            }

            // 情况2：协议相对 URL（//example.com/path）
            if (url.size() >= 2 && url[0] == '/' && url[1] == '/')
            {
                return proxy_path + "/" + target_url->getScheme() + ":" + url;
            }

            // 构造基础 URL（scheme + authority）
            std::string base = target_url->getScheme() + "://" + target_url->getAuthority();

            // 情况3：根相对路径（/path）
            if (url[0] == '/')
            {
                return proxy_path + "/" + base + url;
            }

            // 情况4：相对路径（需要拼接当前路径的目录部分）
            std::string current_path = target_url->getPath();
            // 拼接：base + 当前目录 + "/" + 相对路径
            std::string resolved_path = current_path + "/" + url;
            /*
                <a href="../home">     →    /home       →    /blue/http://www.baidu.com/home
                <a href="../../top">   →    /top        →    /blue/http://www.baidu.com/top
                <a href="./page">      →    /s/page     →    /blue/http://www.baidu.com/s/page
                <a href="images/a.png">→    /s/images/a.png → /blue/http://www.baidu.com/s/images/a.png
            */
            resolved_path = normalize_path(resolved_path);
            return proxy_path + "/" + base + resolved_path;
        }

        // HTML 属性值转义
        static std::string escape_html_attr(const std::string &value)
        {
            std::string result;
            result.reserve(value.size());
            for (char c : value)
            {
                switch (c)
                {
                case '&':
                    result += "&amp;";
                    break;
                case '"':
                    result += "&quot;";
                    break;
                case '<':
                    result += "&lt;";
                    break;
                case '>':
                    result += "&gt;";
                    break;
                default:
                    result += c;
                }
            }
            return result;
        }

        // HTML 文本转义
        static void escape_text(const char *text, std::stringstream &ss)
        {
            while (*text)
            {
                switch (*text)
                {
                case '&':
                    ss << "&amp;";
                    break;
                case '<':
                    ss << "&lt;";
                    break;
                case '>':
                    ss << "&gt;";
                    break;
                default:
                    ss << *text;
                }
                text++;
            }
        }

        // 判断是否是 void 元素（HTML5 规范）
        static bool is_void_element(GumboTag tag)
        {
            switch (tag)
            {
            case GUMBO_TAG_AREA:
            case GUMBO_TAG_BASE:
            case GUMBO_TAG_BR:
            case GUMBO_TAG_COL:
            case GUMBO_TAG_EMBED:
            case GUMBO_TAG_HR:
            case GUMBO_TAG_IMG:
            case GUMBO_TAG_INPUT:
            case GUMBO_TAG_LINK:
            case GUMBO_TAG_META:
            case GUMBO_TAG_PARAM:
            case GUMBO_TAG_SOURCE:
            case GUMBO_TAG_TRACK:
            case GUMBO_TAG_WBR:
                return true;
            default:
                return false;
            }
        }

        // 判断属性是否需要重写
        static bool is_rewritable_attribute(GumboTag tag, const std::string &attr_name)
        {
            switch (tag)
            {
            case GUMBO_TAG_A:
            case GUMBO_TAG_LINK:
            case GUMBO_TAG_BASE:
                return attr_name == "href";

            case GUMBO_TAG_IMG:
            case GUMBO_TAG_SCRIPT:
            case GUMBO_TAG_IFRAME:
            case GUMBO_TAG_EMBED:
            case GUMBO_TAG_VIDEO:
            case GUMBO_TAG_AUDIO:
            case GUMBO_TAG_INPUT: // <input type="image" src="...">
                return attr_name == "src";

            case GUMBO_TAG_FORM:
                return attr_name == "action";

            case GUMBO_TAG_OBJECT:
                return attr_name == "data";

            case GUMBO_TAG_SOURCE:
                return attr_name == "src" || attr_name == "srcset";

            default:
                return false;
            }
        }

        // 处理 srcset 属性（可能包含多个 URL 和描述符）
        static std::string rewrite_srcset(const std::string &srcset,
                                          const std::string &target,
                                          const std::string &proxy_path)
        {
            std::stringstream result;
            size_t pos = 0;
            const size_t len = srcset.length();

            while (pos < len)
            {
                // 跳过空白
                while (pos < len && (srcset[pos] == ' ' || srcset[pos] == '\t'))
                {
                    result << srcset[pos];
                    pos++;
                }

                if (pos >= len)
                    break;

                // 找到 URL 结束位置（空白或逗号）
                size_t url_start = pos;
                while (pos < len && srcset[pos] != ',' && srcset[pos] != ' ' && srcset[pos] != '\t')
                {
                    pos++;
                }

                if (pos > url_start)
                {
                    std::string url_part = srcset.substr(url_start, pos - url_start);
                    result << rewrite_url(url_part, target, proxy_path);
                }

                // 输出空白和描述符（如 "480w"）
                while (pos < len && srcset[pos] != ',')
                {
                    result << srcset[pos];
                    pos++;
                }

                // 输出逗号
                if (pos < len && srcset[pos] == ',')
                {
                    result << ',';
                    pos++;
                }
            }

            return result.str();
        }

        // 序列化属性（并重写需要的 URL）
        static void serialize_attributes(const GumboVector *attributes,
                                         std::stringstream &ss,
                                         const std::string &target,
                                         const std::string &proxy_path,
                                         GumboTag tag)
        {
            for (unsigned int i = 0; i < attributes->length; i++)
            {
                auto *attr = static_cast<GumboAttribute *>(attributes->data[i]);
                std::string attr_name(attr->name);
                std::string attr_value(attr->value);

                // 判断是否需要重写
                if (is_rewritable_attribute(tag, attr_name))
                {
                    BLUE_LOG_WARN(g_logger) << "Rewriting: tag=" << (int)tag 
                            << " attr=" << attr_name 
                            << " value=" << attr_value;
                    if (attr_name == "srcset")
                    {
                        attr_value = rewrite_srcset(attr_value, target, proxy_path);
                    }
                    else
                    {
                        attr_value = rewrite_url(attr_value, target, proxy_path);
                    }
                }

                ss << " " << attr->name << "=\"" << escape_html_attr(attr_value) << "\"";
            }
        }

        // 递归序列化 DOM 节点
        static void serialize_node(GumboNode *node,
                                   std::stringstream &ss,
                                   const std::string &target,
                                   const std::string &proxy_path,
                                   bool base_injected = false)
        {
            if (!node)
                return;

            switch (node->type)
            {
            case GUMBO_NODE_DOCUMENT:
            {
                const GumboVector *children = &node->v.document.children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                   ss, target, proxy_path, base_injected);
                }
                break;
            }

            case GUMBO_NODE_ELEMENT:
            {
                GumboElement *element = &node->v.element;
                const char *tag_name = gumbo_normalized_tagname(element->tag);

                // ✅ 跳过 CSP 相关的 <meta> 标签
                if (element->tag == GUMBO_TAG_META)
                {
                    GumboAttribute* http_equiv = gumbo_get_attribute(&element->attributes, "http-equiv");
                    if (http_equiv && strcasecmp(http_equiv->value, "Content-Security-Policy") == 0)
                    {
                        // 跳过这个 meta 标签，不输出
                        break;  // 直接跳出，不序列化
                    }
                }

                ss << "<" << tag_name;
                serialize_attributes(&element->attributes, ss, target, proxy_path, element->tag);

                // 在 <head> 内注入 <base> 和 JS 拦截器（只注入一次）
                if (element->tag == GUMBO_TAG_HEAD && !base_injected)
                {
                    ss << ">";
                    ss << "<base href=\"" << proxy_path << "/" << target << "\">";
                    
                    // ✅ 注入 JS 拦截器
                    auto target_url = blue::Url::CreateUrl(target);
                    if (target_url)
                    {
                        std::string authority = target_url->getScheme() + "://" + target_url->getAuthority();
                        
                        ss << "<script>"
                        << "(function(){"
                        << "var p='" << proxy_path << "/" << authority << "';"
                        << "var origOpen=XMLHttpRequest.prototype.open;"
                        << "XMLHttpRequest.prototype.open=function(m,u){"
                        << "if(u.indexOf('/')===0&&u.indexOf('" << proxy_path << "/')!==0)u=p+u;"
                        << "origOpen.call(this,m,u);"
                        << "};"
                        << "var origFetch=window.fetch;"
                        << "window.fetch=function(u,o){"
                        << "if(typeof u==='string'&&u.indexOf('/')===0&&u.indexOf('" << proxy_path << "/')!==0)u=p+u;"
                        << "return origFetch.call(this,u,o);"
                        << "};"
                        << "})();"
                        << "</script>";
                    }
                    
                    base_injected = true;

                    const GumboVector *children = &element->children;
                    for (unsigned int i = 0; i < children->length; i++)
                    {
                        serialize_node(static_cast<GumboNode *>(children->data[i]),
                                    ss, target, proxy_path, base_injected);
                    }

                    ss << "</head>";
                    return;
                }

                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                ss, target, proxy_path, base_injected);
                }

                if (!is_void_element(element->tag))
                {
                    ss << "</" << tag_name << ">";
                }
                break;
            }

            case GUMBO_NODE_TEXT:
                escape_text(node->v.text.text, ss);
                break;

            case GUMBO_NODE_CDATA:
                ss << "<![CDATA[" << node->v.text.text << "]]>";
                break;

            case GUMBO_NODE_COMMENT:
                ss << "<!--" << node->v.text.text << "-->";
                break;

            case GUMBO_NODE_WHITESPACE:
                ss << node->v.text.text;
                break;

            case GUMBO_NODE_TEMPLATE:
            {
                GumboElement *element = &node->v.element;
                ss << "<template";
                serialize_attributes(&element->attributes, ss, target, proxy_path, GUMBO_TAG_TEMPLATE);
                ss << ">";

                const GumboVector *children = &element->children;
                for (unsigned int i = 0; i < children->length; i++)
                {
                    serialize_node(static_cast<GumboNode *>(children->data[i]),
                                   ss, target, proxy_path, base_injected);
                }

                ss << "</template>";
                break;
            }
            }
        }

        // 解析主函数
        static std::string _process_html(const std::string &html,
                                         const std::string &target,
                                         const std::string &proxy_path)
        {
            GumboOutput *output = gumbo_parse(html.c_str());
            if (!output)
            {
                return html; // 解析失败，返回原文
            }

            std::stringstream ss;
            serialize_node(output->root, ss, target, proxy_path);

            gumbo_destroy_output(&kGumboDefaultOptions, output);

            return ss.str();
        }

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
            auto remoteAddress = std::dynamic_pointer_cast<IPAddress>(sock->getRemoteAddress());
            auto localAddress = std::dynamic_pointer_cast<IPAddress>(sock->getLocalAddress());
            _setIpAndPort(remoteAddress, m_remoteIP, m_remotePort);
            _setIpAndPort(localAddress, m_localIp, m_localPort);
            
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << remoteAddress->toString() << " ip : " << m_remoteIP << " port : " << m_remotePort;
            // BLUE_LOG_INFO(g_logger) << "remoteaddress : " << localAddress->toString()  << " ip : " << m_localIp  << " port : " << m_localPort;

            // ===== 检测 TLS，如果是就替换为 SSLSocket =====
            char first_byte;
            int peek_ret = ::recv(sock->getSocketfd(), &first_byte, 1, MSG_PEEK | MSG_DONTWAIT);
            
            std::shared_ptr<HttpSession> session;
            if (peek_ret == 1 && first_byte == 0x16) 
            {
                // HTTPS：创建 SSLSocket，握手，然后直接当 SocketStream 用
                auto ssl_sock = std::make_shared<SSLSocket>(sock,true,true);
                if (!ssl_sock->isValid()) 
                {
                    BLUE_LOG_ERROR(g_logger) << "SSLSocket creation failed (cert not found?)";
                    sock->close();
                    return;
                }
                if (!ssl_sock->handshake()) 
                {
                    BLUE_LOG_ERROR(g_logger) << "SSL handshake failed";
                    sock->close();
                    return;
                }
                session = std::make_shared<HttpSession>(ssl_sock);
            } else 
            {
                auto stream = std::make_shared<SocketStream>(sock);
                session = std::make_shared<HttpSession>(stream);
            }

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
                // BLUE_LOG_INFO(g_logger) << "host : " << requestPtr->getHeader("Host");
                auto responsePtr = std::make_shared<HttpResponse>(requestPtr->getVersion(), (requestPtr->isKeepAlive() || temkeepAlive));
                temkeepAlive = (requestPtr->isKeepAlive() || temkeepAlive);

                if (requestPtr->getMethod() == HttpMethod::CONNECT)
                {
                    BLUE_LOG_INFO(g_logger) << "CONNECT: " << requestPtr->getPath();
                    _handleConnect(sock, requestPtr);
                    return;  // CONNECT 不进入普通处理流程
                }
                std::string path = requestPtr->getPath();
                std::string host = requestPtr->getHeader("Host");
                std::string targeturl;
                std::string target_param = requestPtr->getParam("target", "");
                if (!target_param.empty())
                {
                    targeturl = target_param;
                    // BLUE_LOG_INFO(g_logger) << "Old format target: " << targeturl;
                    _forwardRequest(requestPtr, responsePtr, targeturl, false);
                }
                // ===== 正向代理 =====
                // 正向代理：Host 不是 localhost，path 就是目标路径
                else if (!host.empty() && 
                        host.find("localhost") == std::string::npos && 
                        host.find("127.0.0.1") == std::string::npos)
                {
                    targeturl = "http://" + host + path;
                    std::string query = requestPtr->getQuery();
                    if (!query.empty()) targeturl += "?" + query;
                    
                    // BLUE_LOG_INFO(g_logger) << "Forward proxy: " << targeturl;
                    _forwardRequest(requestPtr, responsePtr, targeturl, true);
                }
                // ===== 反向代理（路径前缀模式）=====
                else if (path.find("/blue/") == 0)
                {
                    size_t scheme_pos = path.find("http://");
                    if (scheme_pos == std::string::npos)
                        scheme_pos = path.find("https://");
                    if (scheme_pos != std::string::npos)
                    {
                        targeturl = path.substr(scheme_pos);
                        // BLUE_LOG_INFO(g_logger) << "Reverse proxy: " << targeturl;
                        _forwardRequest(requestPtr, responsePtr, targeturl, false);
                    }
                    else
                    {
                        // BLUE_LOG_INFO(g_logger) << "no target";
                        m_dispatch->handle(requestPtr, responsePtr, session);
                    }
                }
                else
                {
                    // BLUE_LOG_INFO(g_logger) << "no target";
                    m_dispatch->handle(requestPtr, responsePtr, session);
                }
                // 自动补全缺失的响应头
                if (responsePtr->getHeader("Content-Type").empty())
                {
                    responsePtr->setHeader("Content-Type", "text/html; charset=utf-8");
                }
                if (responsePtr->getHeader("Content-Length").empty())
                {
                    responsePtr->setHeader("Content-Length", std::to_string(responsePtr->getBody().size()));
                }
                session->sendResponse(responsePtr, requestPtr);
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

        template <typename T>
        void HttpServer<T>::_setIpAndPort(blue::IPAddress::IPAddressPtr address, std::string &ip, std::string &port)
        {
            if (address->getFamily() == AF_INET)
            {
                ip = std::dynamic_pointer_cast<IPv4Address>(address)->getIp();
            }
            else if (address->getFamily() == AF_INET6)
            {
                ip = std::dynamic_pointer_cast<IPv6Address>(address)->getIp();
            }
            else
            {
                ip = std::dynamic_pointer_cast<UnknowAddress>(address)->getIp();
            }
            port = std::to_string(address->getPort());
        }

        template <typename T>
        void HttpServer<T>::_forwardRequest(HttpRequest::HttpRequestPtr request,
                                            HttpResponse::HttpResponsePtr response, 
                                            const std::string &targeturl,
                                            bool isForwardProxy)
        {

            // 设置下游ip(remoteip)
            std::string xxf = request->getHeader("X-Forwarded-For");
            if (!xxf.empty())
            {
                xxf += ", ";
            }
            xxf += m_remoteIP;
            request->setHeader("X-Forwarded-For", xxf);
            auto UrlPtr = blue::Url::CreateUrl(targeturl);
            if (!UrlPtr)
            {
                response->setStatus(blue::http::HttpStatus::BAD_REQUEST);
                response->setBody("invalid target url");
                return;
            }
            // else
            // {
            //     BLUE_LOG_WARN(g_logger) << "DoRequest URL: " << targeturl 
            //                 << " path: " << UrlPtr->getPath()
            //                 << " host: " << UrlPtr->getHost();
            // }

            // 代理前缀
            std::string proxy_path = "/blue";
            // BLUE_LOG_INFO(g_logger) << "Proxy path: " << proxy_path
            //                         << ", target: " << targeturl;

            // 将客户端的请求中的header拿出来作为我们发给targeturl的header
            std::map<std::string,std::string> headers;
            _setHeaders(request,headers);

            // BLUE_LOG_WARN(g_logger) << "=== Request to target ===";
            // BLUE_LOG_WARN(g_logger) << "URL: " << targeturl;
            // BLUE_LOG_WARN(g_logger) << "Method: " << (int)request->getMethod();
            // BLUE_LOG_WARN(g_logger) << "Headers:";
            // for (auto& h : headers) {
            //     BLUE_LOG_WARN(g_logger) << "  " << h.first << ": " << h.second;
            // }
            // BLUE_LOG_WARN(g_logger) << "Body size: " << request->getBody().size();

            // 同步转发给上游(非连接池)
            // auto result = blue::http::HttpConnection::DoRequest(
            //     request->getMethod(), targeturl, 5000, headers, request->getBody());

            std::string poolKey = UrlPtr->getScheme() + "://" + UrlPtr->getHost() + ":" + std::to_string(UrlPtr->getPort());
            // 全局连接池缓存
            static std::map<std::string, HttpConnectionPool::HttpConnectionPoolPtr> s_pools;
            static http::HttpConnectionPool::MmutexType s_poolMutex;
            HttpConnectionPool::HttpConnectionPoolPtr pool;
            {
                blue::http::HttpConnectionPool::MmutexType lock(s_poolMutex);
                auto it = s_pools.find(poolKey);
                if (it == s_pools.end())
                {
                    pool = std::make_shared<HttpConnectionPool>(
                        UrlPtr->getHost(), "", UrlPtr->getPort(), 10, 60000, 100);
                    s_pools[poolKey] = pool;
                }
                else
                {
                    pool = it->second;
                }
            }
            // 用连接池发请求（复用连接）
            auto result = pool->doRequest(request->getMethod(), UrlPtr, 5000, headers, request->getBody());

            if (result->result == (int)blue::http::HttpResult::ResultStatus::OK && result->response)
            {
                for (auto &header : result->response->getHeaders())
                {
                    response->setHeader(header.first, header.second);
                }
                response->delHeader("Transfer-Encoding"); // 和 Content-Length 互斥
                int status = (int)result->response->getStatus();
                if (isForwardProxy)
                {
                    response->setBody(result->response->getBody());
                    response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                    response->setStatus((blue::http::HttpStatus)status);
                    return;
                }

                if (status == 301 || status == 302 || status == 307 || status == 308)
                {
                    std::string location = result->response->getHeader("Location");
                    if (!location.empty())
                    {
                        // 重定向到targeturl成循环不允许
                        // TODO
                        if (location == targeturl || location == targeturl + "/") 
                        {
                            BLUE_LOG_WARN(g_logger) << "Redirect loop detected: " << location;
                            response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                            response->setBody("Redirect loop detected");
                            return;
                        }

                        // 把重定向 URL 也改成代理模式
                        std::string new_location = proxy_path + "/" + location;
                        response->setHeader("Location", new_location);
                        response->delHeader("Content-Length"); // 重定向没有 body
                        response->setBody("");                 // 清空 body
                        response->setStatus((blue::http::HttpStatus)status);
                        // BLUE_LOG_WARN(g_logger) << "Rewrite redirect: "
                        //                         << location << " → " << new_location;
                        return;
                    }
                }

                std::string set_cookie = result->response->getHeader("Set-Cookie");
                if (!set_cookie.empty())
                {
                    std::string fixed_cookie = fix_cookie_domain(set_cookie);
                    response->setHeader("Set-Cookie", fixed_cookie);
                }

                std::string ContentType = result->response->getHeader("Content-Type");
                if (ContentType.find("text/html") != std::string::npos)
                {
                    auto original_html = result->response->getBody();
                    auto modified_body = _process_html(original_html, targeturl, proxy_path);
                    response->setBody(modified_body);
                    response->setStatus((blue::http::HttpStatus)status);
                    // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                    response->setHeader("Content-Length", std::to_string(modified_body.size()));
                    // BLUE_LOG_INFO(g_logger) << "HTML modified, original: "
                    //                         << original_html.size() << "B, modified: "
                    //                         << modified_body.size() << "B";
                }
                else if (ContentType.find("text/css") != std::string::npos)
                {
                    auto original_css = result->response->getBody();
                    auto modified_css = _process_css(original_css, targeturl, proxy_path);
                    response->setBody(modified_css);
                    response->setStatus((blue::http::HttpStatus)status);
                    // 设置的修改后未压缩大小,稍后在sendresponse时会设置压缩后的length
                    response->setHeader("Content-Length", std::to_string(modified_css.size()));
                }
                else
                {
                    response->setBody(result->response->getBody());
                    response->setHeader("Content-Length", std::to_string(result->response->getBody().size()));
                    response->setStatus((blue::http::HttpStatus)status);
                }
                if (!ContentType.empty())
                {
                    response->setHeader("Content-Type", ContentType);
                }
            }
            else
            {
                response->setStatus(blue::http::HttpStatus::BAD_GATEWAY);
                response->setBody("forward failed: " + result->error);
            }
        }

        template <typename T>
        void HttpServer<T>::_handleConnect(MSocket::MSocketPtr sock,
                                            HttpRequest::HttpRequestPtr request)
        {
            // BLUE_LOG_WARN(g_logger) << "CONNECT path: " << request->getPath()
            //             << " host: " << request->getHeader("Host")
            //             << " query: " << request->getQuery();
            // CONNECT 的 path 是 host:port
            std::string host_port = request->getHeader("Host");
            if (host_port.empty())
            {
                host_port = request->getPath();
            }
            
            // 解析 host 和 port
            std::string host = host_port;
            uint16_t port = 443;
            size_t colon = host_port.find(':');
            if (colon != std::string::npos)
            {
                host = host_port.substr(0, colon);
                port = std::stoi(host_port.substr(colon + 1));
            }
            
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel to " << host << ":" << port;

            // 连接目标站
            auto addr = blue::Address::LookupAnyIpAddress(host);
            if (!addr) return;
            addr->setPort(port);
            
            auto remote_sock = blue::MSocket::CreateTcp(addr);
            if (!remote_sock->connect(addr)) return;
            
            int client_fd = sock->getSocketfd();
            int remote_fd = remote_sock->getSocketfd();
            // 返回 200 给浏览器，表示隧道建立
            std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
            ::send(client_fd, ok.c_str(), ok.size(), 0);
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel established";
            
            
            char buf[16384];
            fd_set fds;
            
            while (true)
            {
                FD_ZERO(&fds);
                FD_SET(client_fd, &fds);
                FD_SET(remote_fd, &fds);
                
                int max_fd = std::max(client_fd, remote_fd);
                struct timeval tv = {5, 0};
                
                int ret = select(max_fd + 1, &fds, nullptr, nullptr, nullptr);
                if (ret <= 0)
                {
                    BLUE_LOG_WARN(g_logger) << "CONNECT select timeout or error: " << ret;
                    break;
                }
                
                if (FD_ISSET(client_fd, &fds))
                {
                    ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
                    if (n <= 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "CONNECT client closed, n=" << n;
                        break;
                    }
                    ssize_t sent = ::send(remote_fd, buf, n, 0);
                    if (sent <= 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "CONNECT remote send failed: " << sent 
                            << " errno=" << errno << " " << strerror(errno);
                        break;
                    }
                }
                
                if (FD_ISSET(remote_fd, &fds))
                {
                    ssize_t n = ::recv(remote_fd, buf, sizeof(buf), 0);
                    if (n <= 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "CONNECT remote closed, n=" << n;
                        break;
                    }
                    ssize_t sent = ::send(client_fd, buf, n, 0);
                    if (sent <= 0)
                    {
                        BLUE_LOG_INFO(g_logger) << "CONNECT client send failed: " << sent;
                        break;
                    }
                }
            }
            
            remote_sock->close();
            BLUE_LOG_INFO(g_logger) << "CONNECT tunnel closed";
        }


        template <typename T>
        void HttpServer<T>::_setHeaders(HttpRequest::HttpRequestPtr request, std::map<std::string,std::string> &headers)
        {
            for (auto &[key,val] : request->getHeaders())
            {
                // 跳过 Host（让 DoRequest 自己设置）
                if (strcasecmp(key.c_str(), "host") == 0)
                    continue;
                // 跳过 Referer（指向 localhost，百度会拒绝）
                if (strcasecmp(key.c_str(), "referer") == 0)
                    continue;
                // 跳过 Sec-Fetch-* 头（导致跨域问题）
                if (strncasecmp(key.c_str(), "sec-", 4) == 0)
                    continue;
                headers[key] = val;
            }
        }

        template class blue::http::HttpServer<int>;
        template class blue::http::HttpServer<timeval>;
    }
}