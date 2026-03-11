#include "scheduler.hpp"
#include <ctime>
#include <algorithm>
#include <iostream>

namespace shrimp {

Scheduler::Scheduler() : logger_("scheduler") {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addCronTask(const std::string& name,
                           const std::string& cronExpr,
                           TaskCallback callback) {
    CronTask task(name, cronExpr);
    task.nextRun = std::chrono::system_clock::now();
    cronTasks_.push_back(task);
    callbacks_[name] = std::move(callback);
    logger_.info("Added cron task: " + name + " (" + cronExpr + ")");
}

void Scheduler::addHeartbeatTask(const std::string& name,
                                 int intervalMinutes,
                                 TaskCallback callback) {
    HeartbeatTask task(name, intervalMinutes);
    task.nextBeat = std::chrono::system_clock::now() + 
                    std::chrono::minutes(intervalMinutes);
    heartbeatTasks_.push_back(task);
    callbacks_[name] = std::move(callback);
    logger_.info("Added heartbeat task: " + name + 
                 " (every " + std::to_string(intervalMinutes) + " min)");
}

void Scheduler::start() {
    if (running_.load()) return;
    
    running_ = true;
    schedulerThread_ = std::thread(&Scheduler::schedulerThread, this);
    logger_.info("Scheduler started");
}

void Scheduler::stop() {
    if (!running_.load()) return;
    
    running_ = false;
    cv_.notify_all();
    
    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }
    
    logger_.info("Scheduler stopped");
}

void Scheduler::schedulerThread() {
    while (running_.load()) {
        auto now = std::chrono::system_clock::now();
        
        // 检查Cron任务
        for (auto& task : cronTasks_) {
            if (now >= task.nextRun) {
                processCronTask(task);
                task.lastRun = now;
                // 简单的下一个整点计算
                task.nextRun = now + std::chrono::hours(1);
            }
        }
        
        // 检查Heartbeat任务
        for (auto& task : heartbeatTasks_) {
            if (now >= task.nextBeat) {
                processHeartbeatTask(task);
                task.lastBeat = now;
                task.nextBeat = now + std::chrono::minutes(task.intervalMinutes);
            }
        }
        
        // 等待或短暂休眠
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(10), [this] { 
            return !running_.load(); 
        });
    }
}

void Scheduler::processCronTask(const CronTask& task) {
    logger_.info("Executing cron task: " + task.name);
    auto it = callbacks_.find(task.name);
    if (it != callbacks_.end()) {
        it->second(task);
    }
}

void Scheduler::processHeartbeatTask(const HeartbeatTask& task) {
    logger_.debug("Heartbeat: " + task.name);
    auto it = callbacks_.find(task.name);
    if (it != callbacks_.end()) {
        it->second(task);
    }
}

std::chrono::system_clock::time_point Scheduler::getNextRunTime(const std::string& taskName) const {
    for (const auto& task : cronTasks_) {
        if (task.name == taskName) {
            return task.nextRun;
        }
    }
    for (const auto& task : heartbeatTasks_) {
        if (task.name == taskName) {
            return task.nextBeat;
        }
    }
    return std::chrono::system_clock::now();
}

} // namespace shrimp
