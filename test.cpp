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


// 新增：加载并绘制OBJ模型
// 修复后的 DrawOBJModel 函数
// 修改后的 DrawOBJModel 函数
void DrawOBJModel(Renderer& renderer, const std::string& filename)
{
    // 设置相机与投影
    Vec3 eye(0.0f, 0.0f, 3.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);
    renderer.setViewMatrix(Matrix4x4f::lookAt(eye, target, up));
    
    int Width = renderer.getFrameBuffer().getWidth();
    int Height = renderer.getFrameBuffer().getHeight();
    float aspect = static_cast<float>(Width) / static_cast<float>(Height);
    float fov = 3.14159f / 4.0f; // 45度视角
    renderer.setProjMatrix(Matrix4x4f::perspective(fov, aspect, 0.1f, 100.0f));
    
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
    Matrix4x4f rotX = Matrix4x4f::rotationX(angle);
    Matrix4x4f rotY = Matrix4x4f::rotationY(angle * 2);
    renderer.setModelMatrix(rotX * rotY);
    
    // 清除屏幕为黑色
    renderer.clear(Color(0, 0, 0));
    
    // 设置光源 - 调整光源位置和强度
    Light light(Vec3(2.0f, 2.0f, 2.0f), Vec3(1.0f, 1.0f, 1.0f), 1.2f, 0.3f);
    renderer.setLight(light);
    
    // 使用Phong着色器代替旧的光照系统
    std::shared_ptr<IShader> phongShader = createPhongShader();
    std::shared_ptr<IShader> basicShader = createBasicShader();
    renderer.setShader(phongShader);
    // renderer.setShader(basicShader);
    
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
// void CreateSimpleOBJ(const std::string& filename)
// {
//     std::ofstream file(filename);
//     if (!file) {
//         std::cerr << "无法创建文件：" << filename << std::endl;
//         return;
//     }
    
//     // 写入OBJ文件头
//     file << "# 简单的四面体OBJ文件\n";
//     file << "# 由SimpleRenderer生成\n\n";
    
//     // 写入4个顶点
//     file << "v 0.0 1.0 0.0\n";  // 顶点1 (顶部)
//     file << "v -1.0 -1.0 -1.0\n";  // 顶点2 (左下前)
//     file << "v 1.0 -1.0 -1.0\n";  // 顶点3 (右下前)
//     file << "v 0.0 -1.0 1.0\n";  // 顶点4 (底部后)
    
//     // 写入顶点法线
//     file << "\n# 顶点法线\n";
//     file << "vn 0.0 1.0 0.0\n";  // 上方向
//     file << "vn -0.577 -0.577 -0.577\n";  // 左下前方向
//     file << "vn 0.577 -0.577 -0.577\n";  // 右下前方向
//     file << "vn 0.0 -0.577 0.577\n";  // 底部后方向
    
//     // 写入纹理坐标（简单的）
//     file << "\n# 纹理坐标\n";
//     file << "vt 0.5 1.0\n";
//     file << "vt 0.0 0.0\n";
//     file << "vt 1.0 0.0\n";
//     file << "vt 0.5 0.5\n";
    
//     // 写入面（四个三角形面）
//     file << "\n# 面\n";
//     file << "f 1/1/1 2/2/2 3/3/3\n";  // 前面
//     file << "f 1/1/1 3/3/3 4/4/4\n";  // 右面
//     file << "f 1/1/1 4/4/4 2/2/2\n";  // 左面
//     file << "f 2/2/2 4/4/4 3/3/3\n";  // 底面
    
//     file.close();
//     std::cout << "已创建简单OBJ文件：" << filename << std::endl;
// }