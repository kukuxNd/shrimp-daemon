#pragma once

#include <string>
#include <vector>
#include <map>

namespace shrimp {

/**
 * Agent配置
 */
struct AgentConfig {
    std::string name;
    std::string type;
    bool enabled = true;
    std::map<std::string, std::string> params;
};

/**
 * 心跳配置
 */
struct HeartbeatConfig {
    int intervalMinutes = 30;
    std::string prompt = "检查系统状态";
};

/**
 * 守护进程配置
 */
class Config {
public:
    Config() = default;
    explicit Config(const std::string& configPath);
    
    // 从文件加载
    void load(const std::string& path);
    
    // 保存到文件
    void save(const std::string& path);
    
    // 获取配置
    std::string get(const std::string& key, const std::string& defaultVal = "") const;
    int getInt(const std::string& key, int defaultVal = 0) const;
    bool getBool(const std::string& key, bool defaultVal = false) const;
    
    // 设置配置
    void set(const std::string& key, const std::string& value);
    
    // Agent配置
    std::vector<AgentConfig> agents;
    HeartbeatConfig heartbeat;
    
    // 全局配置
    std::string workspace = "./workspace";
    std::string logLevel = "info";
    int maxRetries = 3;
    int retryDelayMs = 1000;

private:
    std::map<std::string, std::string> values_;
    void parseLine(const std::string& line);
};

} // namespace shrimp
