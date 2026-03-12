#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <optional>
#include "logger.hpp"

namespace shrimp {

// ========== 轻量级协程库 (仿Go风格) ==========

/**
 * 协程栈大小配置
 */
constexpr size_t DEFAULT_STACK_SIZE = 8 * 1024;  // 8KB 轻量栈

/**
 * 协程状态
 */
enum class CoroutineState {
    READY,
    RUNNING,
    SUSPENDED,
    DEAD
};

/**
 * 轻量级协程 (用户态线程)
 * 每个协程只占用 ~8KB 栈空间，100万协程仅需 ~8GB
 */
class Coroutine {
public:
    using Entry = std::function<void()>;
    
    explicit Coroutine(Entry entry, size_t stackSize = DEFAULT_STACK_SIZE);
    ~Coroutine();
    
    // 协程控制
    void resume();
    void yield();
    
    // 状态查询
    CoroutineState getState() const { return state_; }
    bool isDead() const { return state_ == CoroutineState::DEAD; }
    
    // ID
    uint64_t id() const { return id_; }
    
private:
    static uint64_t nextId();
    
    uint64_t id_;
    Entry entry_;
    CoroutineState state_ = CoroutineState::READY;
    // 实际栈内存在 malloc'd 内存中
    void* stack_ = nullptr;
    size_t stackSize_;
    
    // 协程上下文 (assembly 切换)
    void* context_[2];  // 保存 RSP, RBP
    
    static void stub(Coroutine* co);
};

/**
 * 协程调度器 (N:M 模型)
 * 一个线程可以运行多个协程
 */
class CoroutineScheduler {
public:
    CoroutineScheduler();
    ~CoroutineScheduler();
    
    // 创建协程
    uint64_t spawn(Coroutine::Entry entry);
    
    // 运行调度器 (在专用线程上)
    void run();
    
    // 停止调度器
    void stop();
    
    // 协程数量
    size_t count() const;
    
    // 当前运行协程ID
    uint64_t currentId() const { return currentCoroutineId_; }
    
    // 协程让步
    static void yield();
    
    // 调度器实例 (线程局部)
    static CoroutineScheduler* current();

private:
    void schedule();
    void recycleDead();
    
    std::vector<std::thread> workers_;
    std::queue<uint64_t> readyQueue_;
    std::map<uint64_t, std::unique_ptr<Coroutine>> coroutines_;
    
    std::atomic<bool> running_{false};
    std::mutex mutex_;
    std::condition_variable cv_;
    
    uint64_t currentCoroutineId_ = 0;
    uint64_t nextCoroutineId_ = 0;
    
    Logger logger_;
};

// ========== 高性能无锁队列 ==========

/**
 * 单生产者单消费者无锁队列 (MPSC)
 * 用于协程间高效通信
 */
template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t capacity = 1024);
    ~LockFreeQueue();
    
    // 推入 (可多线程)
    bool push(T&& value);
    bool push(const T& value);
    
    // 弹出 (单消费者)
    std::optional<T> pop();
    
    bool empty() const;
    size_t size() const;
    
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        
        Node(const T& d) : data(d), next(nullptr) {}
        Node(T&& d) : data(std::move(d)), next(nullptr) {}
    };
    
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    size_t capacity_;
};

// ========== 任务执行器 (Actor模型) ==========

/**
 * 任务类型
 */
struct Task {
    uint64_t id;
    std::string agentName;
    std::string action;
    std::chrono::system_clock::time_point createdAt;
    std::function<void()> handler;
    
    Task(uint64_t tid, const std::string& agent, const std::string& act, std::function<void()> h)
        : id(tid), agentName(agent), action(act), 
          createdAt(std::chrono::system_clock::now()), handler(std::move(h)) {}
};

/**
 * 高性能任务执行器
 * 基于协程 + 无锁队列，支持百万并发
 */
class TaskExecutor {
public:
    TaskExecutor();
    ~TaskExecutor();
    
    // 提交任务 (异步)
    uint64_t submit(const std::string& agent, const std::string& action, std::function<void()> handler);
    
    // 批量提交
    uint64_t submitBatch(const std::string& agent, const std::string& action, 
                        std::vector<std::function<void()>> handlers);
    
