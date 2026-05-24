# Blue Proxy

基于 C++11/14/17 的 HTTP/HTTPS 代理服务器，支持正向代理、简单反向代理、HTML 动态重写、Redis 缓存、MySQL 日志、Docker 容器化部署。

关于日志，配置，线程，协程，协程调度，网络等等几乎所有的都是跟随sylar的教学跟写的，但其中也有一些我并未完全跟着sylar。
比如关于日志模块一些无锁编程，以及线程与协程模块的简单改写.但是主要思路都是跟随sylar写的，这里非常感谢sylar可以在哔站上
讲授有关c++服务器框架的内容
## 特性

- **多模式代理**
  - 正向代理（HTTP/HTTPS CONNECT 隧道）
  - 反向代理（路径前缀 `/blue/`）
  - WebSocket 代理（正向模式）
  - PAC 自动代理配置
- **http解析**：基于llhttp重新封装的解析模块
- **协程调度**：基于 ucontext 的用户态协程，支持数千并发
- **HTML/CSS 动态重写**：基于 Gumbo DOM 解析器，自动改写链接、注入 JS 拦截器
- **SSL/TLS**：OpenSSL 双向支持（服务端 + 客户端模式）
- **Redis 集成**：IP 限流（可配置）、响应缓存（正向/反向）
- **MySQL 集成**：连接池、请求日志、统计查询
- **连接池**：HTTP 连接池复用 + MySQL 连接池
- **Web 管理面板**：Dashboard / 请求日志（搜索+分页） / 连接池状态 / 配置查看 / 请求趋势图表
- **配置热更新**：ConfigVar 系统，修改配置无需重启
- **Docker 部署**：docker-compose 一键启动

## 快速开始

### Docker 一键部署

\`\`\`bash
docker compose up -d
\`\`\`

### 本地编译运行

\`\`\`bash
# 依赖
sudo apt install build-essential cmake libboost-all-dev libssl-dev zlib1g-dev \
  libmysqlclient-dev libhiredis-dev libgumbo-dev libidn2-dev ragel libyaml-cpp-dev

# 编译
cd build && cmake .. && make -j$(nproc)

# 启动 MySQL 和 Redis（Docker）
docker compose up -d mysql redis

# 启动代理
./bin/test_proxy
\`\`\`

## 使用方式

| 模式 | 访问方式 |
|------|---------|
| 正向代理 HTTP | 浏览器设置代理 `localhost:8020` |
| 正向代理 HTTPS | 浏览器设置代理 `localhost:8020`（CONNECT 隧道） |
| 反向代理 | `http://localhost:8020/blue/http://目标URL` |
| PAC 自动代理 | 浏览器配置 PAC：`http://localhost:8020/proxy.pac` |
| 管理面板 | `http://localhost:8020/admin` |

## 配置项

| 配置 | 默认值 | 说明 |
|------|--------|------|
| `proxy.rate_limit` | 100 | 每分钟限流阈值 |
| `proxy.cache_expire` | 60 | 缓存过期时间（秒） |
| `proxy.tunnel_timeout` | 1000 | 隧道 select 超时（毫秒） |
| `db.host` | localhost | 数据库主机 |
| `redis.host` | 127.0.0.1 | Redis 主机 |

## 主要模块详解
一些测试代码几乎全是ai写的

### 1. 日志模块 (`blue/log.h/cpp`)
- 基于 sylar 架构，在配置模块和线程模块加入后部分采用无锁实现
- 支持协程安全的日志输出
- 测试：`tests/test`

### 2. 配置模块 (`blue/config.h/cpp`)
- 参考 sylar 实现，部分使用无锁或减少锁粒度
- 支持配置热更新（ConfigVar + Listener 模式）
- 所有配置集中在 `configinit.cpp` 管理
- 测试：`tests/test_config`

### 3. 线程模块 (`blue/mthread.h/cpp`)
- 信号量：使用 C++11 `std::condition_variable` + `std::unique_lock`，替代 Linux 原生信号量 API
- 互斥锁/读写锁/自旋锁：保留 sylar 的 RAII 模板封装
- Mutex 底层使用 `std::mutex` 和 `std::shared_mutex`
- 加入 `try_lock` 支持
- 历史版本：`oldblue/mthread.h/cpp`
- 测试：`tests/test_thread`

### 4. 协程模块 (`blue/fiber.h/cpp`) & 调度器 (`blue/schedular.h/cpp`)
- 架构与 sylar 一致
- ucontext 上下文切换通过 `Context` 模块抽象（`blue/context.h/cpp`），便于日后替换引擎（如 boost::context）
- `IOManager` 基于 epoll 事件驱动
- 测试：`tests/test_fiber` / `tests/test_iomanager` / `tests/test_schedular`

