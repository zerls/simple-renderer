#pragma once

#include <vector>
#include "maths.h"

// 定义颜色结构体（修改为与float4互相转换）
struct Color
{
    uint8_t r, g, b, a;

    // 构造函数
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t t) : r(t), g(t), b(t), a(t) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    
    // 从float4构造
    explicit Color(const float4& color) : 
        r(static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255)),
        g(static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255)),
        b(static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255)),
        a(static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255)) {};

    // 转换为float4
    float4 toFloat4() const {
        return float4(
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f);
    }

    // 颜色混合
    Color blend(const Color &other, float factor) const{
        float inv_factor = 1.0f - factor;
        return Color(
            static_cast<uint8_t>(r * inv_factor + other.r * factor),
            static_cast<uint8_t>(g * inv_factor + other.g * factor),
            static_cast<uint8_t>(b * inv_factor + other.b * factor),
            static_cast<uint8_t>(a * inv_factor + other.a * factor));
    }
};

// 修改Vertex使用float4存储颜色
struct Vertex
{
    Vec3f position; // 位置
    Vec3f normal;   // 法线
    Vec4f tangent;  // 切线 (w分量为符号)
    Vec2f texCoord; // 纹理坐标
    float4 color;   // 顶点颜色(改为float4)

    Vertex() = default;
    Vertex(const Vec3f &pos, const float4 &col) : position(pos), color(col) {}
    Vertex(const Vec3f &pos, const Vec3f &norm, const Vec2f &tex, const float4 &col) 
        : position(pos), normal(norm), texCoord(tex), color(col), tangent(0.0f, 0.0f, 0.0f, 1.0f) {}
    Vertex(const Vec3f &pos, const Vec3f &norm, const Vec4f &tan, const Vec2f &tex, const float4 &col) 
        : position(pos), normal(norm), tangent(tan), texCoord(tex), color(col) {}
    
    // 添加便利构造函数，从Color转换
    Vertex(const Vec3f &pos, const Color &col) : position(pos), color(col.toFloat4()) {}
    Vertex(const Vec3f &pos, const Vec3f &norm, const Vec2f &tex, const Color &col) 
        : position(pos), normal(norm), texCoord(tex), color(col.toFloat4()), tangent(0.0f, 0.0f, 0.0f, 1.0f) {}
    Vertex(const Vec3f &pos, const Vec3f &norm, const Vec4f &tan, const Vec2f &tex, const Color &col) 
        : position(pos), normal(norm), tangent(tan), texCoord(tex), color(col.toFloat4()) {}
};

// 光照结构体
struct Light
{
    float3 position;        // 光源位置
    float3 color;           // 光源颜色
    float intensity;        // 光源强度
    float ambientIntensity; // 环境光强度
    
    // 阴影相关参数
    bool castShadow;        // 是否投射阴影
    Matrix4x4f lightViewMatrix;   // 光源视角变换矩阵
    Matrix4x4f lightProjMatrix;   // 光源投影矩阵

    Light() : position(0, 0, 0), color(1, 1, 1), intensity(1.0f), ambientIntensity(0.1f), castShadow(false) {}
    Light(const float3 &pos, const float3 &col, float intens, float ambIntens)
        : position(pos), color(col), intensity(intens), ambientIntensity(ambIntens), castShadow(false) {}
};

// 表面材质属性结构体保持不变
struct Surface
{
    float3 ambient;  // 环境光反射系数
    float3 diffuse;  // 漫反射系数
    float3 specular; // 镜面反射系数
    float shininess; // 光泽度（用于镜面反射计算）

    Surface() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.7f, 0.7f, 0.7f),
                 specular(0.2f, 0.2f, 0.2f), shininess(32.0f) {}
    Surface(const float3 &amb, const float3 &diff, const float3 &spec, float shin)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shin) {}
};

// 三角形结构体（已修改为使用float4颜色）
struct Triangle
{
    std::array<Vertex, 3> vertices;

    Triangle() = default;
    Triangle(const Vertex &v1, const Vertex &v2, const Vertex &v3)
    {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
    }
};

// 定义面的结构保持不变
struct Face {
    std::vector<int> vertexIndices;    // 顶点索引
    std::vector<int> texCoordIndices;  // 纹理坐标索引
    std::vector<int> normalIndices;    // 法线索引
    std::vector<int> tangentIndices;   // 切线索引
    
    Face() = default;
};