// Renderer.h
// 基本光栅化渲染器的头文件

#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <array>

// 定义颜色结构体
struct Color {
    uint8_t r, g, b, a;
    
    // 构造函数
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    
    // 颜色混合
    Color blend(const Color& other, float factor) const {
        float inv_factor = 1.0f - factor;
        return Color(
            static_cast<uint8_t>(r * inv_factor + other.r * factor),
            static_cast<uint8_t>(g * inv_factor + other.g * factor),
            static_cast<uint8_t>(b * inv_factor + other.b * factor),
            static_cast<uint8_t>(a * inv_factor + other.a * factor)
        );
    }
};

// 定义向量结构体
struct Vec2 {
    float x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

// 矩阵结构体，按照指定格式 m.m01, m.m23, m.m02
struct Mat4 {
    // 第一行
    float m00, m01, m02, m03;
    // 第二行
    float m10, m11, m12, m13;
    // 第三行
    float m20, m21, m22, m23;
    // 第四行
    float m30, m31, m32, m33;
    
    // 默认构造为单位矩阵
    Mat4() : 
        m00(1), m01(0), m02(0), m03(0),
        m10(0), m11(1), m12(0), m13(0),
        m20(0), m21(0), m22(1), m23(0),
        m30(0), m31(0), m32(0), m33(1) {}
    
    // 静态方法创建单位矩阵
    static Mat4 identity() {
        return Mat4();
    }
};

// 顶点结构体
struct Vertex {
    Vec3 position;  // 位置
    Vec3 normal;    // 法线
    Vec2 texCoord;  // 纹理坐标
    Color color;    // 顶点颜色
    
    Vertex() = default;
    Vertex(const Vec3& pos, const Color& col) : position(pos), color(col) {}
    Vertex(const Vec3& pos, const Vec3& norm, const Vec2& tex, const Color& col) : 
        position(pos), normal(norm), texCoord(tex), color(col) {}
};

// 三角形结构体
struct Triangle {
    std::array<Vertex, 3> vertices;
    
    Triangle() = default;
    Triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
    }
};

// 帧缓冲类
class FrameBuffer {
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer() = default;
    
    // 设置像素
    void setPixel(int x, int y, const Color& color);
    // 清除缓冲区
    void clear(const Color& color = Color(0, 0, 0, 255));
    // 获取缓冲区数据
    const uint8_t* getData() const { return frameData.data(); }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width;
    int height;
    std::vector<uint8_t> frameData; // 按 RGBA 格式存储
};

// 光栅化渲染器类
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer() = default;
    
    // 绘制三角形
    void drawTriangle(const Triangle& triangle);
    // 清除屏幕
    void clear(const Color& color = Color(0, 0, 0, 255));
    // 获取帧缓冲
    const FrameBuffer& getFrameBuffer() const { return *frameBuffer; }
    
private:
    std::unique_ptr<FrameBuffer> frameBuffer;
};