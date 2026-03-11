#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace shrimp {

/**
 * 日志级别
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

/**
 * 线程安全的日志系统
 */
class Logger {
public:
    explicit Logger(const std::string& name = "shrimp");
    ~Logger();
    
    void setLevel(LogLevel level) { level_ = level; }
    void setLogFile(const std::string& path);
    
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);

private:
    void log(LogLevel level, const std::string& msg);
    std::string levelToString(LogLevel level) const;
    std::string timestamp() const;
    
    std::string name_;
    LogLevel level_ = LogLevel::INFO;
    std::ofstream file_;
    std::mutex mutex_;
    bool toFile_ = false;
};

} // namespace shrimp