### 5. 定时器 (`blue/timer.h/cpp`)
- 与 sylar 一致

### 6. 网络地址模块 (`blue/address.h/cpp`)
- 支持 IPv4/IPv6 地址解析
- 集成 Url 解析，自动处理 scheme 和端口
- 测试：`tests/test_address`

### 7. ByteArray 模块 (`blue/bytearray.h/cpp`)
- 将 sylar 的单链表改为双链表实现
- 支持分散读写（readv/writev 风格的 iovec）
- 测试：`tests/test_bytearray` / `tests/test_bytearrayBuffer`

### 8. Socket 模块
- **MSocket** (`blue/msocket.h/cpp`)：TCP socket 封装
- **MStream** (`blue/mstream.h/cpp`)：流式读写基类
- **SocketStream** (`blue/msocketstream.h/cpp`)：基于 ByteArray 的流式 socket 读写
- 参考 sylar 实现
- 测试：`tests/test_socket`

### 9. TCP 服务 (`blue/tcpServer.h/cpp`)
- 基于协程的 TCP 服务器
- 测试：`tests/test_tcpServer` / `examples/echoServer`

### 10. HTTP 协议 (`http/`)
- **http.cpp**：HTTP 请求/响应结构定义，加入 gzip 压缩/解压
- **httpParser.cpp**：使用 **llhttp** 替代 sylar 的解析库
  - 桥接模式：私有静态方法供 llhttp 回调，外部接口支持用户绑定回调
  - 支持 `on_UrlComplete`、`on_MessageComplete`、`on_Body` 等回调
  - URL 解析委托给 Ragel 生成的状态机（`blue/urlparser.cpp`）
  - 流式接收 body 数据
  - 请求和响应解析复用同一框架
  - 测试：`tests/test_httpParser`
- **httpconnection.cpp**：HTTP 连接管理
  - 支持连接池复用（`HttpConnectionPool`）
  - `DoRequest` 集成 `SSLSocket` 实现 HTTPS 客户端
  - 构造函数接受 `MStream` 智能指针，统一 HTTP/HTTPS 接口
  - 测试：`tests/test_httpconnection` / `tests/test_http_pool_stress`

### 11. SSL/TLS (`blue/sslsocket.h/cpp`)
- 继承 `SocketStream`，重写 read/write 为 SSL_read/SSL_write
- 双模式：服务端（加载证书 + `SSL_accept`）/ 客户端（`SSL_connect` + SNI）
- 集成到 `HttpConnection::DoRequest` 实现 HTTPS 请求
- 集成到 `handleClient` 实现 HTTPS 反向代理

### 12. 代理核心 (`http/httpserver.cpp`)
- 请求路由：CONNECT 隧道 → 本地路由 → 旧格式 → 正向代理 → 反向代理
- HTML 重写：Gumbo DOM 解析 + 正则 CSS 处理 + JS 拦截注入
- 重定向 Location 改写、Cookie Domain 修复
- CSP 头删除、Transfer-Encoding 冲突处理
- 路径前缀模式（`/blue/https://目标URL`）
- 测试：`tests/test_proxy` / `tests/test_proxy_full`

### 13. 数据库 (`blue/dbmanager.h/cpp`, `blue/mysqlpool.h/cpp`)
- MySQL 连接池：支持空闲连接复用、断线重连
- 请求日志自动记录到 `request_logs` 表
- 错误日志、连接池统计

### 14. Redis (`blue/redismanager.h/cpp`)
- 基于 hiredis 的 Redis 客户端
- IP 限流：滑动窗口计数器（INCR + EXPIRE）
- 响应缓存：正向/反向代理均支持

### 15. URL 解析模块 (`blue/url.h/cpp`, `blue/urlparser.cpp`)
- 基于 Ragel 状态机实现高性能 URL 解析（`blue/url_parser.rl` → `urlparser.cpp`）
- **完整支持中文 URL**：Unicode 主机名（如 `http://中文域名.com/路径`）自动编解码
- URL 组件拆分：scheme / userinfo / host / port / path / query / fragment
- 自动端口推断：HTTP 默认 80，HTTPS 默认 443
- IPv4/IPv6 地址识别（`HostType` 枚举）
- 双模式输出：`getAsciiURL()` 返回编码后 URL，`getUnicodeURL()` 返回解码后 URL
- 支持 `createAddress()` 直接创建网络地址对象
- 测试：`tests/test_url`

**API 一览：**

