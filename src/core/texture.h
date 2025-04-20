// texture.h
// 定义纹理类和TGA文件加载

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "common.h"

// 纹理类型枚举
enum class TextureType {
    DIFFUSE,    // 漫反射贴图
    NORMAL,     // 法线贴图
    SPECULAR,   // 镜面反射贴图
    ROUGHNESS,  // 粗糙度贴图
    EMISSIVE    // 自发光贴图
    // 可以根据需要添加更多类型
};

// 纹理类
class Texture {
public:
    Texture();
    ~Texture() = default;
    
    // 从TGA文件加载纹理
    bool loadFromTGA(const std::string& filename);
    
    // 获取纹理颜色（使用UV坐标）
    Color sample(float2 uv)const;
    
    // Getters
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    const std::vector<uint8_t>& getData() const { return data; }
    TextureType getType() const { return type; }
    
    // Setters
    void setType(TextureType type) { this->type = type; }
    
private:
    int width = 0;
    int height = 0;
    int channels = 0;  // 颜色通道数（通常为3或4）
    std::vector<uint8_t> data;
    TextureType type = TextureType::DIFFUSE;  // 默认为漫反射贴图
    
    // 不同采样模式实现
    Color sampleNearest(float u, float v) const;
    Color sampleBilinear(float u, float v) const;
};

// 加载TGA文件函数
std::shared_ptr<Texture> loadTexture(const std::string& filename, TextureType type = TextureType::DIFFUSE);