#include "maths.h"
#include <algorithm> 
// Matrix4x4 实现
template<typename T>
Matrix4x4<T> Matrix4x4<T>::operator*(const Matrix4x4<T>& other) const {
    Matrix4x4<T> result;
    
    // 优化矩阵乘法实现，减少索引计算
    const T* a = m;
    const T* b = other.m;
    T* r = result.m;
    
    // 第一行
    r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
    
    // 第二行
    r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
    
    // 第三行
    r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
    
    // 第四行
    r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
    
    return result;
}

template<typename T>
Matrix4x4<T>  Matrix4x4<T>::transposed() const
{
    Matrix4x4 result;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.m[i * 4 + j] = m[j * 4 + i];
        }
    }
    return result;
}

// 变换向量
template<typename T>
Vec4<T> Matrix4x4<T>::transform(const Vec4<T>& v) const
{
    return Vec4<T>(
        m00 * v.x + m01 * v.y + m02 * v.z + m03 * v.w,
        m10 * v.x + m11 * v.y + m12 * v.z + m13 * v.w,
        m20 * v.x + m21 * v.y + m22 * v.z + m23 * v.w,
        m30 * v.x + m31 * v.y + m32 * v.z + m33 * v.w);
}
template<typename T>
Vec4<T> Matrix4x4<T>::operator*(const Vec4<T>& v) const {
   return transform(v);
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
    angle =  angle/180.0f * M_PI; // 将角度转换为弧度
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
    angle =  angle/180.0f * M_PI; // 将角度转换为弧度
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
    angle =  angle/180.0f * M_PI; // 将角度转换为弧度
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
template Vec4<float> Matrix4x4<float>::transform(const Vec4<float>& vec) const;
template Vec4<float> Matrix4x4<float>::operator*(const Vec4<float>& v) const;

// 矩阵-向量变换函数
Vec3f transform(const Matrix4x4f& matrix, const Vec3f& vector, float w) {
    const float* m = matrix.m;
    float x = vector.x * m[0] + vector.y * m[1] + vector.z * m[2] + w * m[3];
    float y = vector.x * m[4] + vector.y * m[5] + vector.z * m[6] + w * m[7];
    float z = vector.x * m[8] + vector.y * m[9] + vector.z * m[10] + w * m[11];
    float wOut = vector.x * m[12] + vector.y * m[13] + vector.z * m[14] + w * m[15];
    
    // 透视除法
    if (std::abs(wOut) > 1e-6f) {
        float invW = 1.0f / wOut;
        return Vec3f(x * invW, y * invW, z * invW);
    }
    
    return Vec3f(x, y, z);
}


Vec3f transformNoDiv(const Matrix4x4f& matrix, const Vec3f& vector, float w) {
    const float* m = matrix.m;
    return Vec3f(
        vector.x * m[0] + vector.y * m[1] + vector.z * m[2] + w * m[3],
        vector.x * m[4] + vector.y * m[5] + vector.z * m[6] + w * m[7],
        vector.x * m[8] + vector.y * m[9] + vector.z * m[10] + w * m[11]
    );
}

Vec3f transformDir(const Matrix4x4f& mat, const Vec3f& dir) {
    const float* m = mat.m;
    return Vec3f(
        m[0] * dir.x + m[1] * dir.y + m[2] * dir.z,
        m[4] * dir.x + m[5] * dir.y + m[6] * dir.z,
        m[8] * dir.x + m[9] * dir.y + m[10] * dir.z
    );
}

Vec3f transformNormal(const Matrix4x4f& modelMatrix, const Vec3f& normal) {
    // 简化实现: 假设模型矩阵只有旋转和均匀缩放，可以直接使用模型矩阵

    return normalize(transformDir(modelMatrix,normal));
}


// Shader Common - smoothstep 实现
float smoothstep(float edge0, float edge1, float x) {
    // Clamp x to the range [0, 1]
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    
    return t * t * (3.0f - 2.0f * t);
}

// 模板函数 smoothstep 的向量特化实现
Vec2f smoothstep(Vec2f edge0, Vec2f edge1, Vec2f x) {
    Vec2f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    return result;
}

Vec3f smoothstep(Vec3f edge0, Vec3f edge1, Vec3f x) {
    Vec3f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    result.z = smoothstep(edge0.z, edge1.z, x.z);
    return result;
}

Vec4f smoothstep(Vec4f edge0, Vec4f edge1, Vec4f x) {
    Vec4f result;
    result.x = smoothstep(edge0.x, edge1.x, x.x);
    result.y = smoothstep(edge0.y, edge1.y, x.y);
    result.z = smoothstep(edge0.z, edge1.z, x.z);
    result.w = smoothstep(edge0.w, edge1.w, x.w);
    return result;
}