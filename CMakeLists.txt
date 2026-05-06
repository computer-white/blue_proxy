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
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== 重点：输出目录设置必须在创建目标之前 =====
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 动态库
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)  # 静态库

include_directories(${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include)

# 打印确认（调试用）
message(STATUS "可执行文件输出目录: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "动态库输出目录: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "静态库输出目录: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

find_library(YAML_CPP yaml-cpp PATHS /usr/lib/x86_64-linux-gnu)
find_library(PTHRED pthread PTHRED)
# 添加yaml-cpp所在文件目录
# link_directories(/usr/lib/x86_64-linux-gnu)

# 头文件
include_directories(/usr/include) # yaml-cpp头文件
include_directories(.)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/include)  # llhttp头文件

# llhttp 源文件（稍后会添加到库中）
set(LLHTTP_SRC
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/llhttp.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/api.c
    ${PROJECT_SOURCE_DIR}/thirdparty/llhttp-release-v9.2.0/src/http.c
)

# 设置源文件变量
set(LIB_SRC 
    blue/address.cpp
    blue/bytearray.cpp
    blue/config.cpp
    blue/fdmanager.cpp
    blue/fiber.cpp
    blue/hook.cpp
    blue/httprequest.cpp
    blue/iomanager.cpp
    blue/log.cpp
    blue/msocket.cpp
    blue/mthread.cpp
    blue/schedular.cpp
    blue/timer.cpp
    blue/util.cpp
    http/http.cpp
    http/httpParser.cpp
    ${LLHTTP_SRC}
    # 如果有更多源文件，在这里添加
    # blue/logger.cpp
    # blue/logappender.cpp
)

# 检查blue源文件是否存在
macro(check_blue_file file_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/blue/${file_name}.cpp)
        message(FATAL_ERROR "源文件 blue/${file_name}.cpp 不存在！")
    endif()
endmacro()

check_blue_file(log)
check_blue_file(util)
check_blue_file(address)
check_blue_file(bytearray)
check_blue_file(config)
check_blue_file(fdmanager)
check_blue_file(fiber)
check_blue_file(hook)
check_blue_file(httprequest)
check_blue_file(iomanager)
check_blue_file(msocket)
check_blue_file(mthread)
check_blue_file(schedular)
check_blue_file(timer)


# 检查http源文件是否存在
macro(check_http_file file_name)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/http/${file_name}.cpp)
        message(FATAL_ERROR "源文件 blue/${file_name}.cpp 不存在！")
    endif()
endmacro()

check_http_file(http)
check_http_file(httpParser)

# 创建共享库
add_library(blue SHARED ${LIB_SRC})

# 创建静态库
add_library(blue_static STATIC ${LIB_SRC})

# 设置静态库的输出名称
set_target_properties(blue_static PROPERTIES 
    OUTPUT_NAME "blue"  # 生成 libsylar.a
)

# ========== 创建测试可执行文件 ==========
# 为了简洁，定义一个宏来简化重复代码
macro(add_blue_test test_name)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/${test_name}.cpp)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE blue ${YAML_CPP} ${PTHREAD})
        message(STATUS "测试程序 ${test_name} 已添加")
    else()
        # ** 修正点 3：警告信息使用了正确的文件名 **
        message(WARNING "tests/${test_name}.cpp 不存在，跳过测试程序")
    endif()
endmacro()

# 使用宏添加所有测试，清晰又不会写错文件名
add_blue_test(test)
add_blue_test(test_config)
add_blue_test(test_thread)
add_blue_test(test_util)
add_blue_test(test_fiber)
add_blue_test(test_schedular)
add_blue_test(test_iomanager)
add_blue_test(test_hook)
add_blue_test(test_server)
add_blue_test(test_address)
add_blue_test(test_interface)
add_blue_test(test_ipv4)
add_blue_test(test_socket)
add_blue_test(test_bytearray)
add_blue_test(test_http)
add_blue_test(test_httpParser)

# 打印构建信息
message(STATUS "========================")
message(STATUS "项目: ${PROJECT_NAME}")
message(STATUS "源文件: ${LIB_SRC}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================")
