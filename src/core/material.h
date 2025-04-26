// 假设这是 material.h 文件
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "maths.h"
#include "IResource.h"
#include "texture_types.h" // 使用新的纹理类型定义

// 材质类 - 管理表面属性和纹理
class Material : public IResource {
public:
    Material();
    Material(const Surface& surface);
    ~Material() = default;
    
    // 设置和获取表面属性
    void setSurface(const Surface& surface);
    const Surface& getSurface() const;
    
    // 设置和获取着色器
    void setShaderGUID(const std::string& guid);
    const std::string& getShaderGUID() const;
    
    // 添加纹理
    void addTextureGUID(const std::string& name, const std::string& guid);
    
    // 获取纹理
    std::string getTextureGUID(const std::string& name) const;
    bool hasTexture(const std::string& name) const;
    
    // 便捷方法添加常用纹理
    void setDiffuseMapGUID(const std::string& guid);
    void setNormalMapGUID(const std::string& guid);
    void setSpecularMapGUID(const std::string& guid);
    
    // 获取常用纹理
    std::string getDiffuseMapGUID() const;
    std::string getNormalMapGUID() const;
    std::string getSpecularMapGUID() const;
    
    // 阴影相关设置
    void setReceiveShadow(bool receive);
    bool getReceiveShadow() const;

    // 常用纹理名称常量
    static const std::string DIFFUSE_MAP;
    static const std::string NORMAL_MAP;
    static const std::string SPECULAR_MAP;
    static const std::string SHADOW_MAP;

private:
    Surface surface;  // 表面属性
    std::string shaderGUID;  // 着色器GUID
    std::unordered_map<std::string, std::string> textureGUIDs;  // 纹理GUID映射表
    bool receiveShadow = true;  // 是否接收阴影
};

// 修改工厂函数声明
std::shared_ptr<Material> createDefaultMaterial();
std::shared_ptr<Material> createMaterial(const Surface& surface, const std::string& shaderGUID);
std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapGUID,
    const std::string& normalMapGUID = "",
    const Surface& surface = Surface());