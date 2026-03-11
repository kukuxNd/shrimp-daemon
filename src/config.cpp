#include "config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace shrimp {

Config::Config(const std::string& configPath) {
    load(configPath);
}

void Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open config file: " << path << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        parseLine(line);
    }
    file.close();
}

void Config::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot write config file: " << path << std::endl;
        return;
    }
    
    for (const auto& [key, value] : values_) {
        file << key << "=" << value << "\n";
    }
    file.close();
}

std::string Config::get(const std::string& key, const std::string& defaultVal) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : defaultVal;
}

int Config::getInt(const std::string& key, int defaultVal) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {}
    }
    return defaultVal;
}

bool Config::getBool(const std::string& key, bool defaultVal) const {
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second == "true" || it->second == "1";
    }
    return defaultVal;
}

void Config::set(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void Config::parseLine(const std::string& line) {
    // 跳过空行和注释
    if (line.empty() || line[0] == '#' || line[0] == ';') {
        return;
    }
    
    // 解析 key=value 格式
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // 去除空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        values_[key] = value;
    }
}

} // namespace shrimp
