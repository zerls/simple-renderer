
// shader.cpp
// 着色器实现文件

#include "shader.h"

float smoothstep(float edge0, float edge1, float x) {
    // Clamp x to the range [0, 1]
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    
    // Apply smoothstep formula: 3t² - 2t³
    return t * t * (3 - 2 * t);
}

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
