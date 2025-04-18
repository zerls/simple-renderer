// main.cpp
// 测试光栅化渲染器的主程序

#include "Renderer.h"
#include <iostream>
#include <fstream>
#include <string>

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

int main() {
    // 创建 800x600 的渲染器
    // Renderer renderer(800, 600);
    
    // // 清除屏幕为黑色
    // renderer.clear(Color(0, 0, 0));
    
    // // 创建一个彩色三角形
    // Triangle triangle(
    //     Vertex(Vec3(400.0f, 100.0f, 0.0f), Color(255, 0, 0)),    // 红色顶点
    //     Vertex(Vec3(100.0f, 500.0f, 0.0f), Color(0, 255, 0)),    // 绿色顶点
    //     Vertex(Vec3(700.0f, 500.0f, 0.0f), Color(0, 0, 255))     // 蓝色顶点
    // );
    
    // // 绘制三角形
    // renderer.drawTriangle(triangle);
    
    // // 保存图像
    // saveToPPM("triangle.ppm", renderer.getFrameBuffer());

    const int WIDTH = 800;
    const int HEIGHT = 600;
    
    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);
    
    // 设置透视投影
    float aspect = static_cast<float>(WIDTH) / HEIGHT;
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
    float angle = 3.14159f / 6.0f; // 30度
    
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
    
    std::cout << "渲染完成!" << std::endl;

    
    return 0;
}