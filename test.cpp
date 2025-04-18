#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mesh.h"
#include "renderer.h"

// 将帧缓冲区保存为 PPM 图像文件
void saveToPPM(const std::string& filename, const FrameBuffer& frameBuffer) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法创建文件：" << filename << std::endl;
        return;
    }

    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();
    
    // 写入 PPM 文件头
    file << "P6\n" << width << " " << height << "\n255\n";
    
    // 写入像素数据（注意：PPM 只支持 RGB，不支持 alpha 通道）
    const uint8_t* data = frameBuffer.getData();
    for (int i = 0; i < width * height; ++i) {
        int index = i * 4; // RGBA 格式
        file.write(reinterpret_cast<const char*>(&data[index]), 3); // 只写入 RGB
    }
    
    std::cout << "图像已保存到 " << filename << std::endl;
}

void DrawTriangle(Renderer& renderer){
    // 创建 800x600 的渲染器
    // Renderer renderer(800, 600);
    
    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));
    
    // 创建一个彩色三角形
    Triangle triangle(
        Vertex(Vec3(400.0f, 100.0f, 0.0f), Color(255, 0, 0)),    // 红色顶点
        Vertex(Vec3(100.0f, 500.0f, 0.0f), Color(0, 255, 0)),    // 绿色顶点
        Vertex(Vec3(700.0f, 500.0f, 0.0f), Color(0, 0, 255))     // 蓝色顶点
    );
    
    // 绘制三角形
    renderer.drawTriangle(triangle);
    
    // 保存图像
    saveToPPM("triangle.ppm", renderer.getFrameBuffer());
}

void DrawTriangleWithLighting(Renderer &renderer)
{
    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));

    // 创建一个带法线的三角形
    Triangle triangle(
        Vertex(Vec3(400.0f, 100.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec2(0.5f, 0.0f), Color(255, 0, 0)), // 红色顶点
        Vertex(Vec3(100.0f, 500.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec2(0.0f, 1.0f), Color(0, 255, 0)), // 绿色顶点
        Vertex(Vec3(700.0f, 500.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec2(1.0f, 1.0f), Color(0, 0, 255))  // 蓝色顶点
    );

    // 设置光源
    Light light(Vec3(400.0f, 300.0f, -200.0f), Vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    renderer.setLight(light);
    
    // 启用光照
    renderer.enableLighting(true);

    // 绘制三角形
    renderer.drawTriangle(triangle);

    // 保存图像
    saveToPPM("triangle_lit.ppm", renderer.getFrameBuffer());
    
    // 保存深度图
    // renderer.saveDepthMap("triangle_depth.ppm", 0.0f, 1.0f);
}

void DrawCube(Renderer& renderer){
    // 设置透视投影
    float aspect = static_cast<float>(renderer.getFrameBuffer().getWidth()) / renderer.getFrameBuffer().getHeight();
    float fov = 3.14159f / 4.0f; // 45度视角
    renderer.setProjMatrix(Matrix4x4::perspective(fov, aspect, 0.1f, 100.0f));
    
    // 设置相机位置
    Vec3 eye(0.0f, 0.0f, 3.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);
    renderer.setViewMatrix(Matrix4x4::lookAt(eye, target, up));
    
    // 创建立方体
    Cube cube = Cube::createUnitCube();
    
    // 设置立方体的旋转角度
    float angle = 30.0f*3.14159f / 180.0f; // 
    
    // 设置模型变换（旋转）
    Matrix4x4 rotX = Matrix4x4::rotationX(angle);
    Matrix4x4 rotY = Matrix4x4::rotationY(angle * 2);
    renderer.setModelMatrix(rotX * rotY);
    
    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));
    
    // 绘制立方体
    renderer.drawCube(cube);
    
    // 保存图像
    saveToPPM("cube.ppm", renderer.getFrameBuffer());
}

void DrawCubeWithLighting(Renderer &renderer)
{
    // 创建立方体
    Cube cube = Cube::createUnitCube();

    // 设置立方体的旋转角度
    float angle = 30.0f * 3.14159f / 180.0f; // 30度

    // 设置模型变换（旋转）
    Matrix4x4 rotX = Matrix4x4::rotationX(angle);
    Matrix4x4 rotY = Matrix4x4::rotationY(angle * 2);
    renderer.setModelMatrix(rotX * rotY);

    // 设置相机位置
    Vec3 eye(0.0f, 0.0f, 3.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);
    renderer.setViewMatrix(Matrix4x4::lookAt(eye, target, up));

    int Width = renderer.getFrameBuffer().getWidth();
    int Height = renderer.getFrameBuffer().getHeight();
    // 设置透视投影
    float aspect = static_cast<float>(Width) / static_cast<float>(Height);
    float fov = 3.14159f / 4.0f; // 45度视角
    renderer.setProjMatrix(Matrix4x4::perspective(fov, aspect, 0.1f, 100.0f));

    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));

    // 设置光源
    Light light(Vec3(-1.0f, 1.0f, 2.0f), Vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    renderer.setLight(light);
    
    // 启用光照
    renderer.enableLighting(true);

    // 绘制立方体
    renderer.drawCube(cube);

    // 保存图像
    saveToPPM("cube_lit.ppm", renderer.getFrameBuffer());
    
    // 保存深度图
    // renderer.saveDepthMap("cube_depth.ppm", 0.1f, 3.0f);
}

