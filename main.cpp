// main.cpp
// 测试光栅化渲染器的主程序 - 更新版本
#include "renderer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "mesh.h"
#include "test.h"
#include "shader.h"

int main()
{
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);

    // 创建不同的场景对象，使用不同的着色器
    std::vector<SceneObject> scene;

    // 设置相机与投影
    Vec3f eye(0.0f, 0.0f, 5.0f);
    Vec3f target(0.0f, 0.0f, 0.0f);
    Vec3f up(0.0f, 1.0f, 0.0f);
    renderer.setEye(eye);
    renderer.setViewMatrix(Matrix4x4f::lookAt(eye, target, up));

    float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    float fov = 3.14159f / 4.0f; // 45度视角
    renderer.setProjMatrix(Matrix4x4f::perspective(fov, aspect, 0.1f, 100.0f));

    // 设置光源
    Light light(Vec3f(2.0f, 2.0f, 5.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    renderer.setLight(light);

    // 渲染场景
    // 创建一个简单的OBJ文件用于测试
    std::string testObjFile1 = "sphere.obj";
    std::string testObjFile2 = "box_sphere.obj";

    // 加载OBJ模型并创建两个副本
    std::shared_ptr<Mesh> mesh1 = loadOBJ(testObjFile1);
    std::shared_ptr<Mesh> mesh2 = loadOBJ(testObjFile2);

    if (!mesh1 || !mesh2)
    {
        std::cerr << "加载OBJ模型失败: " << std::endl;
        return 0;
    }

    // 设置第一个模型 - 使用Phong着色器和红色材质
    Material material1(
        Vec3(0.2f, 0.05f, 0.05f), // 环境光反射系数（偏红）
        Vec3(0.8f, 0.2f, 0.2f),   // 漫反射系数（红色）
        Vec3(0.5f, 0.5f, 0.5f),   // 镜面反射系数
        32.0f                     // 光泽度
    );
    mesh1->setMaterial(material1);
    mesh1->setColor(Color(220, 50, 50)); // 红色
    mesh1->setShader(createToonShader());

    // 设置第二个模型 - 使用卡通着色器和蓝色材质
    Material material2(
        Vec3(0.05f, 0.05f, 0.2f), // 环境光反射系数（偏蓝）
        Vec3(0.2f, 0.2f, 0.8f),   // 漫反射系数（蓝色）
        Vec3(0.5f, 0.5f, 0.5f),   // 镜面反射系数
        32.0f                     // 光泽度
    );
    mesh2->setMaterial(material2);
    mesh2->setColor(Color(50, 50, 220)); // 蓝色
    mesh2->setShader(createPhongShader());

    Matrix4x4f modelMatrix1 = Matrix4x4f::translation(-1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(30.0f * 3.14159f / 180.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);

    Matrix4x4f modelMatrix2 = Matrix4x4f::translation(1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(-30.0f * 3.14159f / 180.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);

    scene.push_back(SceneObject(mesh1, modelMatrix1));
    scene.push_back(SceneObject(mesh2, modelMatrix2));

    renderScene(renderer, scene);

    // 保存渲染结果
    saveToPPM("scene_with_multiple_shaders.ppm", renderer.getFrameBuffer());

    std::cout << "渲染完成!" << std::endl;

    return 0;
}