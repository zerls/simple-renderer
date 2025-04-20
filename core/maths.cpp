#include "maths.h"

// 矩阵-向量变换函数
Vec3f transform(const Matrix4x4f& matrix, const Vec3f& vector, float w ) {
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


// 添加辅助函数：transformNoDiv 和 transformNormal 的实现
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