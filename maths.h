// #pragma once
#ifndef MATHS_H
#define MATHS_H
#include <cstdint>

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

#define float3 Vec3
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

    
    // 向量的元素逐个相乘 - 使用运算符重载
    friend Vec3 operator*(const Vec3& a, const Vec3& b) {
        return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
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

// Vec3 transform(const Matrix4x4& matrix, const Vec3& vector, float w = 1.0f);
// Vec3 transformNoDiv(const Matrix4x4& matrix, const Vec3& vector, float w = 1.0f);
// Vec3 transformNormal(const Matrix4x4 &modelMatrix, const Vec3 &normal);

#endif