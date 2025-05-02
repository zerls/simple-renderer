#ifndef MATHS_H
#define MATHS_H

#include <cstdint>
#include <cmath>
#include <concepts>
#include <array>
#include <iostream>

// Vec2 实现
template <typename T = float>
class Vec2
{
public:
    T x, y;
    // 构造函数
    constexpr Vec2() : x(0), y(0) {}
    constexpr Vec2(T x, T y) : x(x), y(y) {}

    constexpr T dot(const Vec2<T> &other) const { return x * other.x + y * other.y; } // 点积
    constexpr T length_squared() const { return x * x + y * y; }                      // 向量长度的平方
    T length() const { return std::sqrt(length_squared()); }                          // 向量长度

    // 归一化
    Vec2<T> normalize() const
    {
        T len = length();
        if (len < static_cast<T>(1e-6))
            return Vec2<T>();
        T invLen = static_cast<T>(1) / len;
        return Vec2<T>(x * invLen, y * invLen);
    }

    // 运算符重载
    Vec2<T> operator+(const Vec2<T> &other) const { return Vec2<T>(x + other.x, y + other.y); }
    Vec2<T> operator-(const Vec2<T> &other) const { return Vec2<T>(x - other.x, y - other.y); }
    Vec2<T> operator*(T scalar) const { return Vec2<T>(x * scalar, y * scalar); }
    Vec2<T> operator*(const Vec2<T> &other) const { return Vec2<T>(x * other.x, y * other.y); }
    Vec2<T> operator/(T scalar) const
    {
        T invScalar = static_cast<T>(1) / scalar;
        return Vec2<T>(x * invScalar, y * invScalar);
    }
    
    // 复合赋值运算符
    Vec2<T>& operator+=(const Vec2<T> &other) { x += other.x; y += other.y; return *this; }
    Vec2<T>& operator-=(const Vec2<T> &other) { x -= other.x; y -= other.y; return *this; }
    Vec2<T>& operator*=(T scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2<T>& operator*=(const Vec2<T> &other) { x *= other.x; y *= other.y; return *this; }
    Vec2<T>& operator/=(T scalar) 
    { 
        T invScalar = static_cast<T>(1) / scalar;
        x *= invScalar; 
        y *= invScalar; 
        return *this; 
    }
};

// Vec3 实现
template <typename T = float>
class Vec3
{
public:
    T x, y, z;
    // 构造函数
    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(T t) : x(t), y(t), z(t) {}
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    constexpr T dot(const Vec3<T> &other) const { return x * other.x + y * other.y + z * other.z; }                                                // 点积
    Vec3<T> cross(const Vec3<T> &other) const { return Vec3<T>(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); } // 叉积
    constexpr T length_squared() const { return x * x + y * y + z * z; }                                                                           // 向量长度的平方
    T length() const { return std::sqrt(length_squared()); }                                                                                       // 向量长度

    // 归一化
    Vec3<T> normalize() const
    {
        T len = length();
        if (len < static_cast<T>(1e-6))
            return Vec3<T>();
        T invLen = static_cast<T>(1) / len;
        return Vec3<T>(x * invLen, y * invLen, z * invLen);
    }

    // 运算符重载
    Vec3<T> operator+(const Vec3<T> &other) const { return Vec3<T>(x + other.x, y + other.y, z + other.z); }
    Vec3<T> operator-(const Vec3<T> &other) const { return Vec3<T>(x - other.x, y - other.y, z - other.z); }
    Vec3<T> operator*(T scalar) const { return Vec3<T>(x * scalar, y * scalar, z * scalar); }
    Vec3<T> operator*(const Vec3<T> &other) const { return Vec3<T>(x * other.x, y * other.y, z * other.z); }
    Vec3<T> operator/(T scalar) const
    {
        T invScalar = static_cast<T>(1) / scalar;
        return Vec3<T>(x * invScalar, y * invScalar, z * invScalar);
    }
    Vec2<T> xy() const { return Vec2<T>(x, y); } // 返回 Vec2<T> 形式的 x, y 分量
    
