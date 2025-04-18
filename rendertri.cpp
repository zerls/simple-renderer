#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>

// 简单的向量结构
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

// 颜色结构
struct Color {
    unsigned char r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(unsigned char r, unsigned char g, unsigned char b) : r(r), g(g), b(b) {}
};

// 三角形结构
struct Triangle {
    Vec3 v0, v1, v2;
    Color c0, c1, c2;
    
    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2,
             const Color& c0, const Color& c1, const Color& c2)
        : v0(v0), v1(v1), v2(v2), c0(c0), c1(c1), c2(c2) {}
};

// 检查点是否在三角形内部（使用重心坐标）
bool pointInTriangle(int x, int y, const Vec3& v0, const Vec3& v1, const Vec3& v2,
                     float& w0, float& w1, float& w2) {
    // 计算重心坐标
    float denominator = ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
    
    // 避免除以零
    if (std::abs(denominator) < 1e-6)
        return false;
    
    w0 = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denominator;
    w1 = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denominator;
    w2 = 1.0f - w0 - w1;
    
    // 如果所有权重都在[0,1]范围内，则点在三角形内部
    return (w0 >= 0 && w1 >= 0 && w2 >= 0);
}

// 插值获取像素颜色
Color interpolateColor(const Color& c0, const Color& c1, const Color& c2,
                       float w0, float w1, float w2) {
    return Color(
        w0 * c0.r + w1 * c1.r + w2 * c2.r,
        w0 * c0.g + w1 * c1.g + w2 * c2.g,
        w0 * c0.b + w1 * c1.b + w2 * c2.b
    );
}

// 渲染单个三角形到PPM文件
bool renderTriangleToPPM(const Triangle& triangle, int width, int height, const std::string& filename) {
    // 创建缓冲区，初始化为黑色
    std::vector<Color> buffer(width * height, Color(0, 0, 0));
    
    // 查找包围盒
    int minX = std::max(0, static_cast<int>(std::min({triangle.v0.x, triangle.v1.x, triangle.v2.x})));
    int maxX = std::min(width - 1, static_cast<int>(std::max({triangle.v0.x, triangle.v1.x, triangle.v2.x})));
    int minY = std::max(0, static_cast<int>(std::min({triangle.v0.y, triangle.v1.y, triangle.v2.y})));
    int maxY = std::min(height - 1, static_cast<int>(std::max({triangle.v0.y, triangle.v1.y, triangle.v2.y})));
    
    // 光栅化三角形
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            float w0, w1, w2;
            if (pointInTriangle(x, y, triangle.v0, triangle.v1, triangle.v2, w0, w1, w2)) {
                // 插值获取像素颜色
                Color color = interpolateColor(triangle.c0, triangle.c1, triangle.c2, w0, w1, w2);
                buffer[y * width + x] = color;
            }
        }
    }
    
    // 将缓冲区写入PPM文件
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件：" << filename << std::endl;
        return false;
    }
    
    // 写入PPM头部
    file << "P3\n" << width << " " << height << "\n255\n";
    
    // 写入像素数据
    for (const auto& color : buffer) {
        file << static_cast<int>(color.r) << " "
             << static_cast<int>(color.g) << " "
             << static_cast<int>(color.b) << "\n";
    }
    
    file.close();
    return true;
}

int main() {
    const int width = 400;
    const int height = 300;
    
    // 定义屏幕坐标系中的三角形（不是NDC）
    Triangle triangle(
        Vec3(100, 100, 0),  // 左下
        Vec3(300, 100, 0),  // 右下
        Vec3(200, 250, 0),  // 顶部
        Color(255, 0, 0),   // 红色
        Color(0, 255, 0),   // 绿色
        Color(0, 0, 255)    // 蓝色
    );
    
    // 渲染三角形到PPM文件
    std::string filename = "triangle.ppm";
    if (renderTriangleToPPM(triangle, width, height, filename)) {
        std::cout << "三角形已渲染到文件：" << filename << std::endl;
    } else {
        std::cerr << "渲染失败" << std::endl;
    }
    
    return 0;
}