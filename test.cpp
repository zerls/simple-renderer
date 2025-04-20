#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mesh.h"
#include "renderer.h"
#include "shader.h"

// 创建一个简单的三角形网格
std::shared_ptr<Mesh> createTriangleMesh(const Color &color)
{
    auto mesh = std::make_shared<Mesh>();

    // 添加顶点
    mesh->addVertex(Vec3f(-0.5f, -0.5f, 0.0f));
    mesh->addVertex(Vec3f(0.5f, -0.5f, 0.0f));
    mesh->addVertex(Vec3f(0.0f, 0.5f, 0.0f));

    // 添加法线
    mesh->addNormal(Vec3f(0.0f, 0.0f, 1.0f));
    mesh->addNormal(Vec3f(0.0f, 0.0f, 1.0f));
    mesh->addNormal(Vec3f(0.0f, 0.0f, 1.0f));

    // 添加纹理坐标
    mesh->addTexCoord(Vec2f(0.0f, 0.0f));
    mesh->addTexCoord(Vec2f(1.0f, 0.0f));
    mesh->addTexCoord(Vec2f(0.5f, 1.0f));

    // 添加面
    Face face;
    face.vertexIndices = {0, 1, 2};
    face.normalIndices = {0, 1, 2};
    face.texCoordIndices = {0, 1, 2};
    mesh->addFace(face);

    // 设置颜色
    mesh->setColor(color);

    return mesh;
}


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
