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
    
    // 点积
    friend float dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    // 叉积
    friend Vec3 cross(const Vec3& a, const Vec3& b) {
        return Vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    
    // 向量长度
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    // 向量归一化
    friend Vec3 normalize(const Vec3& v) {
        float len = v.length();
        if (len < 1e-6f) return Vec3(0, 0, 0);
        float invLen = 1.0f / len;
        return Vec3(v.x * invLen, v.y * invLen, v.z * invLen);
    }
    
    // 标量乘法
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    
    // 向量加法
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    // 向量减法
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
};

// 矩阵结构体扩展

struct Matrix4x4 {
    union {
        struct {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
        float m[16];
    };
    // 默认构造函数 - 单位矩阵
    Matrix4x4() {
        m00 = 1.0f; m01 = 0.0f; m02 = 0.0f; m03 = 0.0f;
        m10 = 0.0f; m11 = 1.0f; m12 = 0.0f; m13 = 0.0f;
        m20 = 0.0f; m21 = 0.0f; m22 = 1.0f; m23 = 0.0f;
        m30 = 0.0f; m31 = 0.0f; m32 = 0.0f; m33 = 1.0f;
    }
    
    // 从16个浮点数构造矩阵
    Matrix4x4(
        float _m00, float _m01, float _m02, float _m03,
        float _m10, float _m11, float _m12, float _m13,
        float _m20, float _m21, float _m22, float _m23,
        float _m30, float _m31, float _m32, float _m33
    ) {
        m00 = _m00; m01 = _m01; m02 = _m02; m03 = _m03;
        m10 = _m10; m11 = _m11; m12 = _m12; m13 = _m13;
        m20 = _m20; m21 = _m21; m22 = _m22; m23 = _m23;
        m30 = _m30; m31 = _m31; m32 = _m32; m33 = _m33;
    }
    
    // 单位矩阵
    static Matrix4x4 identity() {
        return Matrix4x4();
    }
    
    // 矩阵乘法
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row * 4 + col] = 
                    m[row * 4 + 0] * other.m[0 * 4 + col] +
                    m[row * 4 + 1] * other.m[1 * 4 + col] +
                    m[row * 4 + 2] * other.m[2 * 4 + col] +
                    m[row * 4 + 3] * other.m[3 * 4 + col];
            }
        }
        
        return result;
    }
    
    // 平移矩阵
    static Matrix4x4 translation(float x, float y, float z) {
        Matrix4x4 result;
        result.m03 = x;
        result.m13 = y;
        result.m23 = z;
        return result;
    }
    
    // 缩放矩阵
    static Matrix4x4 scaling(float x, float y, float z) {
        Matrix4x4 result;
        result.m00 = x;
        result.m11 = y;
        result.m22 = z;
        return result;
    }
    
    // 绕X轴旋转
    static Matrix4x4 rotationX(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        Matrix4x4 result;
        result.m11 = c;
        result.m12 = -s;
        result.m21 = s;
        result.m22 = c;
        return result;
    }
    
    // 绕Y轴旋转
    static Matrix4x4 rotationY(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        Matrix4x4 result;
        result.m00 = c;
        result.m02 = s;
        result.m20 = -s;
        result.m22 = c;
        return result;
    }
    
    // 绕Z轴旋转
    static Matrix4x4 rotationZ(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        Matrix4x4 result;
        result.m00 = c;
        result.m01 = -s;
        result.m10 = s;
        result.m11 = c;
        return result;
    }
    
    // 透视投影矩阵
    static Matrix4x4 perspective(float fovY, float aspect, float zNear, float zFar) {
        float tanHalfFovY = std::tan(fovY / 2);
        
        Matrix4x4 result;
        result.m00 = 1.0f / (aspect * tanHalfFovY);
        result.m11 = 1.0f / tanHalfFovY;
        result.m22 = -(zFar + zNear) / (zFar - zNear);
        result.m23 = -(2.0f * zFar * zNear) / (zFar - zNear);
        result.m32 = -1.0f;
        result.m33 = 0.0f;
        return result;
    }
    
    // 视图矩阵 (简化版，只考虑位置和目标)
    static Matrix4x4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        // 计算相机坐标系
        Vec3 zaxis = normalize(Vec3(
            eye.x - target.x,
            eye.y - target.y,
            eye.z - target.z
        ));
        Vec3 xaxis = normalize(cross(up, zaxis));
        Vec3 yaxis = cross(zaxis, xaxis);
        
        // 构建视图矩阵
        Matrix4x4 result;
        result.m00 = xaxis.x;  result.m01 = xaxis.y;  result.m02 = xaxis.z;  result.m03 = -dot(xaxis, eye);
        result.m10 = yaxis.x;  result.m11 = yaxis.y;  result.m12 = yaxis.z;  result.m13 = -dot(yaxis, eye);
        result.m20 = zaxis.x;  result.m21 = zaxis.y;  result.m22 = zaxis.z;  result.m23 = -dot(zaxis, eye);
        result.m30 = 0.0f;    result.m31 = 0.0f;    result.m32 = 0.0f;    result.m33 = 1.0f;
        
        return result;
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


