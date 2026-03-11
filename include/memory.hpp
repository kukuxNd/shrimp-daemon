#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>

namespace shrimp {

/**
 * 记忆系统 - 持久化Agent状态
 */
class Memory {
public:
    Memory();
    ~Memory() = default;
    
    // 存储键值对
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key) const;
    bool has(const std::string& key) const;
    
    // 添加时间线记录
    void addTimeline(const std::string& event);
    std::vector<std::string> getTimeline(int limit = 10) const;
    
    // 持久化
    void save(const std::string& path);
    void load(const std::string& path);
    
    // 清理旧记忆
    void prune(int maxDays = 30);
    
    // 获取所有键
    std::vector<std::string> keys() const;

private:
    std::map<std::string, std::string> data_;
    std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> timeline_;
    
    std::string serialize() const;
    void deserialize(const std::string& content);
};

} // namespace shrimp
