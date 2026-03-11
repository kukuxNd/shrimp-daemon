#pragma once

#include <string>
#include <atomic>
#include <memory>
#include "scheduler.hpp"
#include "agent.hpp"
#include "config.hpp"
#include "logger.hpp"

namespace shrimp {

/**
 * 守护进程核心 - 24小时不间断运行
 */
class Daemon {
public:
    explicit Daemon(const Config& config);
    ~Daemon();
    
    // 启动守护进程
    void start();
    
    // 停止守护进程
    void stop();
    
    // 是否运行中
    bool isRunning() const { return running_.load(); }

private:
    void runLoop();
    void handleSignal(int signal);
    
    Config config_;
    std::atomic<bool> running_{false};
    std::unique_ptr<Scheduler> scheduler_;
    std::vector<std::unique_ptr<Agent>> agents_;
    Logger logger_;
};

} // namespace shrimp
