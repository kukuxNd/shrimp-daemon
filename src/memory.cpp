#include "memory.hpp"
#include <algorithm>
#include <iostream>

namespace shrimp {

Memory::Memory() = default;

void Memory::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::string Memory::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return "";
}

bool Memory::has(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void Memory::addTimeline(const std::string& event) {
    timeline_.push_back({std::chrono::system_clock::now(), event});
}

std::vector<std::string> Memory::getTimeline(int limit) const {
    std::vector<std::string> result;
    int count = 0;
    for (auto it = timeline_.rbegin(); 
         it != timeline_.rend() && count < limit; 
         ++it, ++count) {
        result.push_back(it->second);
    }
    return result;
}

void Memory::save(const std::string& path) {
    std::ofstream file(path);
    if (file.is_open()) {
        file << serialize();
        file.close();
    }
}

void Memory::load(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        deserialize(content);
        file.close();
    }
}

void Memory::prune(int maxDays) {
    auto cutoff = std::chrono::system_clock::now() - 
                  std::chrono::hours(maxDays * 24);
    timeline_.erase(
        std::remove_if(timeline_.begin(), timeline_.end(),
            [cutoff](const auto& entry) {
                return entry.first < cutoff;
            }),
        timeline_.end());
}

std::vector<std::string> Memory::keys() const {
    std::vector<std::string> result;
    for (const auto& [key, _] : data_) {
        result.push_back(key);
    }
    return result;
}

std::string Memory::serialize() const {
    std::ostringstream oss;
    oss << "[DATA]\n";
    for (const auto& [key, value] : data_) {
        oss << key << "=" << value << "\n";
    }
    oss << "\n[TIMELINE]\n";
    for (const auto& [time, event] : timeline_) {
        auto timeT = std::chrono::system_clock::to_time_t(time);
        oss << timeT << "|" << event << "\n";
    }
    return oss.str();
}

void Memory::deserialize(const std::string& content) {
    std::istringstream iss(content);
    std::string line;
    bool inData = false;
    bool inTimeline = false;
    
    while (std::getline(iss, line)) {
        if (line == "[DATA]") {
            inData = true;
            inTimeline = false;
        } else if (line == "[TIMELINE]") {
            inData = false;
            inTimeline = true;
        } else if (inData && !line.empty()) {
            auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                data_[key] = value;
            }
        } else if (inTimeline && !line.empty()) {
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string timeStr = line.substr(0, pos);
                std::string event = line.substr(pos + 1);
                try {
                    auto timeT = std::stoll(timeStr);
                    std::chrono::system_clock::time_point tp = 
                        std::chrono::system_clock::from_time_t(timeT);
                    timeline_.push_back({tp, event});
                } catch (...) {}
            }
        }
    }
}

} // namespace shrimp
