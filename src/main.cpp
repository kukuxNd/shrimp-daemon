#include "daemon.hpp"
#include "config.hpp"
#include <iostream>
#include <csignal>
#include <memory>

using namespace shrimp;

std::unique_ptr<Daemon> g_daemon;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_daemon) {
        g_daemon->stop();
    }
}

void printBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║   🦐 ShrimpDaemon v1.0.0 - 24/7 Agent Framework              ║
║                                                              ║
║   "撒旦的化身，魔女与天使的两面派"                            ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printHelp() {
    std::cout << R"(
Usage: shrimp_daemon [options]

Options:
  -c, --config <path>   配置文件路径 (默认: ./shrimp.conf)
  -d, --daemon          以守护进程模式运行
  -h, --help            显示帮助信息
  -v, --version         显示版本信息

Examples:
  shrimp_daemon                      # 使用默认配置启动
  shrimp_daemon -c /etc/shrimp.conf  # 使用指定配置文件
  shrimp_daemon -d                   # 后台运行

Architecture:
  ┌─────────────────┐
  │  Main Daemon    │
  └────────┬────────┘
           │
    ┌──────┼──────┐
    │      │      │
  ┌──▼──┐ ┌──▼──┐ ┌──▼──┐
  │Scout│ │Writer│ │Artist│
  └─────┘ └──────┘ └──────┘
)" << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // 解析命令行参数
    std::string configPath = "./shrimp.conf";
    bool daemonMode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "ShrimpDaemon version 1.0.0" << std::endl;
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configPath = argv[++i];
            }
        } else if (arg == "-d" || arg == "--daemon") {
            daemonMode = true;
        }
    }
    
    // 加载配置
    Config config(configPath);
    
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 创建并启动守护进程
    g_daemon = std::make_unique<Daemon>(config);
    
    std::cout << "Starting ShrimpDaemon..." << std::endl;
    std::cout << "Config: " << configPath << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    
    g_daemon->start();
    
    // 主线程等待
    while (g_daemon->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    g_daemon.reset();
    std::cout << "ShrimpDaemon stopped." << std::endl;
    return 0;
}
