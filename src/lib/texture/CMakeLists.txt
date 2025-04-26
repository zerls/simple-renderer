# Texture库的CMakeLists.txt
add_library(texture STATIC
    texture.cpp
    texture_io.cpp
    texture_sampler.cpp
    texture_utils.cpp
)

# 添加头文件目录
target_include_directories(texture PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../
)