# Blue Proxy

基于 C++11/14/17 的 HTTP/HTTPS 代理服务器，支持正向代理、简单反向代理、HTML 动态重写、Redis 缓存、MySQL 日志、Docker 容器化部署。

## 特性

- **多模式代理**
  - 正向代理（HTTP/HTTPS CONNECT 隧道）
  - 反向代理（路径前缀 `/blue/`）
  - WebSocket 代理（正向模式）
  - PAC 自动代理配置
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
| `proxy.cache_ttl` | 60 | 缓存过期时间（秒） |
| `proxy.tunnel_timeout` | 1000 | 隧道 select 超时（毫秒） |
| `db.host` | localhost | 数据库主机 |
| `redis.host` | 127.0.0.1 | Redis 主机 |

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

C++23 / ucontext / epoll / OpenSSL / Gumbo / llhttp / Ragel / MySQL / Redis / Docker
\`\`\`