// 新增：加载并绘制OBJ模型
// 修复后的 DrawOBJModel 函数
void DrawOBJModel(Renderer& renderer, const std::string& filename)
{
    // 设置相机与投影
    Vec3 eye(0.0f, 0.0f, 3.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);
    renderer.setViewMatrix(Matrix4x4::lookAt(eye, target, up));
    
    int Width = renderer.getFrameBuffer().getWidth();
    int Height = renderer.getFrameBuffer().getHeight();
    float aspect = static_cast<float>(Width) / static_cast<float>(Height);
    float fov = 3.14159f / 4.0f; // 45度视角
    renderer.setProjMatrix(Matrix4x4::perspective(fov, aspect, 0.1f, 100.0f));
    
    // 加载OBJ模型
    std::shared_ptr<Mesh> mesh = loadOBJ(filename);
    if (!mesh) {
        std::cerr << "加载OBJ模型失败: " << filename << std::endl;
        return;
    }
    
    // 设置模型的材质和颜色
    Material material(
        Vec3(0.2f, 0.2f, 0.2f),   // 环境光反射系数
        Vec3(0.8f, 0.8f, 0.8f),   // 漫反射系数
        Vec3(0.5f, 0.5f, 0.5f),   // 镜面反射系数
        32.0f                      // 光泽度
    );
    mesh->setMaterial(material);
    
    // 设置模型颜色 - 使用更明亮的颜色
    mesh->setColor(Color(220, 220, 220)); 
    
    // 设置模型变换（旋转）
    float angle = 30.0f * 3.14159f / 180.0f;
    Matrix4x4 rotX = Matrix4x4::rotationX(angle);
    Matrix4x4 rotY = Matrix4x4::rotationY(angle * 2);
    renderer.setModelMatrix(rotX * rotY);
    
    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));
    
    // 设置光源 - 调整光源位置和强度
    Light light(Vec3(2.0f, 2.0f, 2.0f), Vec3(1.0f, 1.0f, 1.0f), 1.2f, 0.3f);
    renderer.setLight(light);
    
    // 启用光照
    renderer.enableLighting(true);
    
    // 绘制模型
    mesh->draw(renderer);
    
    // 保存渲染结果
    std::string outputFile = filename.substr(0, filename.find_last_of('.')) + ".ppm";
    saveToPPM(outputFile, renderer.getFrameBuffer());
    
    // 保存深度图
    std::string depthFile = filename.substr(0, filename.find_last_of('.')) + "_depth.ppm";
    renderer.saveDepthMap(depthFile, 0.1f, 3.0f);
}

// 新增：创建并保存一个简单的OBJ文件（用于测试）
void CreateSimpleOBJ(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "无法创建文件：" << filename << std::endl;
        return;
    }
    
    // 写入OBJ文件头
    file << "# 简单的四面体OBJ文件\n";
    file << "# 由SimpleRenderer生成\n\n";
    
    // 写入4个顶点
    file << "v 0.0 1.0 0.0\n";  // 顶点1 (顶部)
    file << "v -1.0 -1.0 -1.0\n";  // 顶点2 (左下前)
    file << "v 1.0 -1.0 -1.0\n";  // 顶点3 (右下前)
    file << "v 0.0 -1.0 1.0\n";  // 顶点4 (底部后)
    
    // 写入顶点法线
    file << "\n# 顶点法线\n";
    file << "vn 0.0 1.0 0.0\n";  // 上方向
    file << "vn -0.577 -0.577 -0.577\n";  // 左下前方向
    file << "vn 0.577 -0.577 -0.577\n";  // 右下前方向
    file << "vn 0.0 -0.577 0.577\n";  // 底部后方向
    
    // 写入纹理坐标（简单的）
    file << "\n# 纹理坐标\n";
    file << "vt 0.5 1.0\n";
    file << "vt 0.0 0.0\n";
    file << "vt 1.0 0.0\n";
    file << "vt 0.5 0.5\n";
    
    // 写入面（四个三角形面）
    file << "\n# 面\n";
    file << "f 1/1/1 2/2/2 3/3/3\n";  // 前面
    file << "f 1/1/1 3/3/3 4/4/4\n";  // 右面
    file << "f 1/1/1 4/4/4 2/2/2\n";  // 左面
    file << "f 2/2/2 4/4/4 3/3/3\n";  // 底面
    
    file.close();
    std::cout << "已创建简单OBJ文件：" << filename << std::endl;
}