    // 复合赋值运算符
    Vec3<T>& operator+=(const Vec3<T> &other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3<T>& operator-=(const Vec3<T> &other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3<T>& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vec3<T>& operator*=(const Vec3<T> &other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
    Vec3<T>& operator/=(T scalar) 
    { 
        T invScalar = static_cast<T>(1) / scalar;
        x *= invScalar; 
        y *= invScalar; 
        z *= invScalar;
        return *this; 
    }
};

// Vec4 实现
template <typename T = float>
class Vec4
{
public:
    T x, y, z, w;

    // 构造函数
    constexpr Vec4() : x(0), y(0), z(0), w(0) {}
    constexpr Vec4(T t) : x(t), y(t), z(t), w(t) {}
    constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    constexpr Vec4(const Vec3<T> &v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}

    constexpr T dot(const Vec4<T> &other) const { return x * other.x + y * other.y + z * other.z + w * other.w; } // 点积
    constexpr T length_squared() const { return x * x + y * y + z * z + w * w; }                                  // 向量长度的平方
    T length() const { return std::sqrt(length_squared()); }                                                      // 向量长度

    // 归一化
    Vec4<T> normalize() const
    {
        T len = length();
        if (len < static_cast<T>(1e-6))
            return Vec4<T>();
        T invLen = static_cast<T>(1) / len;
        return Vec4<T>(x * invLen, y * invLen, z * invLen, w * invLen);
    }

    // 运算符重载
    Vec4<T> operator+(const Vec4<T> &other) const { return Vec4<T>(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vec4<T> operator-(const Vec4<T> &other) const { return Vec4<T>(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vec4<T> operator*(T scalar) const { return Vec4<T>(x * scalar, y * scalar, z * scalar, w * scalar); }
    Vec4<T> operator*(const Vec4<T> &other) const { return Vec4<T>(x * other.x, y * other.y, z * other.z, w * other.w); }
    Vec4<T> operator/(T scalar) const
    {
        T invScalar = static_cast<T>(1) / scalar;
        return Vec4<T>(x * invScalar, y * invScalar, z * invScalar, w * invScalar);
    }
    Vec3<T> xyz() const { return Vec3<T>(x, y, z); } // 返回 Vec3<T> 形式的 x, y, z 分量
    
    // 复合赋值运算符
    Vec4<T>& operator+=(const Vec4<T> &other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    Vec4<T>& operator-=(const Vec4<T> &other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
    Vec4<T>& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    Vec4<T>& operator*=(const Vec4<T> &other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this; }
    Vec4<T>& operator/=(T scalar) 
    { 
        T invScalar = static_cast<T>(1) / scalar;
        x *= invScalar; 
        y *= invScalar; 
        z *= invScalar;
        w *= invScalar;
        return *this; 
    }
};

// 全局向量操作函数
template <typename T>
inline Vec2<T> normalize(const Vec2<T> &v) { return v.normalize(); }
template <typename T>
inline Vec3<T> normalize(const Vec3<T> &v) { return v.normalize(); }
template <typename T>
inline Vec4<T> normalize(const Vec4<T> &v) { return v.normalize(); }
template <typename T>
inline T dot(const Vec2<T> &a, const Vec2<T> &b) { return a.dot(b); }
template <typename T>
inline T dot(const Vec3<T> &a, const Vec3<T> &b) { return a.dot(b); }
template <typename T>
inline T dot(const Vec4<T> &a, const Vec4<T> &b) { return a.dot(b); }
template <typename T>
inline Vec3<T> cross(const Vec3<T> &a, const Vec3<T> &b) { return a.cross(b); }

template <typename T>
inline float cross(const Vec2<T> & a, const Vec2<T>& b) {
    return a.x * b.y - a.y * b.x;
}

// 新增全局函数
// min/max 函数
template <typename T>
inline T min(T a, T b) { return a < b ? a : b; }
template <typename T>
inline T max(T a, T b) { return a > b ? a : b; }
template <typename T>
inline Vec2<T> min(const Vec2<T>& a, const Vec2<T>& b) { return Vec2<T>(min(a.x, b.x), min(a.y, b.y)); }
template <typename T>
inline Vec2<T> max(const Vec2<T>& a, const Vec2<T>& b) { return Vec2<T>(max(a.x, b.x), max(a.y, b.y)); }
template <typename T>
inline Vec3<T> min(const Vec3<T>& a, const Vec3<T>& b) { return Vec3<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
template <typename T>
inline Vec3<T> max(const Vec3<T>& a, const Vec3<T>& b) { return Vec3<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
template <typename T>
inline Vec4<T> min(const Vec4<T>& a, const Vec4<T>& b) { return Vec4<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)); }
template <typename T>
inline Vec4<T> max(const Vec4<T>& a, const Vec4<T>& b) { return Vec4<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)); }

// 线性插值函数
template <typename T>
inline T lerp(T a, T b, float t) { return a + (b - a) * t; }
template <typename T>
inline Vec2<T> lerp(const Vec2<T>& a, const Vec2<T>& b, float t) { return a + (b - a) * t; }
template <typename T>
inline Vec3<T> lerp(const Vec3<T>& a, const Vec3<T>& b, float t) { return a + (b - a) * t; }
template <typename T>
inline Vec4<T> lerp(const Vec4<T>& a, const Vec4<T>& b, float t) { return a + (b - a) * t; }

// length 函数
template <typename T>
inline T length(const Vec2<T>& v) { return v.length(); }
template <typename T>
inline T length(const Vec3<T>& v) { return v.length(); }
template <typename T>
inline T length(const Vec4<T>& v) { return v.length(); }

// 类型别名
using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;
using float2 = Vec2<float>;
using float3 = Vec3<float>;
using float4 = Vec4<float>;

// 矩阵类型
template <typename T>
struct Matrix4x4
{
    union
    {
        struct
        {
            T m00, m01, m02, m03;
            T m10, m11, m12, m13;
            T m20, m21, m22, m23;
            T m30, m31, m32, m33;
        };
        T m[16];
    };

    // 默认构造函数 - 单位矩阵
    constexpr Matrix4x4() : m00(1), m01(0), m02(0), m03(0),
                            m10(0), m11(1), m12(0), m13(0),
                            m20(0), m21(0), m22(1), m23(0),
                            m30(0), m31(0), m32(0), m33(1) {}

    static Matrix4x4<T> identity() { return Matrix4x4<T>(); };                                // 单位矩阵
    static Matrix4x4<T> translation(T x, T y, T z);                                           // 平移矩阵
    static Matrix4x4<T> scaling(T x, T y, T z);                                               // 缩放矩阵
    static Matrix4x4<T> rotationX(T angle);                                                   // 绕X轴旋转
    static Matrix4x4<T> rotationY(T angle);                                                   // 绕Y轴旋转
    static Matrix4x4<T> rotationZ(T angle);                                                   // 绕Z轴旋转
    static Matrix4x4<T> perspective(T fovY, T aspect, T zNear, T zFar);                       // 透视投影矩阵
    static Matrix4x4<T> lookAt(const Vec3<T> &eye, const Vec3<T> &target, const Vec3<T> &up); // 视图矩阵
    
    Matrix4x4 operator*(const Matrix4x4 &other) const; // 矩阵乘法
   
    Matrix4x4 transposed() const; // 矩阵转置
    Vec4<T> transform(const Vec4<T>& v) const;  // 变换向量
    Vec4<T> operator*(const Vec4<T>& v) const; // 变换向量
};

using Matrix4x4f = Matrix4x4<float>;

// 添加辅助函数声明
Vec3f transform(const Matrix4x4f &matrix, const Vec3f &vector, float w = 1.0f);
Vec3f transformNoDiv(const Matrix4x4f &matrix, const Vec3f &vector, float w = 1.0f);
Vec3f transformDir(const Matrix4x4f &mat, const Vec3f &dir);
Vec3f transformNormal(const Matrix4x4f &modelMatrix, const Vec3f &normal);


// Shader Common
float smoothstep(float edge0, float edge1, float x);
Vec2f smoothstep(Vec2f edge0, Vec2f edge1, Vec2f x);
Vec3f smoothstep(Vec3f edge0, Vec3f edge1, Vec3f x);
Vec4f smoothstep(Vec4f edge0, Vec4f edge1, Vec4f x);

#endif