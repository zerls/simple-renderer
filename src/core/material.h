// material.h
// 定义材质类，用于管理表面属性和纹理

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "common.h"
#include "texture.h"

// 前向声明
class IShader;

// 材质类 - 管理表面属性和纹理
class Material {
public:
    Material();
    Material(const Surface& surface);
    ~Material() = default;
    
    // 设置和获取表面属性
    void setSurface(const Surface& surface);
    const Surface& getSurface() const;
    
    // 设置和获取着色器
    void setShader(std::shared_ptr<IShader> shader);
    std::shared_ptr<IShader> getShader() const;
    
    // 添加纹理
    void addTexture(const std::string& name, std::shared_ptr<Texture> texture);
    
    // 获取纹理
    std::shared_ptr<Texture> getTexture(const std::string& name) const;
    bool hasTexture(const std::string& name) const;
    
    // 便捷方法添加常用纹理
    void setDiffuseMap(std::shared_ptr<Texture> texture);
    void setNormalMap(std::shared_ptr<Texture> texture);
    void setSpecularMap(std::shared_ptr<Texture> texture);
    
    // 获取常用纹理
    std::shared_ptr<Texture> getDiffuseMap() const;
    std::shared_ptr<Texture> getNormalMap() const;
    std::shared_ptr<Texture> getSpecularMap() const;

        // 常用纹理名称常量
        static const std::string DIFFUSE_MAP;
        static const std::string NORMAL_MAP;
        static const std::string SPECULAR_MAP;

    
private:
    Surface surface;  // 表面属性
    std::shared_ptr<IShader> shader;  // 着色器
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;  // 纹理映射表
    

};

// 材质工厂函数
std::shared_ptr<Material> createDefaultMaterial();
std::shared_ptr<Material> createMaterial(const Surface& surface, std::shared_ptr<IShader> shader);
std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapPath,
    const std::string& normalMapPath = "",
    const Surface& surface = Surface());