// 立方体结构体
struct Cube {
        std::array<Vec3, 8> vertices; // 8个顶点
        std::array<std::array<int, 3>, 12> indices; // 12个三角形(6个面，每个面2个三角形)
        std::array<Color, 6> faceColors; // 6个面的颜色
        
        // 创建单位立方体
        static Cube createUnitCube() {
            Cube cube;
            
            // 定义8个顶点(以原点为中心)
            cube.vertices[0] = Vec3(-0.5f, -0.5f, -0.5f); // 前左下
            cube.vertices[1] = Vec3( 0.5f, -0.5f, -0.5f); // 前右下
            cube.vertices[2] = Vec3( 0.5f,  0.5f, -0.5f); // 前右上
            cube.vertices[3] = Vec3(-0.5f,  0.5f, -0.5f); // 前左上
            cube.vertices[4] = Vec3(-0.5f, -0.5f,  0.5f); // 后左下
            cube.vertices[5] = Vec3( 0.5f, -0.5f,  0.5f); // 后右下
            cube.vertices[6] = Vec3( 0.5f,  0.5f,  0.5f); // 后右上
            cube.vertices[7] = Vec3(-0.5f,  0.5f,  0.5f); // 后左上
            
            // 定义12个三角形(6个面，每个面2个三角形)
            // 前面(z = -0.5)
            cube.indices[0] = {0, 1, 2};
            cube.indices[1] = {0, 2, 3};
            
            // 后面(z = 0.5)
            cube.indices[2] = {5, 4, 7};
            cube.indices[3] = {5, 7, 6};
            
            // 左面(x = -0.5)
            cube.indices[4] = {4, 0, 3};
            cube.indices[5] = {4, 3, 7};
            
            // 右面(x = 0.5)
            cube.indices[6] = {1, 5, 6};
            cube.indices[7] = {1, 6, 2};
            
            // 上面(y = 0.5)
            cube.indices[8] = {3, 2, 6};
            cube.indices[9] = {3, 6, 7};
            
            // 下面(y = -0.5)
            cube.indices[10] = {4, 5, 1};
            cube.indices[11] = {4, 1, 0};
            
            // 设置面的颜色
            cube.faceColors[0] = Color(255, 0, 0);     // 前面 - 红色
            cube.faceColors[1] = Color(0, 255, 0);     // 后面 - 绿色
            cube.faceColors[2] = Color(0, 0, 255);     // 左面 - 蓝色
            cube.faceColors[3] = Color(255, 255, 0);   // 右面 - 黄色
            cube.faceColors[4] = Color(0, 255, 255);   // 上面 - 青色
            cube.faceColors[5] = Color(255, 0, 255);   // 下面 - 紫色
            
            return cube;
        }
    };
      

// 帧缓冲类
class FrameBuffer {
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer() = default;
    
    // 设置像素（带深度测试）
    void setPixel(int x, int y, float depth, const Color& color);
    // 设置像素（不带深度测试）
    void setPixel(int x, int y, const Color& color);
    // 获取深度值
    float getDepth(int x, int y) const;
    // 清除缓冲区
    void clear(const Color& color = Color(0, 0, 0, 255), float depth = 1.0f);
    // 获取帧缓冲区数据
    const uint8_t* getData() const { return frameData.data(); }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width;
    int height;
    std::vector<uint8_t> frameData; // 按 RGBA 格式存储
    std::vector<float> depthBuffer; // 深度缓冲区
    
    // 检查坐标是否在有效范围内
    bool isValidCoord(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    
    // 计算索引
    int calcIndex(int x, int y) const {
        return y * width + x;
    }
};

// 光栅化渲染器类
class Renderer {
    public:
        Renderer(int width, int height);
        ~Renderer() = default;
        
        // 绘制变换后的三角形
        void drawTriangle(const Triangle& triangle, const Matrix4x4& mvpMatrix);
        
        // 绘制未变换的三角形(原始方法)
        void drawTriangle(const Triangle& triangle);
        
        // 清除屏幕
        void clear(const Color& color = Color(0, 0, 0, 255));
        
        // 设置变换矩阵
        void setModelMatrix(const Matrix4x4& matrix) { modelMatrix = matrix; }
        void setViewMatrix(const Matrix4x4& matrix) { viewMatrix = matrix; }
        void setProjMatrix(const Matrix4x4& matrix) { projMatrix = matrix; }
        
        // 获取当前变换矩阵
        Matrix4x4 getModelMatrix() const { return modelMatrix; }
        Matrix4x4 getViewMatrix() const { return viewMatrix; }
        Matrix4x4 getProjMatrix() const { return projMatrix; }
        Matrix4x4 getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }
        
        // 变换顶点位置
        Vec3 transformVertex(const Vec3& position, const Matrix4x4& mvpMatrix);
        
        // 获取帧缓冲
        const FrameBuffer& getFrameBuffer() const { return *frameBuffer; }

        
        void drawCube(const Cube& cube);
        
    private:
        std::unique_ptr<FrameBuffer> frameBuffer;
        Matrix4x4 modelMatrix; // 模型变换
        Matrix4x4 viewMatrix;  // 视图变换
        Matrix4x4 projMatrix;  // 投影变换
    };
