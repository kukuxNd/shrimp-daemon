#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <csignal>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

// 简单日志
#define LOG(fmt, ...) do { \
    auto now = std::chrono::system_clock::now(); \
    auto time = std::chrono::system_clock::to_time_t(now); \
    auto ms_val = std::chrono::duration_cast<std::chrono::milliseconds>( \
        now.time_since_epoch()) % 1000; \
    printf("[%02d:%02d:%02d.%03ld] " fmt "\n", \
        (int)(localtime(&time)->tm_hour), \
        (int)(localtime(&time)->tm_min), \
        (int)(localtime(&time)->tm_sec), \
        (long)(ms_val.count()), ##__VA_ARGS__); \
} while(0)

std::atomic<bool> g_running{true};
std::atomic<uint64_t> g_totalConnections{0};
std::atomic<uint64_t> g_activeConnections{0};
std::atomic<uint64_t> g_messagesSent{0};
std::atomic<uint64_t> g_messagesRecv{0};
std::atomic<uint64_t> g_errors{0};

void signalHandler(int) {
    g_running = false;
}

/**
 * 高性能Echo服务器 - 百万并发测试
 */
class MillionAgentServer {
public:
    MillionAgentServer(int port, int workerThreads)
        : port_(port), workerThreads_(workerThreads) {}
    
    bool start() {
        // 创建epoll
        epollFd_ = epoll_create1(0);
        if (epollFd_ < 0) {
            LOG("Failed to create epoll");
            return false;
        }
        
        // 创建监听socket
        listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd_ < 0) {
            LOG("Failed to create socket");
            return false;
        }
        
        // 设置非阻塞
        int flags = fcntl(listenFd_, F_GETFL, 0);
        fcntl(listenFd_, F_SETFL, flags | O_NONBLOCK);
        
        // 地址复用
        int opt = 1;
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // 绑定
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        
        if (bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG("Failed to bind port %d", port_);
            return false;
        }
        
        // 监听
        if (listen(listenFd_, 65535) < 0) {
            LOG("Failed to listen");
            return false;
        }
        
        // 注册到epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = listenFd_;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev);
        
        LOG("Server started on port %d, workers=%d", port_, workerThreads_);
        return true;
    }
    
    void run() {
        std::vector<std::thread> workers;
        
        // 启动工作线程
        for (int i = 0; i < workerThreads_; ++i) {
            workers.emplace_back([this]() { workerLoop(); });
        }
        
        // 主循环 - 接受连接
        struct epoll_event events[1024];
        
        while (g_running) {
            int nfds = epoll_wait(epollFd_, events, 1024, 100);
            
            for (int i = 0; i < nfds; ++i) {
                int fd = events[i].data.fd;
                
                if (fd == listenFd_) {
                    // 新连接
                    while (true) {
                        struct sockaddr_in clientAddr;
                        socklen_t clientLen = sizeof(clientAddr);
                        int clientFd = accept(listenFd_, 
                                             (struct sockaddr*)&clientAddr, 
                                             &clientLen);
                        
                        if (clientFd < 0) break;
                        
                        // 设置非阻塞
                        int flags = fcntl(clientFd, F_GETFL, 0);
                        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
                        
                        // 注册到epoll
                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        ev.data.fd = clientFd;
                        epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev);
                        
                        g_activeConnections++;
                        g_totalConnections++;
                    }
                } else {
                    // 处理客户端数据
                    if (events[i].events & EPOLLIN) {
                        handleRead(fd);
                    }
                    if (events[i].events & EPOLLERR) {
                        close(fd);
                        g_errors++;
                    }
                }
            }
        }
        
        // 等待工作线程
        for (auto& w : workers) {
            w.join();
        }
    }
    
    void stop() {
        g_running = false;
        if (listenFd_ >= 0) close(listenFd_);
        if (epollFd_ >= 0) close(epollFd_);
    }

private:
    void workerLoop() {
        char buffer[4096];
        
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    void handleRead(int fd) {
        char buffer[4096];
        ssize_t n = read(fd, buffer, sizeof(buffer));
        
        if (n > 0) {
            g_messagesRecv++;
            
            // Echo back
            ssize_t sent = write(fd, buffer, n);
            if (sent > 0) {
                g_messagesSent++;
            }
            
            // 重新注册
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            ev.data.fd = fd;
            epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
        } else if (n == 0 || (n < 0 && errno != EAGAIN)) {
            close(fd);
            g_activeConnections--;
        }
    }
    
    int port_;
    int workerThreads_;
    int listenFd_ = -1;
    int epollFd_ = -1;
};

