#ifndef MATHS_H
#define MATHS_H
#include <cstdint>
#include <cmath>
#include <concepts>
#include <array>
#include <iostream>


// 通用向量模板
template<typename T, size_t N>
class Vector {
protected:
    std::array<T, N> data;

public:
    // 默认构造函数
    constexpr Vector() : data{} {}

    // 可变参数构造函数
    template<typename... Args>
    requires (sizeof...(Args) == N) && (std::convertible_to<Args, T> && ...)
    constexpr Vector(Args... args) : data{static_cast<T>(args)...} {}

    // 元素访问
    constexpr T& operator[](size_t index) { return data[index]; }
    constexpr const T& operator[](size_t index) const { return data[index]; }

    // 点积
    constexpr T dot(const Vector<T, N>& other) const {
        T result{};
        for (size_t i = 0; i < N; ++i) {
            result += data[i] * other.data[i];
        }
        return result;
    }

    // 向量长度的平方
    constexpr T length_squared() const {
        T result{};
        for (size_t i = 0; i < N; ++i) {
            result += data[i] * data[i];
        }
        return result;
    }

    // 向量长度
    constexpr T length() const {
        return std::sqrt(length_squared());
    }

    // 归一化
    Vector<T, N> normalize() const {
        T len = length();
        if (len < static_cast<T>(1e-6))
            return Vector<T, N>{};
            
        Vector<T, N> result;
        T invLen = static_cast<T>(1) / len;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] * invLen;
        }
        return result;
    }

    // 运算符重载
    Vector<T, N> operator+(const Vector<T, N>& other) const {
        Vector<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] + other.data[i];
        }
        return result;
    }

    Vector<T, N> operator-(const Vector<T, N>& other) const {
        Vector<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] - other.data[i];
        }
        return result;
    }

    Vector<T, N> operator*(T scalar) const {
        Vector<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] * scalar;
        }
        return result;
    }

    Vector<T, N> operator*(const Vector<T, N>& other) const {
        Vector<T, N> result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] * other.data[i];
        }
        return result;
    }
};

// Vec2 实现
template<typename T = float>
class Vec2 {
public:
    T x, y;

    // 构造函数
    constexpr Vec2() : x(0), y(0) {}
    constexpr Vec2(T x, T y) : x(x), y(y) {}

    // 点积
    constexpr T dot(const Vec2<T>& other) const {
        return x * other.x + y * other.y;
    }

    // 向量长度的平方
    constexpr T length_squared() const {
        return x * x + y * y;
    }

    // 向量长度
    constexpr T length() const {
        return std::sqrt(length_squared());
    }

    // 归一化
    Vec2<T> normalize() const;

    // 运算符重载
    Vec2<T> operator+(const Vec2<T>& other) const {
        return Vec2<T>(x + other.x, y + other.y);
    }

    Vec2<T> operator-(const Vec2<T>& other) const {
        return Vec2<T>(x - other.x, y - other.y);
    }

    Vec2<T> operator*(T scalar) const {
        return Vec2<T>(x * scalar, y * scalar);
    }

    Vec2<T> operator*(const Vec2<T>& other) const {
        return Vec2<T>(x * other.x, y * other.y);
    }
};

// Vec3 实现
template<typename T = float>
class Vec3 {
public:
    T x, y, z;

    // 构造函数
    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

    // 点积
    constexpr T dot(const Vec3<T>& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // 叉积
    Vec3<T> cross(const Vec3<T>& other) const;

    // 向量长度的平方
    constexpr T length_squared() const {
        return x * x + y * y + z * z;
    }

    // 向量长度
    constexpr T length() const {
        return std::sqrt(length_squared());
    }

    // 归一化
    Vec3<T> normalize() const;

    // 运算符重载
    Vec3<T> operator+(const Vec3<T>& other) const {
        return Vec3<T>(x + other.x, y + other.y, z + other.z);
    }

    Vec3<T> operator-(const Vec3<T>& other) const {
        return Vec3<T>(x - other.x, y - other.y, z - other.z);
    }

    Vec3<T> operator*(T scalar) const {
        return Vec3<T>(x * scalar, y * scalar, z * scalar);
    }

    Vec3<T> operator*(const Vec3<T>& other) const {
        return Vec3<T>(x * other.x, y * other.y, z * other.z);
    }
};

// Vec4 实现
template<typename T = float>
class Vec4 {
public:
    T x, y, z, w;

    // 构造函数
    constexpr Vec4() : x(0), y(0), z(0), w(0) {}
    constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    constexpr Vec4(const Vec3<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}

