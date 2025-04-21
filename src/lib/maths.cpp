#include "maths.h"

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
Vec4<T> Matrix4x4<T>::transform(const Vec4<T> &v) const
{
    return Vec4<T>(
        m00 * v.x + m01 * v.y + m02 * v.z + m03 * v.w,
        m10 * v.x + m11 * v.y + m12 * v.z + m13 * v.w,
        m20 * v.x + m21 * v.y + m22 * v.z + m23 * v.w,
        m30 * v.x + m31 * v.y + m32 * v.z + m33 * v.w);
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
Vec3f transformDir(const Matrix4x4f& mat, const Vec3f& dir) {
    return Vec3f(
        mat.m00 * dir.x + mat.m01 * dir.y + mat.m02 * dir.z,
        mat.m10 * dir.x + mat.m11 * dir.y + mat.m12 * dir.z,
        mat.m20 * dir.x + mat.m21 * dir.y + mat.m22 * dir.z
    );
}
Vec3f transformNormal(const Matrix4x4f& modelMatrix, const Vec3f& normal) {
    // 简化实现: 假设模型矩阵只有旋转和均匀缩放，可以直接使用模型矩阵

    return normalize(transformDir(modelMatrix,normal));
}

// 透视校正插值函数 (Vec2f/float2 版本)
Vec2f interpolatePerspectiveCorrect(
    const Vec2f &attr0, const Vec2f &attr1, const Vec2f &attr2, // 三个顶点的属性值
    const Vec3f &lambda,                                        // 重心坐标系数
    const Vec3f &w,                                             // 顶点的 1/w 值（透视除法前的倒数）
    float w_correct                                             // 插值后的 1/w 用于校正
)
{
    Vec2f result;
    result.x = (lambda.x * attr0.x * w.x + lambda.y * attr1.x * w.y + lambda.z * attr2.x * w.z) * w_correct;
    result.y = (lambda.x * attr0.y * w.x + lambda.y * attr1.y * w.y + lambda.z * attr2.y * w.z) * w_correct;
    return result;
}

// 透视校正插值函数 (Vec3f/float3 版本)
Vec3f interpolatePerspectiveCorrect(
    const Vec3f &attr0, const Vec3f &attr1, const Vec3f &attr2, // 三个顶点的属性值
    const Vec3f &lambda,                                        // 重心坐标系数
    const Vec3f &w,                                             // 顶点的 1/w 值（透视除法前的倒数）
    float w_correct                                             // 插值后的 1/w 用于校正
)
{
    Vec3f result;
    result.x = (lambda.x * attr0.x * w.x + lambda.y * attr1.x * w.y + lambda.z * attr2.x * w.z) * w_correct;
    result.y = (lambda.x * attr0.y * w.x + lambda.y * attr1.y * w.y + lambda.z * attr2.y * w.z) * w_correct;
    result.z = (lambda.x * attr0.z * w.x + lambda.y * attr1.z * w.y + lambda.z * attr2.z * w.z) * w_correct;
    return result;
}

// 透视校正插值函数 (Vec4f/float4 版本)
Vec4f interpolatePerspectiveCorrect(
    const Vec4f &attr0, const Vec4f &attr1, const Vec4f &attr2, // 三个顶点的属性值
    const Vec3f &lambda,                                        // 重心坐标系数
    const Vec3f &w,                                             // 顶点的 1/w 值（透视除法前的倒数）
    float w_correct                                             // 插值后的 1/w 用于校正
)
{
    Vec4f result;
    result.x = (lambda.x * attr0.x * w.x + lambda.y * attr1.x * w.y + lambda.z * attr2.x * w.z) * w_correct;
    result.y = (lambda.x * attr0.y * w.x + lambda.y * attr1.y * w.y + lambda.z * attr2.y * w.z) * w_correct;
    result.z = (lambda.x * attr0.z * w.x + lambda.y * attr1.z * w.y + lambda.z * attr2.z * w.z) * w_correct;
    result.w = (lambda.x * attr0.w * w.x + lambda.y * attr1.w * w.y + lambda.z * attr2.w * w.z) * w_correct;
    return result;
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