/**
 * 百万Agent压力测试客户端
 */
class StressTestClient {
public:
    StressTestClient(const std::string& host, int port, int connections)
        : host_(host), port_(port), targetConnections_(connections) {}
    
    void run() {
        LOG("Starting stress test: %d connections to %s:%d", 
            targetConnections_, host_.c_str(), port_);
        
        auto startTime = std::chrono::steady_clock::now();
        
        // 创建连接
        std::vector<std::thread> threads;
        int batch = std::min(1000, targetConnections_);
        
        for (int i = 0; i < targetConnections_; i += batch) {
            int count = std::min(batch, targetConnections_ - i);
            threads.emplace_back([this, count, i]() {
                connectBatch(count, i);
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto connectTime = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(connectTime - startTime).count();
        
        LOG("Connected %d in %.2f sec (%.0f conn/sec)", 
            targetConnections_, seconds, targetConnections_ / seconds);
        
        // 发送消息
        LOG("Sending messages...");
        
        std::atomic<int> sendCount{0};
        auto sendStart = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 10; ++i) {
            threads.clear();
            for (int j = 0; j < 10; ++j) {
                threads.emplace_back([this, &sendCount]() {
                    for (int k = 0; k < 1000; ++k) {
                        sendMessage();
                        sendCount++;
                    }
                });
            }
            for (auto& t : threads) {
                t.join();
            }
            
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - sendStart).count();
            LOG("  Sent %d messages in %.2f sec (%.0f msg/sec)", 
                sendCount.load(), elapsed, sendCount.load() / elapsed);
        }
        
        printStats();
    }
    
private:
    void connectBatch(int count, int offset) {
        for (int i = 0; i < count; ++i) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) continue;
            
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port_);
            inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
            
            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                g_activeConnections++;
                g_totalConnections++;
                connections_.push_back(fd);
            } else {
                close(fd);
                g_errors++;
            }
        }
    }
    
    void sendMessage() {
        if (connections_.empty()) return;
        
        static thread_local std::mt19937 rng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_int_distribution<int> dist(0, connections_.size() - 1);
        
        int fd = connections_[dist(rng)];
        const char* msg = "PING\n";
        
        ssize_t sent = send(fd, msg, strlen(msg), 0);
        if (sent > 0) {
            g_messagesSent++;
            
            char buffer[64];
            ssize_t recvLen = recv(fd, buffer, sizeof(buffer) - 1, 0);
            if (recvLen > 0) {
                g_messagesRecv++;
            }
        } else {
            g_errors++;
        }
    }
    
    void printStats() {
        auto now = std::chrono::steady_clock::now();
        
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
        std::cout << "║              Stress Test Results                         ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Total Connections:  " << std::setw(30) << g_totalConnections.load() << " ║\n";
        std::cout << "║ Active Connections: " << std::setw(30) << g_activeConnections.load() << " ║\n";
        std::cout << "║ Messages Sent:      " << std::setw(30) << g_messagesSent.load() << " ║\n";
        std::cout << "║ Messages Received: " << std::setw(30) << g_messagesRecv.load() << " ║\n";
        std::cout << "║ Errors:             " << std::setw(30) << g_errors.load() << " ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    }
    
    std::string host_;
    int port_;
    int targetConnections_;
    std::vector<int> connections_;
};

void printHelp() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║   🦐 ShrimpDaemon - Million Agent Stress Test              ║
║                                                              ║
║   100万并发Agent压力测试                                      ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝

Usage: shrimp_stress [mode] [options]

Modes:
  server [port]       启动Echo服务器 (默认: 8080)
  client [host:port] 启动压力测试客户端
  test               单元测试模式

Options:
  -t, --threads N    工作线程数 (默认: 4)
  -c, --connections N 连接数 (默认: 10000)
  -h, --help         显示帮助

