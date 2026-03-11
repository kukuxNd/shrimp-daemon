cmake_minimum_required(VERSION 3.14)
project(ShrimpDaemon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# 查找线程库
find_package(Threads REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/daemon.cpp
    src/scheduler.cpp
    src/agent.cpp
    src/task.cpp
    src/memory.cpp
    src/logger.cpp
    src/config.cpp
)

# 可执行文件
add_executable(shrimp_daemon ${SOURCES})

target_include_directories(shrimp_daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(shrimp_daemon PRIVATE
    Threads::Threads
)

# 安装目标
install(TARGETS shrimp_daemon DESTINATION bin)
