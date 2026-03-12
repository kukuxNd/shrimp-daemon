#include "metrics.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>

namespace shrimp {

// ========== Metrics 实现 ==========

Metrics& Metrics::instance() {
    static Metrics instance;
    return instance;
}

void Metrics::increment(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counters_.find(name) == counters_.end()) {
        counters_[name] = std::make_unique<Counter>();
    }
    counters_[name]->value.fetch_add(value, std::memory_order_relaxed);
}

void Metrics::decrement(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (counters_.find(name) == counters_.end()) {
        counters_[name] = std::make_unique<Counter>();
    }
    counters_[name]->value.fetch_sub(value, std::memory_order_relaxed);
}

void Metrics::gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gauges_.find(name) == gauges_.end()) {
        gauges_[name] = std::make_unique<Gauge>();
    }
    gauges_[name]->value.store(value, std::memory_order_relaxed);
}

void Metrics::timer(const std::string& name, double ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timers_.find(name) == timers_.end()) {
        timers_[name] = std::make_unique<Timer>();
    }
    
    auto& t = *timers_[name];
    t.count.fetch_add(1, std::memory_order_relaxed);
    
    // 浮点数原子加操作 (C++20支持，这里用简单实现)
    double expected = t.sum.load();
    double desired = expected + ms;
    while (!t.sum.compare_exchange_weak(expected, desired)) {
        desired = expected + ms;
    }
    
    // 更新min/max
    double currentMin = t.min.load();
    while (ms < currentMin && !t.min.compare_exchange_weak(currentMin, ms)) {}
    
    double currentMax = t.max.load();
    while (ms > currentMax && !t.max.compare_exchange_weak(currentMax, ms)) {}
}

void Metrics::histogram(const std::string& name, double value) {
    timer(name, value);  // 简化实现
}

Metrics::Snapshot Metrics::snapshot() const {
    Snapshot snap;
    
    for (auto& [name, counter] : counters_) {
        snap.counters[name] = counter->value.load(std::memory_order_relaxed);
    }
    
    for (auto& [name, gauge] : gauges_) {
        snap.gauges[name] = gauge->value.load(std::memory_order_relaxed);
    }
    
    for (auto& [name, timer] : timers_) {
        double avg = timer->count.load() > 0 
            ? timer->sum.load() / timer->count.load()
            : 0;
        snap.timers[name] = avg;
        snap.timerStats[name] = {
            timer->min.load(),
            timer->max.load()
        };
    }
    
    return snap;
}

std::string Metrics::format() const {
    std::ostringstream oss;
    auto snap = snapshot();
    
    oss << "\n╔═══════════════════════════════════════════════════════════╗\n";
    oss << "║                      Metrics                             ║\n";
    oss << "╠═══════════════════════════════════════════════════════════╣\n";
    
    if (!snap.counters.empty()) {
        oss << "║ [Counters]\n";
        for (auto& [name, value] : snap.counters) {
            oss << "║   " << std::left << std::setw(30) << name 
                << " = " << value << "\n";
        }
    }
    
    if (!snap.gauges.empty()) {
        oss << "║ [Gauges]\n";
        for (auto& [name, value] : snap.gauges) {
            oss << "║   " << std::left << std::setw(30) << name 
                << " = " << std::fixed << std::setprecision(2) << value << "\n";
        }
    }
    
    if (!snap.timers.empty()) {
        oss << "║ [Timers (avg/min/max ms)]\n";
        for (auto& [name, avg] : snap.timers) {
            auto [minVal, maxVal] = snap.timerStats[name];
            oss << "║   " << std::left << std::setw(20) << name 
                << " avg=" << std::fixed << std::setprecision(2) << avg
                << " min=" << minVal << " max=" << maxVal << "\n";
        }
    }
    
    oss << "╚═══════════════════════════════════════════════════════════╝\n";
    return oss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    timers_.clear();
}

// ========== Profiler 实现 ==========

Profiler::Profiler(const std::string& name) : name_(name) {
    start_ = std::chrono::high_resolution_clock::now();
}

Profiler::~Profiler() {
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start_).count();
    Metrics::instance().timer(name_, elapsed / 1000.0);
}

double Profiler::elapsed() const {
    return std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - start_).count();
}

// ========== AutoCounter 实现 ==========

AutoCounter::AutoCounter(const std::string& name) : name_(name) {
    Metrics::instance().increment(name_);
}

AutoCounter::~AutoCounter() {
    Metrics::instance().decrement(name_);
}

} // namespace shrimp
