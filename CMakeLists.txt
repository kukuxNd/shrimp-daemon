cmake_minimum_required(VERSION 3.14)
project(ShrimpDaemon VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 编译选项 - 启用所有优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic 
        -O3 -march=native  # 最大优化
        -DNDEBUG
    )
    # 启用链接时优化
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# 查找线程库
find_package(Threads REQUIRED)

# ========== 核心库 ==========
set(CORE_SOURCES
    src/daemon.cpp
    src/scheduler.cpp
    src/agent.cpp
    src/task.cpp
    src/memory.cpp
    src/logger.cpp
    src/config.cpp
    src/metrics.cpp
)

# ========== 主程序 ==========
add_executable(shrimp_daemon ${CORE_SOURCES} src/main.cpp)

target_include_directories(shrimp_daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(shrimp_daemon PRIVATE
    Threads::Threads
)

# ========== 压力测试程序 ==========
add_executable(shrimp_stress src/stress_test.cpp)

target_include_directories(shrimp_stress PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(shrimp_stress PRIVATE
    Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(shrimp_stress PRIVATE
        -O3 -march=native
    )
endif()

# ========== 安装目标 ==========
install(TARGETS shrimp_daemon shrimp_stress DESTINATION bin)

# ========== 测试 ==========
enable_testing()

add_test(NAME demo
    COMMAND shrimp_stress
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME help
    COMMAND shrimp_daemon --help
)
