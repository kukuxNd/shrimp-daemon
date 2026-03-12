#pragma once

#include <chrono>
#include <atomic>
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <memory>
#include <limits>

namespace shrimp {

/**
 * 高性能指标收集系统
 * 无锁统计，支持每秒百万次采样
 */
class Metrics {
public:
    static Metrics& instance();
    
    // 计数器
    void increment(const std::string& name, int64_t value = 1);
    void decrement(const std::string& name, int64_t value = 1);
    
    // 计量器
    void gauge(const std::string& name, double value);
    
    // 计时器 (毫秒)
    void timer(const std::string& name, double ms);
    
    // 直方图
    void histogram(const std::string& name, double value);
    
    // 快照
    struct Snapshot {
        std::map<std::string, int64_t> counters;
        std::map<std::string, double> gauges;
        std::map<std::string, double> timers;  // 平均
        std::map<std::string, std::pair<double, double>> timerStats;  // min/max
    };
    
    Snapshot snapshot() const;
    
    // 格式化输出
    std::string format() const;
    
    // 重置
    void reset();

private:
    Metrics() = default;
    
    // 使用atomic保证无锁
    struct Counter {
        std::atomic<int64_t> value{0};
    };
    
    struct Gauge {
        std::atomic<double> value{0.0};
    };
    
    struct Timer {
        std::atomic<int64_t> count{0};
        std::atomic<double> sum{0.0};
        std::atomic<double> min{std::numeric_limits<double>::max()};
        std::atomic<double> max{std::numeric_limits<double>::lowest()};
    };
    
    std::unordered_map<std::string, std::unique_ptr<Counter>> counters_;
    std::unordered_map<std::string, std::unique_ptr<Gauge>> gauges_;
    std::unordered_map<std::string, std::unique_ptr<Timer>> timers_;
    
    mutable std::mutex mutex_;
};

/**
 * 性能剖析器 - 自动计时
 */
class Profiler {
public:
    explicit Profiler(const std::string& name);
    ~Profiler();
    
    double elapsed() const;

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

/**
 * 自动计数器 - RAII
 */
class AutoCounter {
public:
    AutoCounter(const std::string& name);
    ~AutoCounter();

private:
    std::string name_;
};

} // namespace shrimp
