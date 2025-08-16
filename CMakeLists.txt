cmake_minimum_required(VERSION 3.10)
project(TinySTL)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(include)

# 自动编译所有沙盒测试
file(GLOB TEST_FILES "sandbox/*.cpp")
foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    message("已添加测试: ${test_name}")
endforeach()

# 添加示例程序
add_executable(example example/basic_usage.cpp)