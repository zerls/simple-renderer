#include "maths.h"
#include <algorithm> // 为 std::clamp 添加头文件


// Vec2 实现
template<typename T>
Vec2<T> Vec2<T>::normalize() const {
    T len = length();
    if (len < static_cast<T>(1e-6))
        return Vec2<T>();
    T invLen = static_cast<T>(1) / len;
    return Vec2<T>(x * invLen, y * invLen);
}

// 显式实例化常用类型
template Vec2<float> Vec2<float>::normalize() const;

// Vec3 实现
template<typename T>
Vec3<T> Vec3<T>::cross(const Vec3<T>& other) const {
    return Vec3<T>(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

template<typename T>
Vec3<T> Vec3<T>::normalize() const {
    T len = length();
    if (len < static_cast<T>(1e-6))
        return Vec3<T>();
    T invLen = static_cast<T>(1) / len;
    return Vec3<T>(x * invLen, y * invLen, z * invLen);
}

// 显式实例化常用类型
template Vec3<float> Vec3<float>::cross(const Vec3<float>& other) const;
template Vec3<float> Vec3<float>::normalize() const;

// Vec4 实现
template<typename T>
Vec4<T> Vec4<T>::normalize() const {
    T len = length();
    if (len < static_cast<T>(1e-6))
        return Vec4<T>();
    T invLen = static_cast<T>(1) / len;
    return Vec4<T>(x * invLen, y * invLen, z * invLen, w * invLen);
}

// 显式实例化常用类型
template Vec4<float> Vec4<float>::normalize() const;

// 全局向量操作函数实现
template<typename T>
Vec2<T> normalize(const Vec2<T>& v) {
    return v.normalize();
}

template<typename T>
Vec3<T> normalize(const Vec3<T>& v) {
    return v.normalize();
}

template<typename T>
Vec4<T> normalize(const Vec4<T>& v) {
    return v.normalize();
}

template<typename T>
T dot(const Vec2<T>& a, const Vec2<T>& b) {
    return a.dot(b);
}

template<typename T>
T dot(const Vec3<T>& a, const Vec3<T>& b) {
    return a.dot(b);
}

template<typename T>
T dot(const Vec4<T>& a, const Vec4<T>& b) {
    return a.dot(b);
}

template<typename T>
Vec3<T> cross(const Vec3<T>& a, const Vec3<T>& b) {
    return a.cross(b);
}

// 显式实例化常用类型
template Vec2<float> normalize(const Vec2<float>& v);
template Vec3<float> normalize(const Vec3<float>& v);
template Vec4<float> normalize(const Vec4<float>& v);
template float dot(const Vec2<float>& a, const Vec2<float>& b);
template float dot(const Vec3<float>& a, const Vec3<float>& b);
template float dot(const Vec4<float>& a, const Vec4<float>& b);
template Vec3<float> cross(const Vec3<float>& a, const Vec3<float>& b);

// Matrix3x3 实现
template<typename T>
Matrix3x3<T> Matrix3x3<T>::operator*(const Matrix3x3<T>& other) const {
    return Matrix3x3<T>(
        m00 * other.m00 + m01 * other.m10 + m02 * other.m20,
        m00 * other.m01 + m01 * other.m11 + m02 * other.m21,
        m00 * other.m02 + m01 * other.m12 + m02 * other.m22,
        
        m10 * other.m00 + m11 * other.m10 + m12 * other.m20,
        m10 * other.m01 + m11 * other.m11 + m12 * other.m21,
        m10 * other.m02 + m11 * other.m12 + m12 * other.m22,
        
        m20 * other.m00 + m21 * other.m10 + m22 * other.m20,
        m20 * other.m01 + m21 * other.m11 + m22 * other.m21,
        m20 * other.m02 + m21 * other.m12 + m22 * other.m22
    );
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::identity() {
    return Matrix3x3<T>();
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::translation(T x, T y) {
    Matrix3x3<T> result;
    result.m02 = x;
    result.m12 = y;
    return result;
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::scaling(T x, T y) {
    Matrix3x3<T> result;
    result.m00 = x;
    result.m11 = y;
    return result;
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::rotation(T angle) {
    T c = std::cos(angle);
    T s = std::sin(angle);

    Matrix3x3<T> result;
    result.m00 = c;
    result.m01 = -s;
    result.m10 = s;
    result.m11 = c;
    return result;
}

template<typename T>
T Matrix3x3<T>::determinant() const {
    return m00 * (m11 * m22 - m12 * m21) -
           m01 * (m10 * m22 - m12 * m20) +
           m02 * (m10 * m21 - m11 * m20);
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::inverse() const {
    T det = determinant();
    if (std::abs(det) < static_cast<T>(1e-6))
        return Matrix3x3<T>::identity(); // 返回单位矩阵，表示无法求逆
        
    T invDet = static_cast<T>(1) / det;
    
    return Matrix3x3<T>(
        (m11 * m22 - m12 * m21) * invDet,
        (m02 * m21 - m01 * m22) * invDet,
        (m01 * m12 - m02 * m11) * invDet,
        
        (m12 * m20 - m10 * m22) * invDet,
        (m00 * m22 - m02 * m20) * invDet,
        (m02 * m10 - m00 * m12) * invDet,
        
        (m10 * m21 - m11 * m20) * invDet,
        (m01 * m20 - m00 * m21) * invDet,
        (m00 * m11 - m01 * m10) * invDet
    );
}

// 显式实例化常用类型
template Matrix3x3<float> Matrix3x3<float>::operator*(const Matrix3x3<float>& other) const;
template Matrix3x3<float> Matrix3x3<float>::identity();
template Matrix3x3<float> Matrix3x3<float>::translation(float x, float y);
template Matrix3x3<float> Matrix3x3<float>::scaling(float x, float y);
template Matrix3x3<float> Matrix3x3<float>::rotation(float angle);
template float Matrix3x3<float>::determinant() const;
template Matrix3x3<float> Matrix3x3<float>::inverse() const;

// Matrix4x4 实现
template<typename T>
Matrix4x4<T> Matrix4x4<T>::operator*(const Matrix4x4<T>& other) const {
    Matrix4x4<T> result;
    
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

template<typename T>
Matrix4x4<T> Matrix4x4<T>::identity() {
    return Matrix4x4<T>();
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::translation(T x, T y, T z) {
    Matrix4x4<T> result;
    result.m03 = x;
    result.m13 = y;
    result.m23 = z;
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::scaling(T x, T y, T z) {
    Matrix4x4<T> result;
    result.m00 = x;
    result.m11 = y;
    result.m22 = z;
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::rotationX(T angle) {
    T c = std::cos(angle);
    T s = std::sin(angle);

    Matrix4x4<T> result;
    result.m11 = c;
    result.m12 = -s;
    result.m21 = s;
    result.m22 = c;
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::rotationY(T angle) {
    T c = std::cos(angle);
    T s = std::sin(angle);

    Matrix4x4<T> result;
    result.m00 = c;
    result.m02 = s;
    result.m20 = -s;
    result.m22 = c;
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::rotationZ(T angle) {
    T c = std::cos(angle);
    T s = std::sin(angle);

    Matrix4x4<T> result;
    result.m00 = c;
    result.m01 = -s;
    result.m10 = s;
    result.m11 = c;
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::perspective(T fovY, T aspect, T zNear, T zFar) {
    T tanHalfFovY = std::tan(fovY / static_cast<T>(2));

    Matrix4x4<T> result;
    result.m00 = static_cast<T>(1) / (aspect * tanHalfFovY);
    result.m11 = static_cast<T>(1) / tanHalfFovY;
    result.m22 = -(zFar + zNear) / (zFar - zNear);
    result.m23 = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    result.m32 = -static_cast<T>(1);
    result.m33 = static_cast<T>(0);
    return result;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::lookAt(const Vec3<T>& eye, const Vec3<T>& target, const Vec3<T>& up) {
    // 计算相机坐标系
    Vec3<T> zaxis = (eye - target).normalize();
    Vec3<T> xaxis = up.cross(zaxis).normalize();
    Vec3<T> yaxis = zaxis.cross(xaxis);

    // 构建视图矩阵
    Matrix4x4<T> result;
    result.m00 = xaxis.x;
    result.m01 = xaxis.y;
    result.m02 = xaxis.z;
    result.m03 = -xaxis.dot(eye);
    result.m10 = yaxis.x;
    result.m11 = yaxis.y;
    result.m12 = yaxis.z;
    result.m13 = -yaxis.dot(eye);
    result.m20 = zaxis.x;
    result.m21 = zaxis.y;
    result.m22 = zaxis.z;
    result.m23 = -zaxis.dot(eye);
    result.m30 = static_cast<T>(0);
    result.m31 = static_cast<T>(0);
    result.m32 = static_cast<T>(0);
    result.m33 = static_cast<T>(1);

    return result;
}

// 显式实例化常用类型
template Matrix4x4<float> Matrix4x4<float>::operator*(const Matrix4x4<float>& other) const;
template Matrix4x4<float> Matrix4x4<float>::identity();
template Matrix4x4<float> Matrix4x4<float>::translation(float x, float y, float z);
template Matrix4x4<float> Matrix4x4<float>::scaling(float x, float y, float z);
template Matrix4x4<float> Matrix4x4<float>::rotationX(float angle);
template Matrix4x4<float> Matrix4x4<float>::rotationY(float angle);
template Matrix4x4<float> Matrix4x4<float>::rotationZ(float angle);
template Matrix4x4<float> Matrix4x4<float>::perspective(float fovY, float aspect, float zNear, float zFar);
template Matrix4x4<float> Matrix4x4<float>::lookAt(const Vec3<float>& eye, const Vec3<float>& target, const Vec3<float>& up);

// 通用矩阵模板实现
template<typename T, size_t ROWS, size_t COLS>
template<size_t OTHER_COLS>
Matrix<T, ROWS, OTHER_COLS> Matrix<T, ROWS, COLS>::operator*(const Matrix<T, COLS, OTHER_COLS>& other) const {
    Matrix<T, ROWS, OTHER_COLS> result;
    for (size_t row = 0; row < ROWS; ++row) {
        for (size_t col = 0; col < OTHER_COLS; ++col) {
            result(row, col) = static_cast<T>(0);
            for (size_t k = 0; k < COLS; ++k) {
                result(row, col) += (*this)(row, k) * other(k, col);
            }
        }
    }
    return result;
}

template<typename T, size_t ROWS, size_t COLS>
Matrix<T, ROWS, COLS> Matrix<T, ROWS, COLS>::identity() {
    return Matrix<T, ROWS, COLS>();
}

// 矩阵-向量变换函数
Vec3f transform(const Matrix4x4f& matrix, const Vec3f& vector, float w) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    float wOut = vector.x * matrix.m30 + vector.y * matrix.m31 + vector.z * matrix.m32 + w * matrix.m33;
    
    // 透视除法
    if (std::abs(wOut) > 1e-6f) {
        float invW = 1.0f / wOut;
        return Vec3f(x * invW, y * invW, z * invW);
    }
    
    return Vec3f(x, y, z);
}

Vec3f transformNoDiv(const Matrix4x4f& matrix, const Vec3f& vector, float w) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    
    return Vec3f(x, y, z);
}

Vec3f transformNormal(const Matrix4x4f& modelMatrix, const Vec3f& normal) {
    // 简化实现: 假设模型矩阵只有旋转和均匀缩放，可以直接使用模型矩阵
    Vec3f result;
    result.x = normal.x * modelMatrix.m00 + normal.y * modelMatrix.m01 + normal.z * modelMatrix.m02;
    result.y = normal.x * modelMatrix.m10 + normal.y * modelMatrix.m11 + normal.z * modelMatrix.m12;
    result.z = normal.x * modelMatrix.m20 + normal.y * modelMatrix.m21 + normal.z * modelMatrix.m22;
    
    return normalize(result);
}

// Shader Common - smoothstep 实现
float smoothstep(float edge0, float edge1, float x) {
    // Clamp x to the range [0, 1]
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    
    return t * t * (3.0f - 2.0f * t);
}

// 模板函数 smoothstep 的向量特化实现
template<>
Vec2f smoothstep(Vec2f edge0, Vec2f edge1, Vec2f x) {
    Vec2f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    return result;
}

template<>
Vec3f smoothstep(Vec3f edge0, Vec3f edge1, Vec3f x) {
    Vec3f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    result.z = smoothstep(edge0.z, edge1.z, x.z);
    return result;
}

template<>
Vec4f smoothstep(Vec4f edge0, Vec4f edge1, Vec4f x) {
    Vec4f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    result.z = smoothstep(edge0.z, edge1.z, x.z);
    result.w = smoothstep(edge0.w, edge1.w, x.w);
    return result;
}