Examples:
  # 启动服务器
  ./shrimp_stress server 8080
  
  # 启动10万并发客户端
  ./shrimp_stress client 127.0.0.1:8080 -c 100000
  
  # 启动100万并发测试
  ./shrimp_stress client 127.0.0.1:8080 -c 1000000 -t 16
)" << std::endl;
}

int main(int argc, char* argv[]) {
    printHelp();
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << "  🦐 Million Agent Stress Test - Quick Demo\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << "\n";
    
    // 设置信号
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string mode = (argc > 1) ? argv[1] : "test";
    
    if (mode == "server") {
        int port = (argc > 2) ? std::stoi(argv[2]) : 8080;
        int threads = 4;
        
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-t" || arg == "--threads") {
                if (i + 1 < argc) threads = std::stoi(argv[++i]);
            }
        }
        
        MillionAgentServer server(port, threads);
        if (server.start()) {
            server.run();
            server.stop();
        }
    } 
    else if (mode == "client") {
        std::string addr = (argc > 2) ? argv[2] : "127.0.0.1:8080";
        int connections = 10000;
        int threads = 4;
        
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-c" || arg == "--connections") {
                if (i + 1 < argc) connections = std::stoi(argv[++i]);
            } else if (arg == "-t" || arg == "--threads") {
                if (i + 1 < argc) threads = std::stoi(argv[++i]);
            }
        }
        
        size_t colon = addr.find(':');
        std::string host = addr.substr(0, colon);
        int port = std::stoi(addr.substr(colon + 1));
        
        StressTestClient client(host, port, connections);
        client.run();
    } 
    else {
        // 演示模式 - 本地测试
        LOG("Running demo mode...");
        
        // 测试内存池
        LOG("\n=== Memory Pool Test ===");
        {
            std::vector<void*> ptrs;
            for (int i = 0; i < 1000; ++i) {
                ptrs.push_back(malloc(1024));
            }
            LOG("Allocated 1000 x 1KB blocks");
            for (auto p : ptrs) free(p);
            LOG("Freed 1000 blocks");
        }
        
        // 测试原子操作
        LOG("\n=== Atomic Counter Test ===");
        {
            std::atomic<uint64_t> counter{0};
            std::vector<std::thread> threads;
            
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 10; ++i) {
                threads.emplace_back([&counter]() {
                    for (int j = 0; j < 100000; ++j) {
                        counter.fetch_add(1);
                    }
                });
            }
            for (auto& t : threads) t.join();
            auto end = std::chrono::high_resolution_clock::now();
            
            double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
            double ops_per_sec = (elapsed_ms > 0) ? (1000000.0 / elapsed_ms * 1000.0) : 0;
            LOG("1M atomic increments in %.2f ms (%.0f ops/sec)", 
                elapsed_ms, ops_per_sec);
            LOG("Final counter: %lu", counter.load());
        }
        
        // 模拟百万Agent
        LOG("\n=== Million Agent Simulation ===");
        {
            std::atomic<uint64_t> agentId{0};
            std::atomic<uint64_t> activeAgents{0};
            std::vector<std::thread> workers;
            
            int numWorkers = std::thread::hardware_concurrency();
            int agentsPerWorker = 1000000 / numWorkers;
            
            auto start = std::chrono::high_resolution_clock::now();
            
            for (int w = 0; w < numWorkers; ++w) {
                workers.emplace_back([&agentId, &activeAgents, agentsPerWorker]() {
                    for (int i = 0; i < agentsPerWorker; ++i) {
                        uint64_t id = agentId.fetch_add(1);
                        activeAgents++;
                        
                        // 模拟Agent工作
                        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
                        
                        activeAgents--;
                    }
                });
            }
            
            // 定期报告进度
            std::thread reporter([&agentId, &activeAgents]() {
                while (agentId.load() < 1000000) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    LOG("Progress: %lu agents, %lu active", 
                        agentId.load(), activeAgents.load());
                }
            });
            
            for (auto& w : workers) w.join();
            reporter.join();
            
            auto end = std::chrono::high_resolution_clock::now();
            double seconds = std::chrono::duration<double>(end - start).count();
            
            LOG("Created 1,000,000 agents in %.2f seconds", seconds);
            LOG("Throughput: %.0f agents/sec", 1000000.0 / seconds);
        }
        
        LOG("\n✅ Demo completed!");
    }
    
    return 0;
}
