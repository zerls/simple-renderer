// shader.h
// 定义着色器接口和基本着色器类型

#pragma once

// #include "mesh.h"
#include "maths.h"
#include <memory>

// 光照结构体
struct Light
{
    float3 position;         // 光源位置
    float3 color;            // 光源颜色
    float intensity;        // 光源强度
    float ambientIntensity; // 环境光强度

    Light() : position(0, 0, 0), color(1, 1, 1), intensity(1.0f), ambientIntensity(0.1f) {}
    Light(const float3 &pos, const float3 &col, float intens, float ambIntens)
        : position(pos), color(col), intensity(intens), ambientIntensity(ambIntens) {}
};

// 材质结构体
struct Material
{
    float3 ambient;   // 环境光反射系数
    float3 diffuse;   // 漫反射系数
    float3 specular;  // 镜面反射系数
    float shininess; // 光泽度（用于镜面反射计算）

    Material() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.7f, 0.7f, 0.7f),
                 specular(0.2f, 0.2f, 0.2f), shininess(32.0f) {}
    Material(const float3 &amb, const float3 &diff, const float3 &spec, float shin)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shin) {}
};

// 前向声明
class Renderer;

//Shader Common
float smoothstep(float edge0, float edge1, float x);

// 着色器输入/输出结构体
struct ShaderUniforms
{
    Matrix4x4f modelMatrix; // 模型矩阵
    Matrix4x4f viewMatrix;  // 视图矩阵
    Matrix4x4f projMatrix;  // 投影矩阵
    Matrix4x4f mvpMatrix;   // 组合的MVP矩阵
    float3 eyePosition;      // 相机位置（世界空间）
    Light light;            // 光源信息
    Material material;      // 材质信息
};

// 顶点着色器输入
struct VertexAttributes
{
    float3 position; // 顶点位置（模型空间）
    float3 normal;   // 顶点法线（模型空间）
    Vec2f texCoord; // 纹理坐标
    Color color;    // 顶点颜色
};

// 顶点着色器到片元着色器的传递变量
struct Varyings
{
    float3 position; // 插值后的位置（世界空间）
    float3 normal;   // 插值后的法线（世界空间）
    Vec2f texCoord; // 插值后的纹理坐标
    Color color;    // 插值后的颜色
    float depth;    // 深度值（用于深度测试）
};

// 片元着色器输出
struct FragmentOutput
{
    Color color; // 输出颜色
};

// 着色器接口
class IShader
{
public:
    virtual ~IShader() = default;

    // 设置统一变量
    virtual void setUniforms(const ShaderUniforms &uniforms) = 0;

    // 顶点着色器
    // 输入：单个顶点属性
    // 输出：变换后的屏幕空间位置
    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) = 0;

    // 片元着色器
    // 输入：插值后的Varyings
    // 输出：片元颜色
    virtual FragmentOutput fragmentShader(const Varyings &input) = 0;
};

// 基础着色器实现（无光照）
class BasicShader : public IShader
{
protected:
    ShaderUniforms uniforms;

public:
    virtual void setUniforms(const ShaderUniforms &uniforms) override
    {
        this->uniforms = uniforms;
    }

    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        float3 clipPos = transform(uniforms.mvpMatrix, attributes.position);

        // 将z保存用于深度测试
        output.depth = clipPos.z;

        return clipPos;
    }

    virtual FragmentOutput fragmentShader(const Varyings &input) override
    {
        FragmentOutput output;
        output.color = input.color;
        return output;
    }
};

// 带Phong光照模型的着色器
class PhongShader : public IShader
{
protected:
    ShaderUniforms uniforms;

public:
    virtual void setUniforms(const ShaderUniforms &uniforms) override
    {
        this->uniforms = uniforms;
    }

    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        float3 clipPos = transform(uniforms.mvpMatrix, attributes.position);

        // 将z保存用于深度测试
        output.depth = clipPos.z;

