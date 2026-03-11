#include "agent.hpp"
#include <iostream>
#include <chrono>

namespace shrimp {

// ========== Agent 基类 ==========

Agent::Agent(AgentType type, const std::string& name)
    : type_(type), name_(name), logger_("agent." + name) 
{
    logger_.info("Agent created: " + name);
}

void Agent::loadMemory(const std::string& memoryPath) {
    memory_.load(memoryPath);
    logger_.info("Memory loaded from: " + memoryPath);
}

void Agent::saveMemory(const std::string& memoryPath) {
    memory_.save(memoryPath);
    logger_.info("Memory saved to: " + memoryPath);
}

void Agent::start() {
    setStatus(AgentStatus::RUNNING);
    logger_.info("Agent started");
}

void Agent::stop() {
    setStatus(AgentStatus::STOPPED);
    logger_.info("Agent stopped");
}

// ========== 情报员 ==========

ScoutAgent::ScoutAgent() 
    : Agent(AgentType::SCOUT, "scout") {}

void ScoutAgent::execute(const Task& task) {
    logger_.info("Executing task: " + task.name);
    setStatus(AgentStatus::RUNNING);
    
    gatherIntelligence();
    scanFeeds();
    extractKeywords();
    
    // 保存记忆
    memory().addTimeline("情报收集完成: " + task.name);
    
    setStatus(AgentStatus::IDLE);
    logger_.info("Task completed: " + task.name);
}

void ScoutAgent::gatherIntelligence() {
    logger_.info("Gathering intelligence...");
    // 模拟情报搜集
    memory().set("last_scan", 
        std::to_string(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now())));
}

void ScoutAgent::scanFeeds() {
    logger_.debug("Scanning feeds...");
}

void ScoutAgent::extractKeywords() {
    logger_.debug("Extracting keywords...");
}

// ========== 撰稿人 ==========

WriterAgent::WriterAgent() 
    : Agent(AgentType::WRITER, "writer") {}

void WriterAgent::execute(const Task& task) {
    logger_.info("Executing task: " + task.name);
    setStatus(AgentStatus::RUNNING);
    
    generateContent();
    polishDraft();
    
    memory().addTimeline("内容生成完成: " + task.name);
    
    setStatus(AgentStatus::IDLE);
    logger_.info("Task completed: " + task.name);
}

void WriterAgent::generateContent() {
    logger_.info("Generating content...");
    memory().set("draft_count", 
        std::to_string(std::stoi(memory().get("draft_count")) + 1));
}

void WriterAgent::polishDraft() {
    logger_.debug("Polishing draft...");
}

// ========== 美术官 ==========

ArtistAgent::ArtistAgent() 
    : Agent(AgentType::ARTIST, "artist") {}

void ArtistAgent::execute(const Task& task) {
    logger_.info("Executing task: " + task.name);
    setStatus(AgentStatus::RUNNING);
    
    generateVisual();
    applyStyle();
    
    memory().addTimeline("视觉生成完成: " + task.name);
    
    setStatus(AgentStatus::IDLE);
    logger_.info("Task completed: " + task.name);
}

void ArtistAgent::generateVisual() {
    logger_.info("Generating visual...");
    memory().set("artwork_count", 
        std::to_string(std::stoi(memory().get("artwork_count")) + 1));
}

void ArtistAgent::applyStyle() {
    logger_.debug("Applying style...");
}

// ========== Agent工厂 ==========

std::unique_ptr<Agent> AgentFactory::create(AgentType type) {
    switch (type) {
        case AgentType::SCOUT:
            return std::make_unique<ScoutAgent>();
        case AgentType::WRITER:
            return std::make_unique<WriterAgent>();
        case AgentType::ARTIST:
            return std::make_unique<ArtistAgent>();
        default:
            return nullptr;
    }
}

std::vector<std::unique_ptr<Agent>> AgentFactory::createSquad() {
    std::vector<std::unique_ptr<Agent>> squad;
    squad.push_back(std::make_unique<ScoutAgent>());
    squad.push_back(std::make_unique<WriterAgent>());
    squad.push_back(std::make_unique<ArtistAgent>());
    return squad;
}

} // namespace shrimp
