// material.cpp
// 材质类的实现

#include "material.h"
#include "shader.h"
#include <iostream>

// 定义常用纹理名称常量
const std::string Material::DIFFUSE_MAP = "diffuse";
const std::string Material::NORMAL_MAP = "normal";
const std::string Material::SPECULAR_MAP = "specular";

// 默认构造函数
Material::Material() : surface(Surface()), shader(nullptr) {
}

// 使用表面属性构造
Material::Material(const Surface& surface) : surface(surface), shader(nullptr) {
}

// 设置表面属性
void Material::setSurface(const Surface& surface) {
    this->surface = surface;
}

// 获取表面属性
const Surface& Material::getSurface() const {
    return surface;
}

// 设置着色器
void Material::setShader(std::shared_ptr<IShader> shader) {
    this->shader = shader;
}

// 获取着色器
std::shared_ptr<IShader> Material::getShader() const {
    return shader;
}

// 添加纹理
void Material::addTexture(const std::string& name, std::shared_ptr<Texture> texture) {
    if (!texture) {
        std::cerr << "警告: 尝试添加空纹理 '" << name << "'" << std::endl;
        return;
    }
    
    textures[name] = texture;
}

// 获取纹理
std::shared_ptr<Texture> Material::getTexture(const std::string& name) const {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second;
    }
    return nullptr;
}

// 检查是否有指定纹理
bool Material::hasTexture(const std::string& name) const {
    return textures.find(name) != textures.end();
}

// 设置漫反射贴图
void Material::setDiffuseMap(std::shared_ptr<Texture> texture) {
    
    addTexture(DIFFUSE_MAP, texture);
}

// 设置法线贴图
void Material::setNormalMap(std::shared_ptr<Texture> texture) {
    addTexture(NORMAL_MAP, texture);
}

// 设置镜面反射贴图
void Material::setSpecularMap(std::shared_ptr<Texture> texture) {
    addTexture(SPECULAR_MAP, texture);
}

// 获取漫反射贴图
std::shared_ptr<Texture> Material::getDiffuseMap() const {
    return getTexture(DIFFUSE_MAP);
}

// 获取法线贴图
std::shared_ptr<Texture> Material::getNormalMap() const {
    return getTexture(NORMAL_MAP);
}

// 获取镜面反射贴图
std::shared_ptr<Texture> Material::getSpecularMap() const {
    return getTexture(SPECULAR_MAP);
}

// 创建默认材质
std::shared_ptr<Material> createDefaultMaterial() {
    auto material = std::make_shared<Material>();
    material->setShader(createPhongShader());
    return material;
}

// 创建带着色器的材质
std::shared_ptr<Material> createMaterial(const Surface& surface, std::shared_ptr<IShader> shader) {
    auto material = std::make_shared<Material>(surface);
    material->setShader(shader);
    return material;
}

// 创建带纹理的材质
std::shared_ptr<Material> createTexturedMaterial(
    const std::string& diffuseMapPath,
    const std::string& normalMapPath,
    const Surface& surface) {
    
    auto material = std::make_shared<Material>(surface);
    
    // 加载漫反射贴图
    if (!diffuseMapPath.empty()) {
        auto diffuseMap = loadTexture(diffuseMapPath, TextureType::DIFFUSE);
        if (diffuseMap) {
            material->setDiffuseMap(diffuseMap);
        }
    }
    
    // 加载法线贴图
    if (!normalMapPath.empty()) {
        auto normalMap = loadTexture(normalMapPath, TextureType::NORMAL);
        if (normalMap) {
            material->setNormalMap(normalMap);
        }
    }
    
    // 根据是否有法线贴图选择着色器
    if (material->hasTexture(Material::NORMAL_MAP)) {
        // 如果有法线贴图，使用支持法线贴图的着色器
        // 注意：需要实现这个着色器，这里暂时使用Phong着色器
        material->setShader(createPhongShader());
    } else if (material->hasTexture(Material::DIFFUSE_MAP)) {
        // 如果只有漫反射贴图，使用支持纹理的着色器
        material->setShader(createPhongShader());
    } else {
        // 如果没有纹理，使用基本的Phong着色器
        material->setShader(createPhongShader());
    }
    
    return material;
}