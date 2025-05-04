#include "scene_manager.h"

SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

void SceneManager::initializeScene(SceneType type, Scene& scene, int width, int height) {
    switch(type) {
        case SceneType::SPHERES:
            initSpheresScene(scene, width, height);
            break;
        case SceneType::CUBES:
            initCubesScene(scene, width, height);
            break;
        default:
            initDefaultScene(scene, width, height);
            break;
    }
}

void SceneManager::initDefaultScene(Scene& scene, int width, int height) {
    // 设置相机
    Camera &camera = scene.getCamera();
    camera.setPosition(Vec3f(0.0f, 3.0f, 4.0f));
    camera.setTarget(Vec3f(0.0f, 0.0f, 0.0f));
    camera.setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera.setAspect(static_cast<float>(width) / height);
    camera.setFOV(3.14159f / 4.0f); // 45度视角
    
    // 设置光源
    Light light(Vec3f(1.0f, 2.0f, 4.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    scene.setLight(light);
    
    // 创建材质 - 红色材质带漫反射贴图
    auto material1 = scene.createMaterialWithTextures(
        "RedMaterial",
        "../assets/test.tga", // 漫反射贴图路径
        "../assets/normal_map.tga",  // 法线贴图路径
        float3(0.8f, 0.2f, 0.2f),    // 基础颜色
        32.0f                        // 光泽度
    );
    
    // 创建材质 - 蓝色材质带漫反射贴图
    auto material2 = scene.createMaterialWithTextures(
        "BlueMaterial",
        "", // 漫反射贴图路径
        "../assets/normal_map.tga",                           // 无法线贴图
        float3(0.3f, 0.2f, 0.8f),     // 基础颜色
        10.0f                         // 光泽度
    );
    
    auto material3 = scene.createMaterialWithTextures(
        "Material3",
        "../assets/blue_diffuse.tga", // 漫反射贴图路径
        "",                           // 无法线贴图
        float3(0.2f, 0.2f, 0.8f),     // 基础颜色
        4.0f                          // 光泽度
    );
    
    // 加载模型并添加到场景
    Matrix4x4f modelMatrix1 = Matrix4x4f::translation(-1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(0) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
    
    Matrix4x4f modelMatrix2 = Matrix4x4f::translation(1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(-30.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
    
    Matrix4x4f modelMatrix3 = Matrix4x4f::translation(0.0f, -1.0f, -1.0f) *
                              Matrix4x4f::rotationX(0) *
                              Matrix4x4f::scaling(2.0f, 2.0f, 2.0f);
    // 加载模型
    auto mesh = scene.loadMesh("../assets/sphere.obj", "RedSphere");
    auto mesh2 = scene.loadMesh("../assets/box_sphere.obj", "BlueBox");
    auto mesh3 = scene.loadMesh("../assets/plane.obj", "plane");
    
    auto obj1 = SceneObject("RedSphere", mesh, material2, modelMatrix1);
    auto obj2 = SceneObject("BlueBox", mesh2, material3, modelMatrix2);
    auto obj3 = SceneObject("plane", mesh3, material1, modelMatrix3);
    
    scene.addObject(obj2);
    scene.addObject(obj1);
    scene.addObject(obj3);
}

void SceneManager::initSpheresScene(Scene& scene, int width, int height) {
    // 设置相机
    Camera &camera = scene.getCamera();
    camera.setPosition(Vec3f(0.0f, 5.0f, 10.0f));
    camera.setTarget(Vec3f(0.0f, 0.0f, 0.0f));
    camera.setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera.setAspect(static_cast<float>(width) / height);
    camera.setFOV(3.14159f / 4.0f);
    
    // 设置光源
    Light light(Vec3f(5.0f, 10.0f, 5.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    scene.setLight(light);
    
    // 创建地面材质
    auto floorMaterial = scene.createMaterialWithTextures(
        "FloorMaterial",
        "../assets/test.tga",
        "../assets/normal_map.tga",
        float3(0.5f, 0.5f, 0.5f),
        16.0f
    );
    
    // 创建球体材质
    auto redMaterial = scene.createMaterialWithTextures(
        "RedMaterial",
        "",
        "../assets/normal_map.tga",
        float3(0.8f, 0.2f, 0.2f),
        32.0f
    );
    
    auto greenMaterial = scene.createMaterialWithTextures(
        "GreenMaterial",
        "",
        "../assets/normal_map.tga",
        float3(0.2f, 0.8f, 0.2f),
        32.0f
    );
    
    auto blueMaterial = scene.createMaterialWithTextures(
        "BlueMaterial",
        "",
        "../assets/normal_map.tga",
        float3(0.2f, 0.2f, 0.8f),
        32.0f
    );
    
    // 加载模型
    auto sphereMesh = scene.loadMesh("../assets/sphere.obj", "Sphere");
    auto planeMesh = scene.loadMesh("../assets/plane.obj", "Plane");
    
    // 创建地面
    Matrix4x4f floorMatrix = Matrix4x4f::translation(0.0f, -4.0f, 0.0f) *
                             Matrix4x4f::scaling(5.0f, 1.0f,5.0f);
    auto floorObj = SceneObject("Floor", planeMesh, floorMaterial, floorMatrix);

    
    // 创建多个球体
    const int sphereCount = 7;
    const float radius = 3.0f;
    
    for (int i = 0; i < sphereCount; i++) {
        float angle = 2.0f * 3.14159f * i / sphereCount;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        Matrix4x4f sphereMatrix = Matrix4x4f::translation(x, 0.0f, z) *
                                  Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
        
        // 选择材质
        std::string materialGUID;
        if (i % 3 == 0) materialGUID = redMaterial;
        else if (i % 3 == 1) materialGUID = greenMaterial;
        else materialGUID = blueMaterial;
        
        auto sphereObj = SceneObject("Sphere_" + std::to_string(i), 
                                    sphereMesh, 
                                    materialGUID, 
                                    sphereMatrix);
        scene.addObject(sphereObj);
    }
    scene.addObject(floorObj);
}

void SceneManager::initCubesScene(Scene& scene, int width, int height) {
    // 设置相机
    Camera &camera = scene.getCamera();
    camera.setPosition(Vec3f(0.0f, 4.0f, 8.0f));
    camera.setTarget(Vec3f(0.0f, 0.0f, 0.0f));
    camera.setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera.setAspect(static_cast<float>(width) / height);
    camera.setFOV(3.14159f / 4.0f);
    
    // 设置光源
    Light light(Vec3f(3.0f, 5.0f, 2.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    scene.setLight(light);
    
    // 创建材质
    auto floorMaterial = scene.createMaterialWithTextures(
        "FloorMaterial",
        "../assets/test.tga",
        "../assets/normal_map.tga",
        float3(0.5f, 0.5f, 0.5f),
        16.0f
    );
    
    auto cubeMaterial = scene.createMaterialWithTextures(
        "CubeMaterial",
        "../assets/blue_diffuse.tga",
        "",
        float3(0.2f, 0.2f, 0.8f),
        4.0f
    );
    
    // 加载模型
    auto boxMesh = scene.loadMesh("../assets/box_sphere.obj", "Box");
    auto planeMesh = scene.loadMesh("../assets/plane.obj", "Plane");
    
    // 创建地面
    Matrix4x4f floorMatrix = Matrix4x4f::translation(0.0f, -2.0f, 0.0f) *
                             Matrix4x4f::scaling(10.0f, 1.0f, 10.0f);
    auto floorObj = SceneObject("Floor", planeMesh, floorMaterial, floorMatrix);
    
    
    // 创建立方体塔
    const int levels = 4;
    float y = -1.0f;
    float scale = 1.0f;
    
    for (int level = 0; level < levels; level++) {
        int cubesPerSide = levels - level;
        float offset = (cubesPerSide - 1) * 0.5f;
        
        for (int i = 0; i < cubesPerSide; i++) {
            for (int j = 0; j < cubesPerSide; j++) {
                float x = (i - offset) * scale;
                float z = (j - offset) * scale;
                
                Matrix4x4f cubeMatrix = Matrix4x4f::translation(x, y, z) *
                                       Matrix4x4f::scaling(0.45f, 0.45f, 0.45f);
                
                auto cubeObj = SceneObject("Cube_" + std::to_string(level) + "_" + 
                                          std::to_string(i) + "_" + 
                                          std::to_string(j), 
                                          boxMesh, 
                                          cubeMaterial, 
                                          cubeMatrix);
                scene.addObject(cubeObj);
            }
        }
        
        y += scale;
    }
    scene.addObject(floorObj);
}