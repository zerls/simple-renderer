// Renderer.cpp
// 基本光栅化渲染器的实现文件

#include "Renderer.h"
#include <algorithm>
#include <cmath>

// FrameBuffer 实现
FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height) {
    // 初始化帧缓冲区，每个像素4个字节 (RGBA)
    frameData.resize(width * height * 4, 0);
}

void FrameBuffer::setPixel(int x, int y, const Color& color) {
    // 确保坐标在有效范围内
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    
    // 计算像素在帧缓冲区中的位置
    // 注意：y坐标从上到下，所以需要反转
    int index = (y * width + x) * 4;
    
    // 设置RGBA值
    frameData[index] = color.r;
    frameData[index + 1] = color.g;
    frameData[index + 2] = color.b;
    frameData[index + 3] = color.a;
}

void FrameBuffer::clear(const Color& color) {
    // 使用指定颜色清除整个帧缓冲区
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            setPixel(x, y, color);
        }
    }
}

// Renderer 实现
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)) {
}

void Renderer::clear(const Color& color) {
    frameBuffer->clear(color);
}

// 绘制三角形的简单实现
void Renderer::drawTriangle(const Triangle& triangle) {
    // 提取三角形的三个顶点
    const Vertex& v1 = triangle.vertices[0];
    const Vertex& v2 = triangle.vertices[1];
    const Vertex& v3 = triangle.vertices[2];
    
    // 找到三角形的包围盒
    int minX = static_cast<int>(std::min({v1.position.x, v2.position.x, v3.position.x}));
    int minY = static_cast<int>(std::min({v1.position.y, v2.position.y, v3.position.y}));
    int maxX = static_cast<int>(std::ceil(std::max({v1.position.x, v2.position.x, v3.position.x})));
    int maxY = static_cast<int>(std::ceil(std::max({v1.position.y, v2.position.y, v3.position.y})));
    
    // 裁剪到屏幕范围
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(frameBuffer->getWidth() - 1, maxX);
    maxY = std::min(frameBuffer->getHeight() - 1, maxY);
    
    // 遍历包围盒中的每个像素
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            // 计算重心坐标
            float x0 = v1.position.x;
            float y0 = v1.position.y;
            float x1 = v2.position.x;
            float y1 = v2.position.y;
            float x2 = v3.position.x;
            float y2 = v3.position.y;
            
            // 计算面积的两倍（叉积）
            float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
            
            // 如果面积为0，则三角形是一条线或一个点
            if (std::abs(area) < 1e-6) {
                continue;
            }
            
            // 计算重心坐标
            float w0 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            float w1 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            float w2 = 1.0f - w0 - w1;
            
            // 检查点是否在三角形内
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // 使用重心坐标插值顶点颜色
                Color color(
                    static_cast<uint8_t>(w0 * v1.color.r + w1 * v2.color.r + w2 * v3.color.r),
                    static_cast<uint8_t>(w0 * v1.color.g + w1 * v2.color.g + w2 * v3.color.g),
                    static_cast<uint8_t>(w0 * v1.color.b + w1 * v2.color.b + w2 * v3.color.b),
                    static_cast<uint8_t>(w0 * v1.color.a + w1 * v2.color.a + w2 * v3.color.a)
                );
                
                // 设置像素颜色
                frameBuffer->setPixel(x, y, color);
            }
        }
    }
}