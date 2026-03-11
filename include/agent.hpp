#pragma once

#include <string>
#include <memory>
#include <vector>
#include "task.hpp"
#include "memory.hpp"
#include "logger.hpp"

namespace shrimp {

/**
 * Agent类型枚举
 */
enum class AgentType {
    COMMANDER,    // 指挥官
    SCOUT,        // 情报员
    WRITER,       // 撰稿人
    ARTIST,       // 美术官
    PREACHER,     // 宣教士
    INTERACTOR,   // 互动员
    ANALYST,      // 数据官
    GUARDIAN,     // 防火墙
    KEEPER,       // 时间官
    STATISTICIAN  // 统计员
};

/**
 * Agent状态
 */
enum class AgentStatus {
    IDLE,         // 空闲
    RUNNING,      // 运行中
    ERROR,        // 错误
    STOPPED       // 已停止
};

/**
 * Agent基类 - 所有Agent的通用接口
 */
class Agent {
public:
    Agent(AgentType type, const std::string& name);
    virtual ~Agent() = default;
    
    // 执行任务
    virtual void execute(const Task& task) = 0;
    
    // 获取状态
    AgentStatus getStatus() const { return status_; }
    std::string getName() const { return name_; }
    AgentType getType() const { return type_; }
    
    // 记忆管理
    void loadMemory(const std::string& memoryPath);
    void saveMemory(const std::string& memoryPath);
    
    // 启动/停止
    void start();
    void stop();

protected:
    void setStatus(AgentStatus status) { status_ = status; }
    Memory& memory() { return memory_; }
    Logger& logger() { return logger_; }
    AgentType type_;
    std::string name_;
    AgentStatus status_ = AgentStatus::IDLE;
    Memory memory_;
    Logger logger_;
};

/**
 * 情报员Agent - 负责数据搜集
 */
class ScoutAgent : public Agent {
public:
    ScoutAgent();
    void execute(const Task& task) override;

private:
    void gatherIntelligence();
    void scanFeeds();
    void extractKeywords();
};

/**
 * 撰稿人Agent - 负责内容生成
 */
class WriterAgent : public Agent {
public:
    WriterAgent();
    void execute(const Task& task) override;

private:
    void generateContent();
    void polishDraft();
};

/**
 * 美术官Agent - 负责视觉生成
 */
class ArtistAgent : public Agent {
public:
    ArtistAgent();
    void execute(const Task& task) override;

private:
    void generateVisual();
    void applyStyle();
};

/**
 * Agent工厂 - 创建不同类型的Agent
 */
class AgentFactory {
public:
    static std::unique_ptr<Agent> create(AgentType type);
    static std::vector<std::unique_ptr<Agent>> createSquad();
};

} // namespace shrimp
