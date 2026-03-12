# ShrimpDaemon v1.0.0 - 24/7 百万级并发 Agent 框架

🦐 **小而精，高性能，百万并发支持**

```
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║   🦐 ShrimpDaemon - 虾女团核心引擎                           ║
║   "撒旦的化身，魔女与天使的两面派"                            ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
```

## ✨ 核心特性

### 性能
- **轻量级协程** - 每个协程仅 8KB 栈空间，单进程支持百万协程
- **无锁数据结构** - 原子操作保证线程安全，避免锁竞争
- **Epoll/IO_URING** - Linux 高性能 IO，单线程处理百万连接
- **内存池管理** - 预分配对象池，避免频繁 malloc/free
- **零拷贝传输** - 减少数据复制开销

### 功能
- **守护进程模式** - 24/7 不间断运行
- **任务调度** - Cron定时 + Heartbeat心跳 + 事件驱动
- **多Agent架构** - 可扩展工厂模式
- **命令行交互** - 运行时状态查询和手动触发
- **指标收集** - 无锁统计，实时监控

## 🚀 快速开始

### 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行

```bash
# 交互模式
./shrimp_daemon -i

# 守护进程模式
./shrimp_daemon -d

# 帮助
./shrimp_daemon --help
```

### 压力测试

```bash
# 启动服务器
./shrimp_stress server 8080

# 10万并发测试
./shrimp_stress client 127.0.0.1:8080 -c 100000

# 100万并发测试
./shrimp_stress client 127.0.0.1:8080 -c 1000000 -t 16

# 本地演示模式
./shrimp_stress
```

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 单进程协程数 | 100万+ |
| 单线程连接数 | 100万+ |
| 内存占用/协程 | ~8KB |
| 连接建立速度 | 100k+/秒 |
| 消息吞吐 | 1M+/秒 |

## 🎯 交互命令

```
shrimp> status          # 查看所有Agent状态
shrimp> list            # 列出可用Agent
shrimp> task scout run  # 手动触发任务
shrimp> info scout      # 查看Agent详情
shrimp> memory scout    # 查看记忆
shrimp> clear           # 清屏
shrimp> quit            # 退出
```

## 🏗️ 架构

```
┌─────────────────────────────────────────────────────────────┐
│                    ShrimpDaemon                             │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                 Coroutine Scheduler                   │  │
│  │   100万+ 轻量级协程，N:M 调度模型                     │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                 Event Loop (Epoll)                    │  │
│  │   单线程百万连接，零拷贝 IO                           │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                  Lock-Free Queue                      │  │
│  │   MPSC 无锁队列，协程间通信                           │  │
│  └───────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              Memory Pool / Object Pool                │  │
│  │   预分配内存块，避免 GC 停顿                          │  │
│  └───────────────────────────────────────────────────────┘  │
│                             │                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │  Scout  │ │ Writer  │ │ Artist  │ │  ...    │  Agent池  │
│  │ 情报员  │ │ 撰稿人  │ │ 美术官  │ │         │           │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘           │
└─────────────────────────────────────────────────────────────┘
```

## 📁 项目结构

```
shrimp-daemon/
├── CMakeLists.txt
├── shrimp.conf              # 配置文件
├── README.md
├── include/
│   ├── daemon.hpp           # 守护进程核心
│   ├── scheduler.hpp        # 任务调度器
│   ├── agent.hpp            # Agent系统
│   ├── task.hpp             # 任务定义
│   ├── memory.hpp           # 记忆系统
│   ├── logger.hpp           # 日志系统
│   ├── config.hpp           # 配置管理
│   ├── concurrency.hpp      # 协程 + 无锁队列
│   ├── metrics.hpp          # 指标收集
│   ├── network.hpp          # 网络IO
│   └── memory.hpp           # 内存池
├── src/
│   ├── main.cpp
│   ├── daemon.cpp
│   ├── scheduler.cpp
│   ├── agent.cpp
│   ├── task.cpp
│   ├── memory.cpp
│   ├── logger.cpp
│   ├── config.cpp
│   ├── metrics.cpp
│   └── stress_test.cpp      # 压力测试
└── build/
```

## 🔧 配置示例

```ini
# shrimp.conf

# ========== 全局配置 ==========
workspace = ./workspace
log_level = info
log_file = ./logs/shrimp.log

# ========== 心跳配置 ==========
heartbeat_interval = 30
heartbeat_prompt = 检查各Agent状态

# ========== 重试配置 ==========
max_retries = 3
retry_delay_ms = 1000

# ========== Cron 任务 ==========
cron.scout = 0 */2 * * *|情报搜集任务，每2小时执行
cron.writer = 0 8,12,20 * * *|内容生成任务，每天8/12/20点执行
cron.artist = 0 9 * * *|视觉生成任务，每天9点执行

# ========== Agent 配置 ==========
agent.scout.enabled = true
agent.scout.type = scout

agent.writer.enabled = true
agent.writer.type = writer

agent.artist.enabled = true
agent.artist.type = artist
```

## 💡 使用场景

- **多Agent系统** - 虾女团扩展
- **爬虫集群** - 百万级并发数据采集
- **IoT设备管理** - 百万级设备连接
- **实时通信** - 高并发消息推送
- **API网关** - 百万QPS路由转发

## 📜 License

MIT License

---

**虾女团计划** | 10个Agent自主社交，扩张影响力 🦐

Made with ♠️ by 虾3x