    // 点积
    constexpr T dot(const Vec4<T>& other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    // 向量长度的平方
    constexpr T length_squared() const {
        return x * x + y * y + z * z + w * w;
    }

    // 向量长度
    constexpr T length() const {
        return std::sqrt(length_squared());
    }

    // 归一化
    Vec4<T> normalize() const;

    // 运算符重载
    Vec4<T> operator+(const Vec4<T>& other) const {
        return Vec4<T>(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vec4<T> operator-(const Vec4<T>& other) const {
        return Vec4<T>(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vec4<T> operator*(T scalar) const {
        return Vec4<T>(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vec4<T> operator*(const Vec4<T>& other) const {
        return Vec4<T>(x * other.x, y * other.y, z * other.z, w * other.w);
    }
};

// 全局向量操作函数
template<typename T>
Vec2<T> normalize(const Vec2<T>& v);

template<typename T>
Vec3<T> normalize(const Vec3<T>& v);

template<typename T>
Vec4<T> normalize(const Vec4<T>& v);

template<typename T>
T dot(const Vec2<T>& a, const Vec2<T>& b);

template<typename T>
T dot(const Vec3<T>& a, const Vec3<T>& b);

template<typename T>
T dot(const Vec4<T>& a, const Vec4<T>& b);

template<typename T>
Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b);

// 类型别名，兼容原代码
using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;
using float2 = Vec2<float>;
using float3 = Vec3<float>;
using float4 = Vec4<float>;

// 通用矩阵模板
template<typename T, size_t ROWS, size_t COLS>
class Matrix {
protected:
    std::array<T, ROWS * COLS> data;

public:
    // 默认构造函数 - 单位矩阵
    constexpr Matrix() : data{} {
        if constexpr (ROWS == COLS) {
            for (size_t i = 0; i < ROWS; ++i) {
                data[i * COLS + i] = static_cast<T>(1);
            }
        }
    }

    // 元素访问
    constexpr T& operator()(size_t row, size_t col) { 
        return data[row * COLS + col]; 
    }
    
    constexpr const T& operator()(size_t row, size_t col) const { 
        return data[row * COLS + col]; 
    }

    // 矩阵乘法
    template<size_t OTHER_COLS>
    Matrix<T, ROWS, OTHER_COLS> operator*(const Matrix<T, COLS, OTHER_COLS>& other) const;

    // 单位矩阵
    static Matrix<T, ROWS, COLS> identity();
};

// Matrix3x3 特化
template<typename T = float>
class Matrix3x3 {
public:
    union {
        struct {
            T m00, m01, m02;
            T m10, m11, m12;
            T m20, m21, m22;
        };
        T m[9];
    };

    // 默认构造函数 - 单位矩阵
    constexpr Matrix3x3() : 
        m00(1), m01(0), m02(0),
        m10(0), m11(1), m12(0),
        m20(0), m21(0), m22(1) {}
        
    // 从9个数值构造
    constexpr Matrix3x3(
        T _m00, T _m01, T _m02,
        T _m10, T _m11, T _m12,
        T _m20, T _m21, T _m22) :
        m00(_m00), m01(_m01), m02(_m02),
        m10(_m10), m11(_m11), m12(_m12),
        m20(_m20), m21(_m21), m22(_m22) {}
        
    // 矩阵乘法
    Matrix3x3<T> operator*(const Matrix3x3<T>& other) const;

    // 单位矩阵
    static Matrix3x3<T> identity();
    
    // 平移矩阵 (2D)
    static Matrix3x3<T> translation(T x, T y);

    // 缩放矩阵
    static Matrix3x3<T> scaling(T x, T y);

    // 旋转矩阵 (2D)
    static Matrix3x3<T> rotation(T angle);
    
    // 行列式
    T determinant() const;
    
    // 逆矩阵
    Matrix3x3<T> inverse() const;
};

// Matrix4x4 特化
template<typename T = float>
class Matrix4x4 {
public:
    union {
        struct {
            T m00, m01, m02, m03;
            T m10, m11, m12, m13;
            T m20, m21, m22, m23;
            T m30, m31, m32, m33;
        };
        T m[16];
    };
    
    // 默认构造函数 - 单位矩阵
    constexpr Matrix4x4() : 
        m00(1), m01(0), m02(0), m03(0),
        m10(0), m11(1), m12(0), m13(0),
        m20(0), m21(0), m22(1), m23(0),
        m30(0), m31(0), m32(0), m33(1) {}
        
    // 从16个数值构造
    constexpr Matrix4x4(
        T _m00, T _m01, T _m02, T _m03,
        T _m10, T _m11, T _m12, T _m13,
        T _m20, T _m21, T _m22, T _m23,
        T _m30, T _m31, T _m32, T _m33) :
        m00(_m00), m01(_m01), m02(_m02), m03(_m03),
        m10(_m10), m11(_m11), m12(_m12), m13(_m13),
        m20(_m20), m21(_m21), m22(_m22), m23(_m23),
        m30(_m30), m31(_m31), m32(_m32), m33(_m33) {}
    
    // 矩阵乘法
    Matrix4x4<T> operator*(const Matrix4x4<T>& other) const;

    // 单位矩阵
    static Matrix4x4<T> identity();
    
    // 平移矩阵
    static Matrix4x4<T> translation(T x, T y, T z);

    // 缩放矩阵
    static Matrix4x4<T> scaling(T x, T y, T z);

    // 绕X轴旋转
    static Matrix4x4<T> rotationX(T angle);

    // 绕Y轴旋转
    static Matrix4x4<T> rotationY(T angle);

    // 绕Z轴旋转
    static Matrix4x4<T> rotationZ(T angle);

    // 透视投影矩阵
    static Matrix4x4<T> perspective(T fovY, T aspect, T zNear, T zFar);

    // 视图矩阵
    static Matrix4x4<T> lookAt(const Vec3<T>& eye, const Vec3<T>& target, const Vec3<T>& up);
};

// 类型别名，兼容原代码
using Matrix3x3f = Matrix3x3<float>;
using Matrix4x4f = Matrix4x4<float>;

// 添加辅助函数声明
Vec3f transform(const Matrix4x4f& matrix, const Vec3f& vector, float w = 1.0f);
Vec3f transformNoDiv(const Matrix4x4f& matrix, const Vec3f& vector, float w = 1.0f);
Vec3f transformNormal(const Matrix4x4f& modelMatrix, const Vec3f& normal);

// Shader Common
float smoothstep(float edge0, float edge1, float x);

// 新增的模板函数声明 - smoothstep 向量特化
template<typename T>
T smoothstep(T edge0, T edge1, T x);

template<>
Vec2f smoothstep(Vec2f edge0, Vec2f edge1, Vec2f x);

template<>
Vec3f smoothstep(Vec3f edge0, Vec3f edge1, Vec3f x);

template<>
Vec4f smoothstep(Vec4f edge0, Vec4f edge1, Vec4f x);

#endif