        return clipPos;
    }

    virtual FragmentOutput fragmentShader(const Varyings &input) override
    {
        FragmentOutput output;

        // 确保法线是归一化的
        float3 normal = normalize(input.normal);

        // 计算从顶点到光源的方向向量
        float3 lightDir = normalize(uniforms.light.position - input.position);

        // 计算从顶点到观察者的方向向量
        float3 viewDir = normalize(uniforms.eyePosition - input.position);

        // 计算半程向量（用于镜面反射）
        float3 halfwayDir = normalize(lightDir + viewDir);

        float NoV = dot(normal, viewDir);
        float NoL = dot(normal, lightDir);
        float NoH = dot(normal, halfwayDir);

        // 计算环境光分量
        float3 ambient = uniforms.material.ambient * uniforms.light.color * uniforms.light.ambientIntensity;

        // 计算漫反射分量
        float diff = std::max(NoL, 0.0f);
        float3 diffuse = uniforms.material.diffuse * uniforms.light.color * (diff * uniforms.light.intensity);

        // 计算镜面反射分量
        float spec = std::pow(std::max(NoH, 0.0f), uniforms.material.shininess);
        float3 specular = uniforms.material.specular * uniforms.light.color * spec * uniforms.light.intensity;

        // 合并所有光照分量
        float resultR = ambient.x + diffuse.x + specular.x;
        float resultG = ambient.y + diffuse.y + specular.y;
        float resultB = ambient.z + diffuse.z + specular.z;

        // 确保结果在 [0,1] 范围内
        resultR = std::min(resultR, 1.0f);
        resultG = std::min(resultG, 1.0f);
        resultB = std::min(resultB, 1.0f);

        // 结合原始顶点颜色
        float vertexColorFactor = 0.5f; // 可调整原始顶点颜色的影响因子
        float r = resultR * (input.color.r / 255.0f) * vertexColorFactor + resultR * (1.0f - vertexColorFactor);
        float g = resultG * (input.color.g / 255.0f) * vertexColorFactor + resultG * (1.0f - vertexColorFactor);
        float b = resultB * (input.color.b / 255.0f) * vertexColorFactor + resultB * (1.0f - vertexColorFactor);

        // 转换为颜色输出
            output.color = Color(
                static_cast<uint8_t>(r * 255.0f),
                static_cast<uint8_t>(g * 255.0f),
                static_cast<uint8_t>(b * 255.0f),
                input.color.a);
        return output;
    }
};

//ToonShader 存在边缘线计算错误
// 自定义着色器示例：卡通渲染着色器
class ToonShader : public IShader
{
protected:
    ShaderUniforms uniforms;
    int levels = 4; // 色阶数量

public:
    virtual void setUniforms(const ShaderUniforms &uniforms) override
    {
        this->uniforms = uniforms;
    }

    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        float3 clipPos = transform(uniforms.mvpMatrix, attributes.position);

        // 将z保存用于深度测试
        output.depth = clipPos.z;

        return clipPos;
    }

    virtual FragmentOutput fragmentShader(const Varyings &input) override
    {
        FragmentOutput output;

        // 确保法线是归一化的
        float3 normal = normalize(input.normal);

        // 计算从顶点到光源的方向向量
        float3 lightDir = normalize(uniforms.light.position - input.position);

        // 计算从顶点到观察者的方向向量
        float3 viewDir = normalize(uniforms.eyePosition - input.position);

        // 计算漫反射强度
        float diffuse = std::max(dot(normal, lightDir), 0.0f);

        // 将漫反射强度量化为几个离散级别（卡通效果）
        diffuse = std::floor(diffuse * levels) / levels;

        // 边缘检测（轮廓线效果）- 使用法线与视线方向的点积
        float edge = 1.0f;
        float edgeFactor = dot(normal, viewDir);

        float  edgeThreshold=0.02f;
        // 如果法线与视线方向几乎垂直，则是边缘
        if (edgeFactor < edgeThreshold)
        {
            // 平滑过渡
            float edgeIntensity = smoothstep(0.0f, edgeThreshold, edgeFactor);
            edge = edgeIntensity;
        }

        // 计算基础颜色（不包括边缘）
        float baseR = diffuse * (input.color.r / 255.0f);
        float baseG = diffuse * (input.color.g / 255.0f);
        float baseB = diffuse * (input.color.b / 255.0f);

        // 应用边缘因子
        float r = baseR * edge;
        float g = baseG * edge;
        float b = baseB * edge;

        // 转换为颜色输出
        output.color = Color(
            static_cast<uint8_t>(r * 255.0f),
            static_cast<uint8_t>(g * 255.0f),
            static_cast<uint8_t>(b * 255.0f),
            input.color.a);
        return output;
    }
};

// 创建着色器的工厂函数
std::shared_ptr<IShader> createBasicShader();
std::shared_ptr<IShader> createPhongShader();
std::shared_ptr<IShader> createToonShader();