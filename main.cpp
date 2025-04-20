// main.cpp
// 使用 Scene 类的测试程序
#include "scene.h"
#include "test.h"
// #include <iostream>

int main()
{
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);

    // 创建场景
    Scene scene;

    // 设置相机
    Camera &camera = scene.getCamera();
    camera.setPosition(Vec3f(0.0f, 0.0f, 5.0f));
    camera.setTarget(Vec3f(0.0f, 0.0f, 0.0f));
    camera.setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera.setAspect(static_cast<float>(WIDTH) / HEIGHT);
    camera.setFOV(3.14159f / 4.0f); // 45度视角

    // 设置光源
    Light light(Vec3f(2.0f, 2.0f, 5.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    scene.setLight(light);

    // 创建材质 - 红色材质带漫反射贴图
    auto material1 = scene.createMaterialWithTextures(
        "RedMaterial",
        "../assets/red_diffuse.tga", // 漫反射贴图路径
        "../assets/normal_map.tga",  // 法线贴图路径
        float3(0.8f, 0.2f, 0.2f),    // 基础颜色
        32.0f                        // 光泽度
    );

    // 创建材质 - 蓝色材质带漫反射贴图
    auto material2 = scene.createMaterialWithTextures(
        "BlueMaterial",
        "../assets/blue_diffuse.tga", // 漫反射贴图路径
        "",                           // 无法线贴图
        float3(0.2f, 0.2f, 0.8f),     // 基础颜色
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
                              Matrix4x4f::rotationY(30.0f * 3.14159f / 180.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);

    Matrix4x4f modelMatrix2 = Matrix4x4f::translation(1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(-30.0f * 3.14159f / 180.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);

    Matrix4x4f modelMatrix3 = Matrix4x4f::translation(0.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(0) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
    // 加载模型
    SceneObject obj1 = scene.loadModel("../assets/sphere.obj", "RedSphere", material1, modelMatrix1);
    scene.addObject(obj1);
    SceneObject obj2 = scene.loadModel("../assets/box_sphere.obj", "BlueBox", material2, modelMatrix2);
    scene.addObject(obj2);
    SceneObject obj3 = scene.loadModel("../assets/plane.obj", "plane", material3, modelMatrix3);
    scene.addObject(obj3);
    // 渲染场景
    scene.render(renderer);

    // 保存结果
    saveToPPM("../output/textured_scene.ppm", renderer.getFrameBuffer());

    std::cout << "渲染完成!" << std::endl;

    return 0;
}