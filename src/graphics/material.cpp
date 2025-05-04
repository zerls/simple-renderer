#include "material.h"
#include "shader.h"
#include <iostream>

// 静态常量定义
const std::string Material::DIFFUSE_MAP = "diffuse";
const std::string Material::NORMAL_MAP = "normal";
const std::string Material::SPECULAR_MAP = "specular";
const std::string Material::SHADOW_MAP = "shadow";

//===========================
// 构造函数实现
//===========================

Material::Material() 
    : IResource(ResourceType::MATERIAL), 
      surface(Surface()), 
      receiveShadow(true) {
}

Material::Material(const Surface& surface) 
    : IResource(ResourceType::MATERIAL), 
      surface(surface), 
      receiveShadow(true) {
}

//===========================
// 表面属性相关方法实现
//===========================

void Material::setSurface(const Surface& surface) {
    this->surface = surface;
}

const Surface& Material::getSurface() const {
    return surface;
}

//===========================
// 着色器相关方法实现
//===========================

void Material::setShaderGUID(const std::string& guid) {
    this->shaderGUID = guid;
}

const std::string& Material::getShaderGUID() const {
    return shaderGUID;
}

//===========================
// 纹理管理方法实现
//===========================

void Material::addTextureGUID(const std::string& name, const std::string& guid) {
    if (guid.empty()) {
        std::cerr << "警告: 尝试添加空纹理GUID '" << name << "'" << std::endl;
        return;
    }
    
    textureGUIDs[name] = guid;
}

std::string Material::getTextureGUID(const std::string& name) const {
    auto it = textureGUIDs.find(name);
    if (it != textureGUIDs.end()) {
        return it->second;
    }
    return "";
}

bool Material::hasTexture(const std::string& name) const {
    return textureGUIDs.find(name) != textureGUIDs.end() && !textureGUIDs.at(name).empty();
}

//===========================
// 常用纹理便捷方法实现
//===========================

void Material::setDiffuseMapGUID(const std::string& guid) {
    addTextureGUID(DIFFUSE_MAP, guid);
}

void Material::setNormalMapGUID(const std::string& guid) {
    addTextureGUID(NORMAL_MAP, guid);
}

void Material::setSpecularMapGUID(const std::string& guid) {
    addTextureGUID(SPECULAR_MAP, guid);
}

std::string Material::getDiffuseMapGUID() const {
    return getTextureGUID(DIFFUSE_MAP);
}

std::string Material::getNormalMapGUID() const {
    return getTextureGUID(NORMAL_MAP);
}

std::string Material::getSpecularMapGUID() const {
    return getTextureGUID(SPECULAR_MAP);
}

//===========================
// 阴影相关设置实现
//===========================

void Material::setReceiveShadow(bool receive) {
    this->receiveShadow = receive;
}

bool Material::getReceiveShadow() const {
    return receiveShadow;
}

//===========================
// 材质工厂函数实现
//===========================

std::shared_ptr<Material> createDefaultMaterial() {
    return std::make_shared<Material>();
}

std::shared_ptr<Material> createMaterial(const Surface& surface, const std::string& shaderGUID) {
    auto material = std::make_shared<Material>(surface);
    material->setShaderGUID(shaderGUID);
    return material;
}

std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapGUID,
    const std::string& normalMapGUID,
    const Surface& surface) {
    
    auto material = std::make_shared<Material>(surface);
    
    if (!diffuseMapGUID.empty()) {
        material->setDiffuseMapGUID(diffuseMapGUID);
    }
    
    if (!normalMapGUID.empty()) {
        material->setNormalMapGUID(normalMapGUID);
    }
    
    return material;
}