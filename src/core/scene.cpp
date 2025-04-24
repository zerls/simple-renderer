// scene.cpp
#include "scene.h"
#include <iostream>

// Camera 实现
Camera::Camera()
    : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f),
      fov(3.14159f / 4.0f), aspect(4.0f / 3.0f), nearPlane(0.1f), farPlane(100.0f) {}

void Camera::setPosition(const Vec3f &position) { this->position = position; }

void Camera::setTarget(const Vec3f &target) { this->target = target; }

void Camera::setUp(const Vec3f &up) { this->up = up; }

void Camera::setFOV(float fov) { this->fov = fov; }

void Camera::setAspect(float aspect) { this->aspect = aspect; }

void Camera::setNearPlane(float nearPlane) { this->nearPlane = nearPlane; }

void Camera::setFarPlane(float farPlane) { this->farPlane = farPlane; }

Matrix4x4f Camera::getViewMatrix() const { return Matrix4x4f::lookAt(position, target, up); }

Matrix4x4f Camera::getProjectionMatrix() const { return Matrix4x4f::perspective(fov, aspect, nearPlane, farPlane); }

// Scene 实现
Scene::Scene() : light(Vec3f(0.0f, 10.0f, 10.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f) {}

void Scene::addObject(const SceneObject &object)
{
    objects.push_back(object);
}

void Scene::removeObject(size_t index)
{
    if (index < objects.size())
    {
        objects.erase(objects.begin() + index);
    }
}

void Scene::removeObject(const std::string &name)
{
    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->name == name)
        {
            objects.erase(it);
            return;
        }
    }
}

std::shared_ptr<Mesh> Scene::getMesh(const std::string& guid) {
    return resourceManager.getResource<Mesh>(guid);
}

std::shared_ptr<Material> Scene::getMaterial(const std::string& guid) {
    return resourceManager.getResource<Material>(guid);
}

std::shared_ptr<Texture> Scene::getTexture(const std::string& guid) {
    return resourceManager.getResource<Texture>(guid);
}

std::shared_ptr<IShader> Scene::getShader(const std::string& guid) {
    return resourceManager.getResource<IShader>(guid);
}

// 加载模型的实现
std::string Scene::loadMesh(const std::string& objFile, const std::string& name) {
    // 生成GUID（简单地使用文件名和变换矩阵的哈希）
    std::string guid = objFile + "_" + name;
    
    // 检查是否已经加载
    if (resourceManager.hasResource(guid)) {
        return guid;
    }
    
    // 加载OBJ模型
    auto mesh = ::loadOBJ(objFile);
    if (!mesh) {
        std::cerr << "无法加载模型: " << objFile << std::endl;
        return "";
    }
    
    // 将mesh添加到资源管理器
    resourceManager.addResource(mesh, guid);
    
    return guid;
}

// 加载纹理的实现
std::string Scene::loadTexture(const std::string& filename, TextureType type) {
    // 生成GUID（简单地使用文件名和类型）
    std::string guid = filename + "_" + std::to_string(static_cast<int>(type));
    
    // 检查是否已经加载
    if (resourceManager.hasResource(guid)) {
        return guid;
    }
    
    // 加载纹理
    auto texture = ::loadTexture(filename, type);
    if (!texture) {
        std::cerr << "无法加载纹理: " << filename << std::endl;
        return "";
    }
    
    // 将纹理添加到资源管理器
    resourceManager.addResource(texture, guid);
    
    return guid;
}

// 创建材质的实现
std::string Scene::createMaterial(const std::string& name, const Surface& surface) {
    // 生成GUID
    std::string guid = "material_" + name;
    
    // 检查是否已经存在
    if (resourceManager.hasResource(guid)) {
        return guid;
    }
    
    // 创建材质
    auto material = std::make_shared<Material>(surface);
    
    // 将材质添加到资源管理器
    resourceManager.addResource(material, guid);
    
    return guid;
}

// 根据文件名自动加载纹理并创建材质的实现
std::string Scene::createMaterialWithTextures(
    const std::string& name,
    const std::string& diffuseMap,
    const std::string& normalMap,
    const float3& baseColor,
    float shininess) {
    
    // 生成GUID
    std::string guid = "material_" + name;
    
    // 检查是否已经存在
    if (resourceManager.hasResource(guid)) {
        return guid;
    }

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
        std::string diffuseGUID = loadTexture(diffuseMap, TextureType::DIFFUSE);
        if (!diffuseGUID.empty()) {
            material->setDiffuseMapGUID(diffuseGUID);
        }
    }

    if (!normalMap.empty()) {
        std::string normalGUID = loadTexture(normalMap, TextureType::NORMAL);
        if (!normalGUID.empty()) {
            material->setNormalMapGUID(normalGUID);
        }
    }

    //TODO 暂时移除对贴图的支持，后续需要增加 uniforms 贴图引用表 结构
    std::string shaderGUID;
    // if (material->hasTexture(Material::NORMAL_MAP)) {
    //     // 创建并使用支持法线贴图的着色器
    //     auto shader = createTexturedPhongShader(
    //         getTexture(material->getDiffuseMapGUID()),
    //         getTexture(material->getNormalMapGUID())
    //     );
    //     shaderGUID = createShader(name + "_shader", shader);
    // }
    // else if (material->hasTexture(Material::DIFFUSE_MAP)) {
    //     // 创建并使用支持纹理的着色器
    //     auto shader = createTexturedPhongShader(
    //         getTexture(material->getDiffuseMapGUID())
    //     );
    //     shaderGUID = createShader(name + "_shader", shader);
    // }
    // else {
        // 创建并使用基本的Phong着色器
        auto shader = createPhongShader();
        shaderGUID = createShader(name + "_shader", shader);
    // }

    material->setShaderGUID(shaderGUID);
    
    // 将材质添加到资源管理器
    resourceManager.addResource(material, guid);
    
    return guid;
}

