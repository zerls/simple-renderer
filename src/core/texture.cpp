// texture.cpp
// 纹理类和TGA文件加载的实现

#include "texture.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

// Texture构造函数
Texture::Texture() {
    // 默认构造函数不执行特殊操作
}

// 从TGA文件加载纹理
bool Texture::loadFromTGA(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开纹理文件：" << filename << std::endl;
        return false;
    }
    
    // TGA文件头信息
    uint8_t idLength;         // ID信息长度
    uint8_t colorMapType;     // 色彩映射表类型
    uint8_t imageType;        // 图像类型
    uint16_t colorMapStart;   // 色彩映射起始索引
    uint16_t colorMapLength;  // 色彩映射长度
    uint8_t colorMapDepth;    // 色彩映射深度
    uint16_t xOrigin;         // X起点
    uint16_t yOrigin;         // Y起点
    uint16_t width;           // 宽度
    uint16_t height;          // 高度
    uint8_t bitsPerPixel;     // 像素位数
    uint8_t imageDescriptor;  // 图像描述符
    
    // 读取TGA文件头
    file.read(reinterpret_cast<char*>(&idLength), 1);
    file.read(reinterpret_cast<char*>(&colorMapType), 1);
    file.read(reinterpret_cast<char*>(&imageType), 1);
    file.read(reinterpret_cast<char*>(&colorMapStart), 2);
    file.read(reinterpret_cast<char*>(&colorMapLength), 2);
    file.read(reinterpret_cast<char*>(&colorMapDepth), 1);
    file.read(reinterpret_cast<char*>(&xOrigin), 2);
    file.read(reinterpret_cast<char*>(&yOrigin), 2);
    file.read(reinterpret_cast<char*>(&width), 2);
    file.read(reinterpret_cast<char*>(&height), 2);
    file.read(reinterpret_cast<char*>(&bitsPerPixel), 1);
    file.read(reinterpret_cast<char*>(&imageDescriptor), 1);
    
    // 跳过ID和色彩映射数据
    file.seekg(idLength + colorMapType * colorMapLength, std::ios::cur);
    
    // 目前只支持无压缩的RGB/RGBA图像
    if (imageType != 2 && imageType != 3) {
        std::cerr << "目前仅支持非压缩的RGB/RGBA TGA图像: " << filename << std::endl;
        return false;
    }
    
    // 设置纹理属性
    this->width = width;
    this->height = height;
    this->channels = bitsPerPixel / 8;
    
    // 准备数据数组
    size_t dataSize = width * height * channels;
    data.resize(dataSize);
    
    // 读取像素数据
    file.read(reinterpret_cast<char*>(data.data()), dataSize);
    
    // TGA存储格式是BGR或BGRA，需要转换为RGB或RGBA
    for (size_t i = 0; i < dataSize; i += channels) {
        std::swap(data[i], data[i + 2]); // 交换B和R
    }
    
    // 根据imageDescriptor检查是否需要垂直翻转
    //TODO 也可能是 Renderer 的坐标问题导致图像垂直翻转了，之后需要检查，这里简单的将 !(imageDescriptor & 0x20) 修改为(imageDescriptor & 0x20)
    bool flipVertically = (imageDescriptor & 0x20);
    if (flipVertically) {
        // 垂直翻转图像
        std::vector<uint8_t> flippedData(dataSize);
        int rowSize = width * channels;
        
        for (int y = 0; y < height; y++) {
            int srcRow = height - 1 - y;
            std::memcpy(flippedData.data() + y * rowSize, 
                         data.data() + srcRow * rowSize, 
                         rowSize);
        }
        
        data = std::move(flippedData);
    }
    
    std::cout << "已加载纹理: " << filename << " (" << width << "x" << height 
              << ", " << static_cast<int>(bitsPerPixel) << "位每像素)" << std::endl;
    
    return true;
}

// 采样纹理（使用双线性插值）
Color Texture::sample(float2 uv) const {
    return sampleBilinear(uv.x, uv.y);
}

// 最近点采样
Color Texture::sampleNearest(float u, float v) const {
    if (data.empty() || width <= 0 || height <= 0) {
        return Color(255, 0, 255); // 返回粉色表示错误
    }
    
    // 重复纹理坐标 (Wrap/Repeat模式)
    u = u - std::floor(u);
    v = v - std::floor(v);
    
    // 计算纹理坐标
    int x = static_cast<int>(u * width);
    int y = static_cast<int>(v * height);
    
    // 限制在有效范围内
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    
    // 计算像素索引
    size_t index = (y * width + x) * channels;
    
    // 返回颜色
    if (channels == 3) {
        return Color(data[index], data[index + 1], data[index + 2]);
    } else if (channels == 4) {
        return Color(data[index], data[index + 1], data[index + 2], data[index + 3]);
    } else {
        // 对于其他通道数的简单处理
        uint8_t gray = channels == 1 ? data[index] : 255;
        return Color(gray, gray, gray);
    }
}

// 双线性插值采样
Color Texture::sampleBilinear(float u, float v) const {
    if (data.empty() || width <= 0 || height <= 0) {
        return Color(255, 0, 255); // 返回粉色表示错误
    }
    
    // 重复纹理坐标 (Wrap/Repeat模式)
    u = u - std::floor(u);
    v = v - std::floor(v);
    
    // 计算纹理坐标和小数部分
    float xf = u * width - 0.5f;
    float yf = v * height - 0.5f;
    
    int x = static_cast<int>(std::floor(xf));
    int y = static_cast<int>(std::floor(yf));
    
    float fx = xf - x;
    float fy = yf - y;
    
    // 处理环绕
    int x0 = (x % width + width) % width;
    int y0 = (y % height + height) % height;
    int x1 = ((x + 1) % width + width) % width;
    int y1 = ((y + 1) % height + height) % height;
    
    // 获取四个最近的像素
    Color c00 = sampleNearest(static_cast<float>(x0) / width, static_cast<float>(y0) / height);
    Color c10 = sampleNearest(static_cast<float>(x1) / width, static_cast<float>(y0) / height);
    Color c01 = sampleNearest(static_cast<float>(x0) / width, static_cast<float>(y1) / height);
    Color c11 = sampleNearest(static_cast<float>(x1) / width, static_cast<float>(y1) / height);
    
    // 双线性插值
    Color c0 = c00.blend(c10, fx);
    Color c1 = c01.blend(c11, fx);
    return c0.blend(c1, fy);
}

// 加载纹理工厂函数
std::shared_ptr<Texture> loadTexture(const std::string& filename, TextureType type) {
    auto texture = std::make_shared<Texture>();
    if (texture->loadFromTGA(filename)) {
        texture->setType(type);
        return texture;
    }
    return nullptr;
}