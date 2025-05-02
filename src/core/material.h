#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "maths.h"
#include "IResource.h"
#include "texture_types.h"

/**
 * 材质类 - 管理表面属性和纹理
 * 负责处理材质相关的表面属性、着色器关联和纹理映射
 */
class Material : public IResource {
public:
    // 常用纹理名称常量
    static const std::string DIFFUSE_MAP;
    static const std::string NORMAL_MAP;
    static const std::string SPECULAR_MAP;
    static const std::string SHADOW_MAP;

    // 构造函数和析构函数
    Material();
    explicit Material(const Surface& surface);
    ~Material() = default;
    
    // 表面属性相关方法
    void setSurface(const Surface& surface);
    const Surface& getSurface() const;
    
    // 着色器相关方法
    void setShaderGUID(const std::string& guid);
    const std::string& getShaderGUID() const;
    
    // 纹理管理方法
    void addTextureGUID(const std::string& name, const std::string& guid);
    std::string getTextureGUID(const std::string& name) const;
    bool hasTexture(const std::string& name) const;
    
    // 常用纹理便捷方法
    void setDiffuseMapGUID(const std::string& guid);
    void setNormalMapGUID(const std::string& guid);
    void setSpecularMapGUID(const std::string& guid);
    std::string getDiffuseMapGUID() const;
    std::string getNormalMapGUID() const;
    std::string getSpecularMapGUID() const;
    
    // 阴影相关设置
    void setReceiveShadow(bool receive);
    bool getReceiveShadow() const;

private:
    Surface surface;                                     // 表面属性
    std::string shaderGUID;                              // 着色器GUID
    std::unordered_map<std::string, std::string> textureGUIDs;  // 纹理GUID映射表
    bool receiveShadow{true};                            // 是否接收阴影
};

// 材质工厂函数
std::shared_ptr<Material> createDefaultMaterial();
std::shared_ptr<Material> createMaterial(const Surface& surface, const std::string& shaderGUID);
std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapGUID,
    const std::string& normalMapGUID = "",
    const Surface& surface = Surface());