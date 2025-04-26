// scene.h
// 定义场景类，管理场景中的对象和相机

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "maths.h"
#include "mesh.h"
#include "shader.h"
#include "renderer.h"
#include "material.h"
#include "IResource.h"
#include "texture.h"      // 使用新的纹理库
#include "texture_types.h" // 使用新的纹理类型定义

// 场景对象，包含网格和材质
// 场景对象，包含网格和材质
struct SceneObject {
    std::string name;                 // 对象名称
    std::string meshGUID;             // 网格GUID
    std::string materialGUID;         // 材质GUID
    Matrix4x4f modelMatrix;           // 模型矩阵
    bool castShadow;                  // 是否投射阴影
    bool receiveShadow;               // 是否接收阴影
    
    SceneObject() : castShadow(true), receiveShadow(true) {}
    SceneObject(const std::string& name, const std::string& meshGUID, 
                const std::string& materialGUID, const Matrix4x4f& matrix = Matrix4x4f::identity(),
                bool castShadow = true, bool receiveShadow = true)
        : name(name), meshGUID(meshGUID), materialGUID(materialGUID), 
          modelMatrix(matrix), castShadow(castShadow), receiveShadow(receiveShadow) {}
};

// 相机类
class Camera {
    public:
        Camera();
        
        // 设置相机参数（保持不变）
        void setPosition(const Vec3f& position);
        void setTarget(const Vec3f& target);
        void setUp(const Vec3f& up);
        void setFOV(float fov);
        void setAspect(float aspect);
        void setNearPlane(float nearPlane);
        void setFarPlane(float farPlane);
        
        // 获取相机参数（保持不变）
        Vec3f getPosition() const { return position; }
        Vec3f getTarget() const { return target; }
        Vec3f getUp() const { return up; }
        float getFOV() const { return fov; }
        float getAspect() const { return aspect; }
        float getNearPlane() const { return nearPlane; }
        float getFarPlane() const { return farPlane; }
        
        // 计算视图矩阵和投影矩阵（保持不变）
        Matrix4x4f getViewMatrix() const;
        Matrix4x4f getProjectionMatrix() const;
        
    private:
        Vec3f position;    // 相机位置
        Vec3f target;      // 观察目标点
        Vec3f up;          // 上方向向量
        float fov;         // 视野角度（弧度）
        float aspect;      // 宽高比
        float nearPlane;   // 近平面距离
        float farPlane;    // 远平面距离
    };
    

// 场景类
class Scene {
    public:
        Scene();
        ~Scene() = default;
        
        // 相机管理（保持不变）
        Camera& getCamera() { return camera; }
        const Camera& getCamera() const { return camera; }
        
        // 光源管理
        void setLight(const Light& light) { this->light = light; }
        Light getLight() const { return light; }
        
        // 场景对象管理
        void addObject(const SceneObject& object);
        void removeObject(size_t index);
        void removeObject(const std::string& name);
        size_t getObjectCount() const { return objects.size(); }
        SceneObject& getObject(size_t index) { return objects[index]; }
        const SceneObject& getObject(size_t index) const { return objects[index]; }
        
        // 资源管理
        ResourceManager& getResourceManager() { return resourceManager; }
        
        // 获取网格
        std::shared_ptr<Mesh> getMesh(const std::string& guid);
        
        // 获取材质
        std::shared_ptr<Material> getMaterial(const std::string& guid);
        
        // 获取纹理
        std::shared_ptr<Texture> getTexture(const std::string& guid);
        
        // 获取着色器
        std::shared_ptr<IShader> getShader(const std::string& guid);
    
        // 资源加载帮助函数
        std::string loadMesh(const std::string& objFile, const std::string& name);
        
        // 加载纹理
        std::string loadTexture(const std::string& filename, TextureType type = TextureType::COLOR);
        
        // 创建材质
        std::string createMaterial(const std::string& name, const Surface& surface);
        
        // 根据文件名自动加载纹理并创建材质
        std::string createMaterialWithTextures(
            const std::string& name,
            const std::string& diffuseMap = "",
            const std::string& normalMap = "",
            const float3& baseColor = float3(1.0f, 1.0f, 1.0f),
            float shininess = 32.0f);
        
        // 创建着色器
        std::string createShader(const std::string& name, std::shared_ptr<IShader> shader);
        
        // 配置阴影映射
        void setupShadowMapping(bool enabled, int shadowMapSize = 1024);
        
        // 渲染场景
        void render(Renderer& renderer);
        
        // 单独更新和渲染阴影贴图
        void updateShadowMap(Renderer& renderer);
        
    private:
        Camera camera;                      // 场景相机
        Light light;                        // 主光源
        std::vector<SceneObject> objects;   // 场景对象列表
        ResourceManager resourceManager;    // 资源管理器
        
        // 阴影映射相关
        bool shadowMappingEnabled = false;
        int shadowMapSize = 1024;
        std::string shadowMapGUID;
        std::string shadowShaderGUID;
    };