#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace shrimp {

/**
 * 网络I/O模式
 */
enum class IOMode {
    EPOLL,      // Linux默认
    IO_URING,   // 高性能 (Kernel 5.1+)
    SELECT      // 兼容模式
};

/**
 * 轻量级Socket连接
 */
class Socket {
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();
    
    // 禁止拷贝
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    // 允许移动
    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;
    
    // 连接
    bool connect(const std::string& host, int port);
    bool bind(int port);
    bool listen(int backlog = 128);
    Socket accept();
    
    // I/O
    ssize_t send(const char* data, size_t len);
    ssize_t recv(char* buffer, size_t len);
    
    // 非阻塞
    bool setNonBlocking(bool enabled);
    bool setNoDelay(bool enabled);
    bool setKeepAlive(bool enabled);
    
    // 关闭
    void close();
    bool isConnected() const { return fd_ >= 0; }
    
    int fd() const { return fd_; }

private:
    int fd_ = -1;
};

/**
 * 高性能事件循环 (Epoll/kqueue/io_uring)
 * 单线程可处理百万连接
 */
class EventLoop {
public:
    using EventHandler = std::function<void(int fd, uint32_t events)>;
    using TimerCallback = std::function<void()>;
    
    EventLoop(IOMode mode = IOMode::EPOLL);
    ~EventLoop();
    
    // 添加/移除文件描述符
    bool addFd(int fd, uint32_t events, EventHandler handler);
    bool modifyFd(int fd, uint32_t events);
    bool removeFd(int fd);
    
    // 定时器
    uint64_t addTimer(std::chrono::milliseconds delay, TimerCallback callback, bool repeat = false);
    void removeTimer(uint64_t timerId);
    
    // 运行循环
    void run();
    void stop();
    void runOnce(int timeoutMs = -1);
    
    // 唤醒
    void wakeup();
    
    // 统计
    size_t connectionCount() const;
    size_t timerCount() const;

private:
    IOMode mode_;
    std::atomic<bool> running_{false};
    
    // Epoll相关
    int epollFd_ = -1;
    int wakeupFd_[2] = {-1, -1};
    
    // 处理器
    std::map<int, EventHandler> handlers_;
    
    // 定时器
    struct Timer {
        uint64_t id;
        std::chrono::steady_clock::time_point when;
        std::chrono::milliseconds interval;
        TimerCallback callback;
        bool repeat;
    };
    std::priority_queue<Timer, std::vector<Timer>, 
        std::function<bool(const Timer&, const Timer&)>> timers_;
    
    std::atomic<uint64_t> nextTimerId_{0};
    
    Logger logger_;
};

/**
 * 高性能服务器
 */
class Server {
public:
    Server(IOMode mode = IOMode::EPOLL);
    ~Server();
    
    // 配置
    void setPort(int port) { port_ = port; }
    void setBacklog(int backlog) { backlog_ = backlog; }
    void setWorkerThreads(int count) { workerThreads_ = count; }
    void setHandler(std::function<void(Socket&)> handler);
    
    // 启动/停止
    bool start();
    void stop();
    
    // 状态
    bool isRunning() const { return running_.load(); }
    size_t connectionCount() const;

private:
    void acceptLoop();
    void workerLoop(EventLoop* loop);
    
    int port_ = 8080;
    int backlog_ = 128;
    int workerThreads_ = 4;
    
    std::atomic<bool> running_{false};
    Socket listenSocket_;
    
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<EventLoop>> loops_;
    
    std::function<void(Socket&)> handler_;
    
    Logger logger_;
};

/**
 * 客户端连接池
 */
class ConnectionPool {
public:
    ConnectionPool(const std::string& host, int port, size_t size);
    ~ConnectionPool();
    
    // 获取连接
    Socket* acquire();
    void release(Socket* conn);
    
    // 统计
    size_t available() const;
    size_t total() const;

private:
    std::string host_;
    int port_;
    size_t poolSize_;
    
    std::vector<std::unique_ptr<Socket>> pool_;
    std::queue<Socket*> available_;
    
    std::mutex mutex_;
};

} // namespace shrimp
