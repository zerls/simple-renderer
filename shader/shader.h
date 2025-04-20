// shader.h
// 定义着色器接口和基本着色器类型

#pragma once

// #include "mesh.h"
#include "maths.h"
#include <memory>

// 光照结构体
struct Light
{
    Vec3f position;         // 光源位置
    Vec3f color;            // 光源颜色
    float intensity;        // 光源强度
    float ambientIntensity; // 环境光强度

    Light() : position(0, 0, 0), color(1, 1, 1), intensity(1.0f), ambientIntensity(0.1f) {}
    Light(const Vec3f &pos, const Vec3f &col, float intens, float ambIntens)
        : position(pos), color(col), intensity(intens), ambientIntensity(ambIntens) {}
};

// 材质结构体
struct Material
{
    Vec3f ambient;   // 环境光反射系数
    Vec3f diffuse;   // 漫反射系数
    Vec3f specular;  // 镜面反射系数
    float shininess; // 光泽度（用于镜面反射计算）

    Material() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.7f, 0.7f, 0.7f),
                 specular(0.2f, 0.2f, 0.2f), shininess(32.0f) {}
    Material(const Vec3f &amb, const Vec3f &diff, const Vec3f &spec, float shin)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shin) {}
};

// 前向声明
class Renderer;

// 着色器输入/输出结构体
struct ShaderUniforms
{
    Matrix4x4f modelMatrix; // 模型矩阵
    Matrix4x4f viewMatrix;  // 视图矩阵
    Matrix4x4f projMatrix;  // 投影矩阵
    Matrix4x4f mvpMatrix;   // 组合的MVP矩阵
    Vec3f eyePosition;      // 相机位置（世界空间）
    Light light;            // 光源信息
    Material material;      // 材质信息
};

// 顶点着色器输入
struct VertexAttributes
{
    Vec3f position; // 顶点位置（模型空间）
    Vec3f normal;   // 顶点法线（模型空间）
    Vec2f texCoord; // 纹理坐标
    Color color;    // 顶点颜色
};

// 顶点着色器到片元着色器的传递变量
struct Varyings
{
    Vec3f position; // 插值后的位置（世界空间）
    Vec3f normal;   // 插值后的法线（世界空间）
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
    virtual Vec3f vertexShader(const VertexAttributes &attributes, Varyings &output) = 0;

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

    virtual Vec3f vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        Vec3f clipPos = transform(uniforms.mvpMatrix, attributes.position);

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

    virtual Vec3f vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        Vec3f clipPos = transform(uniforms.mvpMatrix, attributes.position);

        // 将z保存用于深度测试
        output.depth = clipPos.z;

        return clipPos;
    }

    virtual FragmentOutput fragmentShader(const Varyings &input) override
    {
        FragmentOutput output;

        // 确保法线是归一化的
        Vec3f normal = normalize(input.normal);

        // 计算从顶点到光源的方向向量
        Vec3f lightDir = normalize(uniforms.light.position - input.position);

        // 计算从顶点到观察者的方向向量
        Vec3f viewDir = normalize(uniforms.eyePosition - input.position);

        // 计算半程向量（用于镜面反射）
        Vec3f halfwayDir = normalize(lightDir + viewDir);

        // 计算环境光分量
        Vec3f ambient = uniforms.material.ambient * uniforms.light.color * uniforms.light.ambientIntensity;

        // 计算漫反射分量
        float diff = std::max(dot(normal, lightDir), 0.0f);
        Vec3f diffuse = uniforms.material.diffuse * uniforms.light.color * (diff * uniforms.light.intensity);

        // 计算镜面反射分量
        float spec = std::pow(std::max(dot(normal, halfwayDir), 0.0f), uniforms.material.shininess);
        Vec3f specular = uniforms.material.specular * uniforms.light.color * spec * uniforms.light.intensity;

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

    virtual Vec3f vertexShader(const VertexAttributes &attributes, Varyings &output) override
    {
        // 将顶点变换到世界空间（用于片元着色器）
        output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

        // 传递法线和纹理坐标
        output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
        output.texCoord = attributes.texCoord;
        output.color = attributes.color;

        // 变换到裁剪空间，然后到NDC空间
        Vec3f clipPos = transform(uniforms.mvpMatrix, attributes.position);

        // 将z保存用于深度测试
        output.depth = clipPos.z;

        return clipPos;
    }

    virtual FragmentOutput fragmentShader(const Varyings &input) override
    {
        FragmentOutput output;

        // 确保法线是归一化的
        Vec3f normal = normalize(input.normal);

        // 计算从顶点到光源的方向向量
        Vec3f lightDir = normalize(uniforms.light.position - input.position);

        // 计算漫反射强度
        float diffuse = std::max(dot(normal, lightDir), 0.0f);

        // 将漫反射强度量化为几个离散级别（卡通效果）
        diffuse = std::floor(diffuse * levels) / levels;

        // 边缘检测（轮廓线效果）
        Vec3f viewDir = normalize(uniforms.eyePosition - input.position);
        float edge = 1.0f;
        if (dot(normal, viewDir) < 0.2f)
        {
            edge = 0.0f; // 轮廓线为黑色
        }

        // 计算最终颜色
        float r = diffuse * (input.color.r / 255.0f) * edge;
        float g = diffuse * (input.color.g / 255.0f) * edge;
        float b = diffuse * (input.color.b / 255.0f) * edge;

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