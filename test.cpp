#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "mesh.h"
#include "renderer.h"
#include "shader.h"

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

// 保存深度图到PPM文件
void saveDepthMap(const std::string &filename, const FrameBuffer& frameBuffer,float nearPlane, float farPlane)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "无法创建文件：" << filename << std::endl;
        return;
    }

    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();

    // 写入PPM文件头
    file << "P6\n"
         << width << " " << height << "\n255\n";

    // 准备临时缓冲区
    std::vector<uint8_t> depthPixels(width * height * 3);

    // 为了可视化，我们将深度值映射到灰度值
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = frameBuffer.getDepth(x, y);

            // 将深度值从 [nearPlane, farPlane] 映射到 [0, 1]
            float normalizedDepth = (depth - nearPlane) / (farPlane - nearPlane);
            normalizedDepth = std::max(0.0f, std::min(1.0f, normalizedDepth));

            // 将归一化的深度值转换为灰度值
            uint8_t grayValue = static_cast<uint8_t>((1.0f - normalizedDepth) * 255.0f);

            // 设置RGB值（相同的灰度值）
            int index = (y * width + x) * 3;
            depthPixels[index] = grayValue;     // R
            depthPixels[index + 1] = grayValue; // G
            depthPixels[index + 2] = grayValue; // B
        }
    }

    // 写入像素数据
    file.write(reinterpret_cast<const char *>(depthPixels.data()), depthPixels.size());

    std::cout << "深度图已保存到 " << filename << std::endl;
}