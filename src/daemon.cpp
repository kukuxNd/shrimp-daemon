#include "daemon.hpp"
#include <csignal>
#include <iostream>
#include <thread>

namespace shrimp {

Daemon::Daemon(const Config& config)
    : config_(config), logger_("daemon") 
{
    logger_.info("Initializing ShrimpDaemon v1.0.0");
    
    // 初始化调度器
    scheduler_ = std::make_unique<Scheduler>();
    
    // 创建Agent队列
    agents_ = AgentFactory::createSquad();
    
    logger_.info("Created " + std::to_string(agents_.size()) + " agents");
}

Daemon::~Daemon() {
    stop();
}

void Daemon::start() {
    if (running_.load()) {
        logger_.warning("Daemon already running");
        return;
    }
    
    running_ = true;
    logger_.info("Starting daemon...");
    
    // 启动调度器
    scheduler_->start();
    
    // 启动所有Agent
    for (auto& agent : agents_) {
        agent->start();
    }
    
    // 启动主循环
    runLoop();
}

void Daemon::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    logger_.info("Stopping daemon...");
    
    // 停止调度器
    scheduler_->stop();
    
    // 停止所有Agent
    for (auto& agent : agents_) {
        agent->stop();
    }
    
    logger_.info("Daemon stopped");
}

void Daemon::runLoop() {
    logger_.info("Main loop started, daemon is running...");
    
    while (running_.load()) {
        // 主循环处理
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Daemon::handleSignal(int signal) {
    logger_.warning("Received signal: " + std::to_string(signal));
    if (signal == SIGINT || signal == SIGTERM) {
        stop();
    }
}

} // namespace shrimp
