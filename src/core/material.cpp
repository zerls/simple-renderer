#include "material.h"
#include "shader.h"
#include <iostream>

// 定义常用纹理名称常量
const std::string Material::DIFFUSE_MAP = "diffuse";
const std::string Material::NORMAL_MAP = "normal";
const std::string Material::SPECULAR_MAP = "specular";
const std::string Material::SHADOW_MAP = "shadow";

// 默认构造函数
Material::Material() : IResource(ResourceType::MATERIAL), surface(Surface()), receiveShadow(true) {
}

// 使用表面属性构造
Material::Material(const Surface& surface) : IResource(ResourceType::MATERIAL), surface(surface), receiveShadow(true) {
}

// 设置表面属性
void Material::setSurface(const Surface& surface) {
    this->surface = surface;
}

// 获取表面属性
const Surface& Material::getSurface() const {
    return surface;
}

// 设置着色器GUID
void Material::setShaderGUID(const std::string& guid) {
    this->shaderGUID = guid;
}

// 获取着色器GUID
const std::string& Material::getShaderGUID() const {
    return shaderGUID;
}

// 添加纹理GUID
void Material::addTextureGUID(const std::string& name, const std::string& guid) {
    if (guid.empty()) {
        std::cerr << "警告: 尝试添加空纹理GUID '" << name << "'" << std::endl;
        return;
    }
    
    textureGUIDs[name] = guid;
}

// 获取纹理GUID
std::string Material::getTextureGUID(const std::string& name) const {
    auto it = textureGUIDs.find(name);
    if (it != textureGUIDs.end()) {
        return it->second;
    }
    return "";
}

// 检查是否有指定纹理
bool Material::hasTexture(const std::string& name) const {
    return textureGUIDs.find(name) != textureGUIDs.end() && !textureGUIDs.at(name).empty();
}

// 设置漫反射贴图GUID
void Material::setDiffuseMapGUID(const std::string& guid) {
    addTextureGUID(DIFFUSE_MAP, guid);
}

// 设置法线贴图GUID
void Material::setNormalMapGUID(const std::string& guid) {
    addTextureGUID(NORMAL_MAP, guid);
}

// 设置镜面反射贴图GUID
void Material::setSpecularMapGUID(const std::string& guid) {
    addTextureGUID(SPECULAR_MAP, guid);
}

// 获取漫反射贴图GUID
std::string Material::getDiffuseMapGUID() const {
    return getTextureGUID(DIFFUSE_MAP);
}

// 获取法线贴图GUID
std::string Material::getNormalMapGUID() const {
    return getTextureGUID(NORMAL_MAP);
}

// 获取镜面反射贴图GUID
std::string Material::getSpecularMapGUID() const {
    return getTextureGUID(SPECULAR_MAP);
}

// 设置是否接收阴影
void Material::setReceiveShadow(bool receive) {
    this->receiveShadow = receive;
}

// 获取是否接收阴影
bool Material::getReceiveShadow() const {
    return receiveShadow;
}

// 创建默认材质 - 注意：现在不会创建着色器，而是返回一个空的shaderGUID
std::shared_ptr<Material> createDefaultMaterial() {
    auto material = std::make_shared<Material>();
    // 不设置着色器，让Scene类负责管理着色器
    return material;
}

// 创建带着色器的材质
std::shared_ptr<Material> createMaterial(const Surface& surface, const std::string& shaderGUID) {
    auto material = std::make_shared<Material>(surface);
    material->setShaderGUID(shaderGUID);
    return material;
}

// 创建带纹理的材质
std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapGUID,
    const std::string& normalMapGUID,
    const Surface& surface) {
    
    auto material = std::make_shared<Material>(surface);
    
    // 设置纹理GUID
    if (!diffuseMapGUID.empty()) {
        material->setDiffuseMapGUID(diffuseMapGUID);
    }
    
    if (!normalMapGUID.empty()) {
        material->setNormalMapGUID(normalMapGUID);
    }
    
    // 不设置着色器，让调用者（通常是Scene类）决定使用哪个着色器
    return material;
}