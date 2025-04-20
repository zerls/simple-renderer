
// shader.cpp
// 着色器实现文件

#include "shader.h"

// 创建基础着色器
std::shared_ptr<IShader> createBasicShader() {
    return std::make_shared<BasicShader>();
}

// 创建Phong着色器
std::shared_ptr<IShader> createPhongShader() {
    return std::make_shared<PhongShader>();
}

// 创建Toon着色器
std::shared_ptr<IShader> createToonShader() {
    return std::make_shared<ToonShader>();
}
