#include "task.hpp"
#include <sstream>
#include <iostream>

namespace shrimp {

// ========== CronExpr 实现 ==========

CronExpr CronExpr::parse(const std::string& expr) {
    CronExpr result;
    std::istringstream iss(expr);
    std::string token;
    
    // 简化解析: 支持 "*/n" 或 "n" 格式
    int field = 0;
    while (std::getline(iss, token, ' ') && field < 5) {
        if (token == "*") {
            // -1 表示任意
        } else if (token.substr(0, 2) == "*/") {
            // */n 格式 - 这里简化处理
            field++;
        } else {
            try {
                int val = std::stoi(token);
                switch (field) {
                    case 0: result.minute = val; break;
                    case 1: result.hour = val; break;
                    case 2: result.dayOfMonth = val; break;
                    case 3: result.month = val; break;
                    case 4: result.dayOfWeek = val; break;
                }
            } catch (...) {}
            field++;
        }
    }
    
    return result;
}

bool CronExpr::matches(const std::tm& time) const {
    if (minute >= 0 && time.tm_min != minute) return false;
    if (hour >= 0 && time.tm_hour != hour) return false;
    if (dayOfMonth >= 0 && time.tm_mday != dayOfMonth) return false;
    if (month >= 0 && time.tm_mon + 1 != month) return false;
    if (dayOfWeek >= 0 && time.tm_wday != dayOfWeek) return false;
    return true;
}

// ========== CronTask 实现 ==========

CronTask::CronTask(const std::string& n, const std::string& expr)
    : Task(n), cronExpr(CronExpr::parse(expr)) {}

} // namespace shrimp
