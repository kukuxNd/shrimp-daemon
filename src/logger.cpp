#include "logger.hpp"
#include <iostream>
#include <sstream>

namespace shrimp {

Logger::Logger(const std::string& name) : name_(name) {}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    file_.open(path, std::ios::app);
    toFile_ = file_.is_open();
}

void Logger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, msg);
}

void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, msg);
}

void Logger::warning(const std::string& msg) {
    log(LogLevel::WARNING, msg);
}

void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, msg);
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    oss << "[" << timestamp() << "] "
        << "[" << levelToString(level) << "] "
        << "[" << name_ << "] " << msg;
    
    std::string logLine = oss.str();
    
    // 输出到控制台
    if (level >= LogLevel::WARNING) {
        std::cerr << logLine << std::endl;
    } else {
        std::cout << logLine << std::endl;
    }
    
    // 输出到文件
    if (toFile_ && file_.is_open()) {
        file_ << logLine << std::endl;
        file_.flush();
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // namespace shrimp
