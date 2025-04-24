// texture.h修改
#pragma once

#include <string>
#include <vector>
#include <memory>
#include "common.h"
#include "IResource.h"

// 纹理类型枚举
enum class TextureType {
    DIFFUSE,    // 漫反射贴图
    NORMAL,     // 法线贴图
    SPECULAR,   // 镜面反射贴图
    ROUGHNESS,  // 粗糙度贴图
    EMISSIVE,   // 自发光贴图
    SHADOW      // 阴影贴图(新增)
    // 可以根据需要添加更多类型
};

// 纹理类
class Texture : public IResource {
public:
    Texture();
    ~Texture() = default;
    
    // 从TGA文件加载纹理
    bool loadFromTGA(const std::string& filename);
    
    // 获取纹理颜色（使用UV坐标）- 返回float4
    float4 sample(float2 uv) const;
    
    // 阴影贴图专用采样方法
    float sampleShadowMap(float2 uv) const;
    
    // Getters
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    const std::vector<uint8_t>& getData() const { return data; }
    TextureType getType() const { return textureType; }
    
    // Setters
    void setType(TextureType type) { this->textureType = type; }

    //TODO 没有封装
    int width = 0;
    int height = 0;
    int channels = 0;  // 颜色通道数（通常为3或4）
    std::vector<uint8_t> data;
    
private:
    TextureType textureType = TextureType::DIFFUSE;  // 默认为漫反射贴图
    
    // 不同采样模式实现
    float4 sampleNearest(float u, float v) const;
    float4 sampleBilinear(float u, float v) const;
};

// 加载TGA文件函数
std::shared_ptr<Texture> loadTexture(const std::string& filename, TextureType type = TextureType::DIFFUSE);