# test/texture_test/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(TextureTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找源文件
set(TEXTURE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib/texture/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib/texture/texture_io.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib/texture/texture_sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib/texture/texture_utils.cpp
    # 添加其他texture相关源文件
)

# 添加头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lib
    # 添加其他必要的头文件目录
)

# 创建测试可执行文件
add_executable(texture_test texture_test.cpp ${TEXTURE_SOURCES})

# 如果需要链接其他库
# target_link_libraries(texture_test 其他库)