| 方法 | 说明 |
|------|------|
| `CreateUrl(url)` | 静态工厂方法，解析 URL 字符串 |
| `getScheme()` | 获取协议（http/https） |
| `getHost()` | 获取主机名（ASCII 编码形式） |
| `getUnicodeHost()` | 获取主机名（Unicode 解码形式） |
| `getPath()` | 获取路径（默认为 `/`） |
| `getQuery()` | 获取查询参数 |
| `getFragment()` | 获取片段标识符 |
| `getPort()` | 获取端口（自动推断默认端口） |
| `getAuthority()` | 获取 userinfo@host:port |
| `getAsciiURL()` | 获取完整 URL（编码形式） |
| `getUnicodeURL()` | 获取完整 URL（解码形式） |
| `createAddress()` | 根据 host 创建 `Address` 对象 |

## 压力测试

**测试环境**
- CPU：12th Gen Intel Core i5-12500H（8 核 16 线程，4.4GHz）
- 内存：8GB DDR5（WSL2 分配）
- 操作系统：Ubuntu 22.04 LTS（WSL2 on Windows 11）
- 内核：Linux 6.6.87.2-microsoft-standard-WSL2
- 编译器：g++ 11.4.0，C++11/14/17
- 并发模型：固定 16 工作线程 + 用户态协程（Fiber）
- 目标站：www.baidu.com

> **注**：测试在 WSL2 虚拟机 + `-O0` 调试模式下进行。开启 `-O2` 优化并在物理 Linux 环境下运行，QPS 预期提升 2-3 倍。1000 并发时受 WSL2 内存限制（8GB）及百度限流影响，失败率上升。

| 测试 | 协程数 | 总请求 | 成功 | 失败 | 耗时 | QPS | 平均延迟 | P50 | P99 | 吞吐量 |
|------|:--:|:--:|:--:|:--:|------|------|------|------|------|------|
| HTTP-100 | 100 | 1,000 | 998 | 2 | 11.2s | **88.8** | 1219ms | 1272ms | 1697ms | 2.50 MB/s |
| HTTP-200 | 200 | 2,000 | 1,998 | 2 | 11.2s | **178.7** | 2804ms | 2847ms | 4114ms | 5.03 MB/s |
| HTTP-500 | 500 | 5,000 | 4,992 | 8 | 42.7s | **116.9** | 10297ms | 9428ms | 20644ms | 3.30 MB/s |
| HTTP-1000 | 1000 | 10,000 | 5,888 | 4,112 | 50.6s | **116.4** | 13096ms | 12948ms | 22187ms | 3.27 MB/s |
| HTTPS-50 | 50 | 200 | 200 | 0 | 0.8s | **245.4** | 421ms | 410ms | 583ms | 6.91 MB/s |
| HTTPS-100 | 100 | 500 | 499 | 1 | 10.3s | **48.5** | 1144ms | 1149ms | 1472ms | 1.36 MB/s |
| HTTPS-200 | 200 | 1,000 | 999 | 1 | 10.6s | **94.3** | 2087ms | 2026ms | 2741ms | 2.65 MB/s |

**关键发现**
- 最佳 QPS：**HTTPS-50**（245.4 req/s），100% 成功率，P99 延迟仅 583ms
- 最低延迟：**HTTPS-50**（平均 421ms）
- HTTP-200 为吞吐量甜点：**178.7 QPS，99.9% 成功率**
- HTTP-1000 失败率 41.12%，主要受目标服务器限流及 WSL2 资源限制影响
- 固定线程 + 协程池模型有效降低了线程切换开销，16 个线程即可承载上千并发协程

测试代码：`tests/test_benchmark.cpp`

## 项目结构

\`\`\`
├── blue/               # 核心库
│   ├── fiber.h/cpp     # 协程
│   ├── schedular.h/cpp # 调度器
│   ├── iomanager.h/cpp # I/O 事件循环
│   ├── context.h/cpp   # 上下文切换
│   ├── dbmanager.h/cpp # 数据库管理器
│   ├── redismanager.h/cpp # Redis 管理器
│   └── mysqlpool.h/cpp # MySQL 连接池
├── http/               # HTTP 协议
│   ├── httpserver.cpp  # 代理服务器（核心）
│   ├── httpconnection.cpp # HTTP 连接
│   └── httpParser.cpp  # HTTP 解析器
|-- oldblue/            # 开始跟着syalr写的fiber和thread库
├── tests/              # 测试
|-- thirdparty/         # 第三方库 llhttp
├── Dockerfile
├── docker-compose.yml
└── README.md
\`\`\`

## 技术栈

C++11/14 / AI / ucontext / epoll / OpenSSL / Gumbo / llhttp / Ragel / MySQL / Redis / Docker
\`\`\`