    // 等待任务完成
    void wait(uint64_t taskId);
    
    // 任务状态
    enum class TaskStatus { PENDING, RUNNING, DONE, FAILED };
    TaskStatus getStatus(uint64_t taskId) const;
    
    // 统计
    size_t pendingCount() const;
    size_t runningCount() const;
    size_t completedCount() const;
    
    // 启动/停止
    void start(int workerThreads = 4);
    void stop();

private:
    void workerLoop();
    
    struct TaskContext {
        std::atomic<TaskStatus> status{TaskStatus::PENDING};
        std::atomic<bool> done{false};
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    std::vector<std::thread> workers_;
    std::queue<Task> pendingQueue_;
    std::map<uint64_t, std::shared_ptr<TaskContext>> tasks_;
    
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> nextTaskId_{0};
    std::atomic<uint64_t> completedCount_{0};
    
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    
    Logger logger_;
};

// ========== Agent池管理 (百万级) ==========

/**
 * Agent统计信息
 */
struct AgentStats {
    uint64_t total = 0;
    uint64_t active = 0;
    uint64_t idle = 0;
    uint64_t error = 0;
    uint64_t createdPerSec = 0;
    uint64_t processedPerSec = 0;
};

/**
 * Agent池 - 支持百万级Agent管理
 * 使用稀疏存储 + 对象池
 */
class AgentPool {
public:
    AgentPool();
    ~AgentPool();
    
    // 注册Agent
    bool registerAgent(const std::string& name, const std::string& type);
    
    // 注销Agent
    bool unregisterAgent(const std::string& name);
    
    // 获取Agent信息
    bool getAgent(const std::string& name) const;
    
    // 批量创建Agent
    uint64_t batchCreate(const std::string& prefix, const std::string& type, uint64_t count);
    
    // 批量删除
    uint64_t batchDelete(const std::string& prefix);
    
    // 统计
    AgentStats stats() const;
    
    // Agent数量
    size_t size() const { return agents_.size(); }

private:
    struct AgentInfo {
        std::string name;
        std::string type;
        std::chrono::system_clock::time_point createdAt;
        std::atomic<uint64_t> taskCount{0};
        std::atomic<bool> active{false};
    };
    
    // 稀疏存储: 使用无序_map O(1)查找
    std::unordered_map<std::string, std::unique_ptr<AgentInfo>> agents_;
    
    mutable std::mutex mutex_;
    Logger logger_;
};

// ========== 高性能事件循环 ==========

/**
 * 事件类型
 */
enum class EventType {
    TASK_SUBMITTED,
    TASK_COMPLETED,
    TASK_FAILED,
    AGENT_REGISTERED,
    AGENT_UNREGISTERED,
    HEARTBEAT,
    TIMEOUT
};

/**
 * 事件
 */
struct Event {
    EventType type;
    std::string source;
    std::string data;
    std::chrono::system_clock::time_point timestamp;
    
    Event(EventType t, const std::string& s, const std::string& d = "")
        : type(t), source(s), data(d), 
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * 高性能事件总线 (发布订阅)
 * 使用无锁队列，支持百万级事件
 */
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;
    
    EventBus();
    ~EventBus();
    
    // 订阅
    uint64_t subscribe(EventType type, Handler handler);
    uint64_t subscribe(const std::vector<EventType>& types, Handler handler);
    
    // 取消订阅
    void unsubscribe(uint64_t subscriptionId);
    
    // 发布事件
    void publish(const Event& event);
    
    // 事件队列大小
    size_t queueSize() const;
    
    // 启动事件循环
    void start();
    void stop();

private:
    void eventLoop();
    
    struct Subscription {
        uint64_t id;
        EventType type;
        Handler handler;
        std::atomic<bool> active{true};
    };
    
    std::vector<std::thread> loops_;
    std::vector<std::unique_ptr<LockFreeQueue<Event>>> queues_;
    std::map<uint64_t, std::unique_ptr<Subscription>> subscriptions_;
    
    std::atomic<bool> running_{false};
    uint64_t nextSubId_ = 0;
    
    std::mutex mutex_;
    Logger logger_;
};

} // namespace shrimp
