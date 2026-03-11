#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "task.hpp"
#include "logger.hpp"

namespace shrimp {

/**
 * 任务调度器 - 支持Cron表达式和Heartbeat轮询
 */
class Scheduler {
public:
    using TaskCallback = std::function<void(const Task&)>;
    
    Scheduler();
    ~Scheduler();
    
    // 添加定时任务 (Cron风格)
    void addCronTask(const std::string& name, 
                     const std::string& cronExpr,
                     TaskCallback callback);
    
    // 添加心跳任务 (固定间隔)
    void addHeartbeatTask(const std::string& name,
                          int intervalMinutes,
                          TaskCallback callback);
    
    // 启动调度器
    void start();
    
    // 停止调度器
    void stop();
    
    // 获取下次执行时间
    std::chrono::system_clock::time_point getNextRunTime(const std::string& taskName) const;

private:
    void schedulerThread();
    void processCronTask(const CronTask& task);
    void processHeartbeatTask(const HeartbeatTask& task);
    
    std::vector<CronTask> cronTasks_;
    std::vector<HeartbeatTask> heartbeatTasks_;
    std::map<std::string, TaskCallback> callbacks_;
    
    std::atomic<bool> running_{false};
    std::thread schedulerThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
    Logger logger_;
};

} // namespace shrimp
