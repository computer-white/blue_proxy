cmake_minimum_required(VERSION 3.10)
project(blue)


# 可选：显示详细编译信息
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
# ===== 添加相对路径支持 =====
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # 将源码路径映射为 . (当前目录)
    add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.)
    # 同时处理调试信息中的路径
    add_compile_options(-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.)
    message(STATUS "启用相对路径支持，源码根目录: ${CMAKE_SOURCE_DIR}")
endif()
# ===========================
# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== 重点：输出目录设置必须在创建目标之前 =====
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 静态库

# 打印确认（调试用）
message(STATUS "可执行文件输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "动态库输出目录: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "静态库输出目录: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

find_library(YAML_CPP yaml-cpp)
find_library(PTHRED pthread)
# 添加yaml-cpp所在文件目录
link_directories(/usr/lib/x86_64-linux-gnu)

# 头文件
include_directories(/usr/include) # yaml-cpp头文件
include_directories(.)

# 设置源文件变量
set(LIB_SRC 
    blue/config.cpp
    blue/fdmanager.cpp
    blue/fiber.cpp
    blue/hook.cpp
    blue/iomanager.cpp
    blue/log.cpp
    blue/mthread.cpp
    blue/schedular.cpp
    blue/timer.cpp
    blue/util.cpp
    # 如果有更多源文件，在这里添加
    # blue/logger.cpp
    # blue/logappender.cpp
)

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/log.cpp)
    message(FATAL_ERROR "源文件 blue/log.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/util.cpp)
    message(FATAL_ERROR "源文件 blue/util.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/config.cpp)
    message(FATAL_ERROR "源文件 blue/config.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/mthread.cpp)
    message(FATAL_ERROR "源文件 blue/mthread.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/fiber.cpp)
    message(FATAL_ERROR "源文件 blue/fiber.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/schedular.cpp)
    message(FATAL_ERROR "源文件 blue/schedular.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/iomanager.cpp)
    message(FATAL_ERROR "源文件 blue/iomanager.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/timer.cpp)
    message(FATAL_ERROR "源文件 blue/timer.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/hook.cpp)
    message(FATAL_ERROR "源文件 blue/hook.cpp 不存在！")
endif()

# 检查源文件是否存在
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/fdmanager.cpp)
    message(FATAL_ERROR "源文件 blue/fdmanager.cpp 不存在！")
endif()

# 创建共享库
add_library(blue SHARED ${LIB_SRC})

# 创建静态库
add_library(blue_static STATIC ${LIB_SRC})

# 设置静态库的输出名称
set_target_properties(blue_static PROPERTIES 
    OUTPUT_NAME "blue"  # 生成 libsylar.a
)

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp)
    add_executable(test tests/test.cpp)
    target_link_libraries(test PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test.cpp 不存在，跳过测试程序")
endif()

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_config.cpp)
    add_executable(test_config tests/test_config.cpp)
    target_link_libraries(test_config PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库
    # target_link_libraries(test_config PRIVATE yaml-cpp)  # 显式链接
    
    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_config.cpp 不存在，跳过测试程序")
endif()

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_thread.cpp)
    add_executable(test_thread tests/test_thread.cpp)
    target_link_libraries(test_thread PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_thread.cpp 不存在，跳过测试程序")
endif()

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_util.cpp)
    add_executable(test_util tests/test_util.cpp)
    target_link_libraries(test_util PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_util.cpp 不存在，跳过测试程序")
endif()

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_fiber.cpp)
    add_executable(test_fiber tests/test_fiber.cpp)
    target_link_libraries(test_fiber PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_fiber.cpp 不存在，跳过测试程序")
endif()

# 创建测试可执行文件
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_schedular.cpp)
    add_executable(test_schedular tests/test_schedular.cpp)
    target_link_libraries(test_schedular PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_fiber.cpp 不存在，跳过测试程序")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_iomanager.cpp)
    add_executable(test_iomanager tests/test_iomanager.cpp)
    target_link_libraries(test_iomanager PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库
    # target_link_libraries(test_config PRIVATE yaml-cpp)  # 显式链接

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_iomanager.cpp 不存在，跳过测试程序")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_hook.cpp)
    add_executable(test_hook tests/test_hook.cpp)
    target_link_libraries(test_hook PRIVATE blue ${YAML_CPP} ${PTHREAD})  # 链接动态库
    # target_link_libraries(test_config PRIVATE yaml-cpp)  # 显式链接

    message(STATUS "测试程序已添加")
else()
    message(WARNING "tests/test_hook.cpp 不存在，跳过测试程序")
endif()

# 打印构建信息
message(STATUS "========================")
message(STATUS "项目: ${PROJECT_NAME}")
message(STATUS "源文件: ${LIB_SRC}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================")
