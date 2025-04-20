// scene.cpp
#include "scene.h"
#include <iostream>

// Camera 实现
Camera::Camera()
    : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f),
      fov(3.14159f / 4.0f), aspect(4.0f / 3.0f), nearPlane(0.1f), farPlane(100.0f)
{
}

void Camera::setPosition(const Vec3f& position) {
    this->position = position;
}

void Camera::setTarget(const Vec3f& target) {
    this->target = target;
}

void Camera::setUp(const Vec3f& up) {
    this->up = up;
}

void Camera::setFOV(float fov) {
    this->fov = fov;
}

void Camera::setAspect(float aspect) {
    this->aspect = aspect;
}

void Camera::setNearPlane(float nearPlane) {
    this->nearPlane = nearPlane;
}

void Camera::setFarPlane(float farPlane) {
    this->farPlane = farPlane;
}

Matrix4x4f Camera::getViewMatrix() const {
    return Matrix4x4f::lookAt(position, target, up);
}

Matrix4x4f Camera::getProjectionMatrix() const {
    return Matrix4x4f::perspective(fov, aspect, nearPlane, farPlane);
}

// Scene 实现
Scene::Scene()
    : light(Vec3f(0.0f, 10.0f, 10.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f)
{
}

void Scene::addObject(const SceneObject& object) {
    objects.push_back(object);
}

void Scene::removeObject(size_t index) {
    if (index < objects.size()) {
        objects.erase(objects.begin() + index);
    }
}

void Scene::removeObject(const std::string& name) {
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (it->name == name) {
            objects.erase(it);
            return;
        }
    }
}

SceneObject Scene::loadModel(const std::string& objFile, const std::string& name, 
                          std::shared_ptr<Material> material, const Matrix4x4f& transform) {
    // 加载OBJ模型
    auto mesh = loadOBJ(objFile);
    if (!mesh) {
        std::cerr << "无法加载模型: " << objFile << std::endl;
        return SceneObject();
    }
    
    // 创建场景对象
    SceneObject obj(name, mesh, material, transform);
    return obj;
}

std::shared_ptr<Material> Scene::createMaterialWithTextures(
    const std::string& name,
    const std::string& diffuseMap,
    const std::string& normalMap,
    const float3& baseColor,
    float shininess) {
    
    // 创建表面属性
    Surface surface;
    surface.ambient = float3(0.1f, 0.1f, 0.1f);
    surface.diffuse = baseColor;
    surface.specular = float3(0.5f, 0.5f, 0.5f);
    surface.shininess = shininess;
    
    // 创建材质
    auto material = std::make_shared<Material>(surface);
    
    // 加载纹理
    if (!diffuseMap.empty()) {
        auto texture = loadTexture(diffuseMap, TextureType::DIFFUSE);
        if (texture) 
            material->setDiffuseMap(texture);
        
    }
    
    if (!normalMap.empty()) {
        auto texture = loadTexture(normalMap, TextureType::NORMAL);
        if (texture) 
            material->setNormalMap(texture);
    
    }
    
    // 选择合适的着色器 - 根据贴图类型
    if (material->hasTexture(Material::NORMAL_MAP)) {
        // 使用支持法线贴图的着色器
        material->setShader(createTexturedPhongShader()); // 需要扩展为支持法线贴图的着色器
    } else if (material->hasTexture(Material::DIFFUSE_MAP)) {
        // 使用支持纹理的着色器
        material->setShader(createTexturedPhongShader(material->getDiffuseMap()));
    } else {
        // 使用基本着色器
        material->setShader(createPhongShader());
    }
    
    return material;
}

void Scene::render(Renderer& renderer) {
    // 应用相机设置
    renderer.setViewMatrix(camera.getViewMatrix());
    renderer.setProjMatrix(camera.getProjectionMatrix());
    renderer.setEye(camera.getPosition());
    
    // 设置光源
    renderer.setLight(light);
    
    // 清屏
    renderer.clear(Color(40, 40, 60)); // 深蓝灰色背景
    
    // 渲染所有对象
    for (const auto& obj : objects) {
        // 设置模型变换
        renderer.setModelMatrix(obj.modelMatrix);
        
        // 使用对象的材质渲染网格
        renderer.drawMesh(obj.mesh, obj.material);
    }
}