// 创建着色器的实现
std::string Scene::createShader(const std::string& name, std::shared_ptr<IShader> shader) {
    // 生成GUID
    std::string guid = "shader_" + name;
    
    // 检查是否已经存在
    if (resourceManager.hasResource(guid)) {
        return guid;
    }
    
    // 将着色器添加到资源管理器
    resourceManager.addResource(shader, guid);
    
    return guid;
}

// 配置阴影映射的实现
void Scene::setupShadowMapping(bool enabled, int shadowMapSize) {
    shadowMappingEnabled = enabled;
    this->shadowMapSize = shadowMapSize;
    
    if (enabled) {
        // 创建阴影贴图着色器
        auto shadowShader = createShadowMapShader();
        shadowShaderGUID = createShader("shadow_map_shader", shadowShader);
    }
}

// 更新阴影贴图的实现
void Scene::updateShadowMap(Renderer& renderer) {
    if (!shadowMappingEnabled || objects.empty()) {
        return;
    }
    
    // 创建或获取阴影贴图
    std::shared_ptr<Texture> shadowMap;
    if (shadowMapGUID.empty()) {
        // 创建新的阴影贴图
        shadowMap = renderer.createShadowMap(shadowMapSize, shadowMapSize);
        shadowMapGUID = "texture_shadowmap";
        resourceManager.addResource(shadowMap, shadowMapGUID);
    } else {
        // 获取已有的阴影贴图
        shadowMap = getTexture(shadowMapGUID);
    }
    
    // 计算光源的视图矩阵
    Matrix4x4f lightViewMatrix = Matrix4x4f::lookAt(
        light.position,              // 光源位置
        float3(0.0f, 0.0f, 0.0f),    // 观察原点
        float3(0.0f, 1.0f, 0.0f)     // 上方向
    );
    
    // 计算光源的投影矩阵
    // 注：对于方向光，可以使用正交投影；对于点光源，可以使用透视投影
    Matrix4x4f lightProjMatrix = Matrix4x4f::perspective(
        3.14159f / 4.0f,  // FOV
        1.0f,             // 宽高比
        0.1f,             // 近平面
        100.0f            // 远平面
    );
    // renderer.getProjMatrix();
    // 更新光源的变换矩阵
    light.lightViewMatrix = lightViewMatrix;
    light.lightProjMatrix = lightProjMatrix;
    light.castShadow = true;
    renderer.setViewMatrix(lightViewMatrix);
    renderer.setProjMatrix(lightProjMatrix);
    // 收集要投射阴影的网格
    std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> shadowCasters;
    for (const auto& obj : objects) {
        if (obj.castShadow) {
            auto mesh = getMesh(obj.meshGUID);
            if (mesh) {
                // 存储网格和它的模型变换矩阵
                // 这样在阴影计算时可以应用正确的变换
                shadowCasters.push_back({mesh, obj.modelMatrix});
            }
        }
    }
    
    // 渲染阴影贴图
    renderer.shadowPass(shadowCasters);
}

// 渲染场景的实现
void Scene::render(Renderer& renderer) {
    // 如果启用了阴影映射，先更新阴影贴图
    if (shadowMappingEnabled) {
        updateShadowMap(renderer);
    }
    
    // 应用相机设置
    renderer.setViewMatrix(camera.getViewMatrix());
    renderer.setProjMatrix(camera.getProjectionMatrix());
    renderer.setEye(camera.getPosition());

    // 设置光源
    renderer.setLight(light);

    // 清屏
    renderer.clear(float4(0.16f, 0.16f, 0.24f, 1.0f)); // 深蓝灰色背景

    // 准备光源空间变换矩阵，用于阴影映射
    Matrix4x4f lightSpaceMatrix = light.lightProjMatrix * light.lightViewMatrix;
    
    // 准备阴影贴图
    std::shared_ptr<Texture> shadowMap = nullptr;
    if (shadowMappingEnabled) {
        shadowMap = getTexture(shadowMapGUID);
    }

    // 渲染所有对象
    for (const auto& obj : objects) {
        // 设置模型变换
        renderer.setModelMatrix(obj.modelMatrix);
        
        // 获取网格和材质
        auto mesh = getMesh(obj.meshGUID);
        auto material = getMaterial(obj.materialGUID);
        
        if (!mesh || !material) {
            continue;
        }
        
        // 获取着色器
        auto shader = getShader(material->getShaderGUID());
        if (!shader) {
            continue;
        }
        
        // 设置着色器的统一变量
        ShaderUniforms uniforms;
        uniforms.modelMatrix = obj.modelMatrix;
        uniforms.viewMatrix = renderer.getViewMatrix();
        uniforms.projMatrix = renderer.getProjMatrix();
        uniforms.mvpMatrix = renderer.getMVPMatrix();
        uniforms.eyePosition = renderer.getEye();
        uniforms.light = renderer.getLight();
        uniforms.surface = material->getSurface();
        
        // 如果启用了阴影映射，设置阴影相关参数
        if (shadowMappingEnabled && obj.receiveShadow && shadowMap) {
            uniforms.useShadowMap = true;
            uniforms.shadowMap = shadowMap;
            uniforms.lightSpaceMatrix = lightSpaceMatrix;
        } else {
            uniforms.useShadowMap = false;
        }
        
        shader->setUniforms(uniforms);
        
        // 使用对象的材质和着色器渲染网格
        renderer.drawMesh(mesh, shader);
    }
}