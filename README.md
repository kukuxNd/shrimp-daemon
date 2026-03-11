# ShrimpDaemon 🦐

> 24小时不间断运行的C++ Agent框架 - 虾女团的后台引擎

## ✨ 特性

- **守护进程**: 常驻后台运行，支持信号处理
- **任务调度**: Cron定时任务 + Heartbeat心跳轮询
- **多Agent架构**: 可扩展的Agent工厂模式
- **记忆系统**: 文件持久化的状态管理
- **日志系统**: 线程安全的多级别日志
- **配置管理**: 简洁的INI风格配置文件

## 🏗️ 架构

```
┌─────────────────────────────────────────────┐
│              ShrimpDaemon                   │
│  ┌───────────────────────────────────────┐  │
│  │           Scheduler                   │  │
│  │  ┌─────────┐  ┌──────────┐           │  │
│  │  │  Cron   │  │Heartbeat │           │  │
│  │  │ Tasks   │  │  Tasks   │           │  │
│  │  └─────────┘  └──────────┘           │  │
│  └───────────────────────────────────────┘  │
│                    │                        │
│  ┌─────────────────┼───────────────────┐   │
│  │                 ▼                   │   │
│  │   ┌────────┐ ┌────────┐ ┌────────┐  │   │
│  │   │ Scout  │ │ Writer │ │ Artist │  │   │
│  │   │情报员  │ │撰稿人  │ │美术官  │  │   │
│  │   └────────┘ └────────┘ └────────┘  │   │
│  │              Agent Squad            │   │
│  └─────────────────────────────────────┘   │
│                                             │
│  ┌─────────────────────────────────────┐   │
│  │    Memory   │   Logger   │ Config   │   │
│  └─────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

## 🚀 快速开始

### 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行

```bash
# 前台运行
./shrimp_daemon

# 使用指定配置文件
./shrimp_daemon -c /etc/shrimp.conf

# 后台运行
./shrimp_daemon -d
```

### 配置示例

```ini
# 心跳间隔 (分钟)
heartbeat_interval = 30

# Cron任务
cron.scout = 0 */2 * * *|情报搜集任务

# Agent开关
agent.scout.enabled = true
```

## 📁 项目结构

```
shrimp-daemon/
├── CMakeLists.txt          # 构建配置
├── shrimp.conf              # 默认配置
├── README.md
├── include/                 # 头文件
│   ├── daemon.hpp           # 守护进程
│   ├── scheduler.hpp        # 调度器
│   ├── agent.hpp            # Agent基类
│   ├── task.hpp             # 任务定义
│   ├── memory.hpp           # 记忆系统
│   ├── logger.hpp           # 日志系统
│   └── config.hpp           # 配置管理
└── src/                     # 实现文件
    ├── main.cpp
    ├── daemon.cpp
    ├── scheduler.cpp
    ├── agent.cpp
    ├── task.cpp
    ├── memory.cpp
    ├── logger.cpp
    └── config.cpp
```

## 🔧 扩展Agent

```cpp
// 自定义Agent
class MyAgent : public Agent {
public:
    MyAgent() : Agent(AgentType::CUSTOM, "my_agent") {}
    
    void execute(const Task& task) override {
        // 你的逻辑
        logger().info("执行任务: " + task.name);
        memory().set("last_run", timestamp());
    }
};

// 注册到工厂
std::unique_ptr<Agent> AgentFactory::create(AgentType type) {
    switch (type) {
        case AgentType::CUSTOM:
            return std::make_unique<MyAgent>();
        // ...
    }
}
```

## 📊 核心组件

### Scheduler (调度器)
- **Cron任务**: 支持 `minute hour day month weekday` 格式
- **Heartbeat任务**: 固定间隔轮询
- **线程安全**: 独立调度线程，条件变量唤醒

### Memory (记忆系统)
- **键值存储**: `set(key, value)` / `get(key)`
- **时间线**: `addTimeline(event)` 记录事件
- **持久化**: 自动序列化/反序列化

### Logger (日志系统)
- **多级别**: DEBUG / INFO / WARNING / ERROR
- **线程安全**: 互斥锁保护
- **双输出**: 控制台 + 文件

## 🎯 设计原则

1. **单一职责**: 每个Agent只做一件事
2. **状态隔离**: 独立的Memory实例
3. **故障隔离**: 异常不传播到其他Agent
4. **文件持久化**: 重启不丢失状态

## 📜 License

MIT License

---

**虾女团计划** | 10个Agent自主社交，扩张影响力 🦐
