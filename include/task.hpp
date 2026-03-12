#pragma once

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <functional>
#include <memory>

namespace shrimp {

// 前向声明
class Logger;
class Memory;

/**
 * 简化的Cron解析器
 */
struct CronExpr {
    int minute = -1;    // -1 表示每分钟
    int hour = -1;      // -1 表示每小时
    int dayOfMonth = -1;
    int month = -1;
    int dayOfWeek = -1;
    
    static CronExpr parse(const std::string& expr);
    bool matches(const std::tm& time) const;
};

/**
 * 基础任务类
 */
struct Task {
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point scheduledAt;
    int priority = 0;  // 0=普通, 1=高优先级, 2=紧急
    
    Task(const std::string& n, const std::string& desc = "")
        : name(n), description(desc), 
          createdAt(std::chrono::system_clock::now()) {}
};

/**
 * Cron任务 - 定时执行
 */
struct CronTask : Task {
    CronExpr cronExpr;
    std::chrono::system_clock::time_point lastRun;
    std::chrono::system_clock::time_point nextRun;
    
    CronTask(const std::string& n, const std::string& expr);
};

/**
 * Heartbeat任务 - 周期轮询
 */
struct HeartbeatTask : Task {
    int intervalMinutes;
    std::chrono::system_clock::time_point lastBeat;
    std::chrono::system_clock::time_point nextBeat;
    
    HeartbeatTask(const std::string& n, int interval)
        : Task(n), intervalMinutes(interval) {}
};

/**
 * 事件驱动任务
 */
struct EventTask : Task {
    std::string trigger;  // 触发事件类型
    std::string payload;  // 事件数据
    
    EventTask(const std::string& n, const std::string& trig, const std::string& data)
        : Task(n), trigger(trig), payload(data) {}
};

} // namespace shrimp
