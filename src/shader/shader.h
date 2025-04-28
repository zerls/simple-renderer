// 假设这是 shader.h 文件
#pragma once

#include <memory>
#include "maths.h"
#include "texture.h" // 使用新的纹理库
#include "common.h"
#include <memory>

// 着色器输入/输出结构体
struct ShaderUniforms
{
    Matrix4x4f modelMatrix; // 模型矩阵
    Matrix4x4f viewMatrix;  // 视图矩阵
    Matrix4x4f projMatrix;  // 投影矩阵
    Matrix4x4f mvpMatrix;   // 组合的MVP矩阵
    float3 eyePosition;     // 相机位置（世界空间）
    Light light;            // 光源信息
    Surface surface;        // 材质信息
    // 阴影相关
    bool useShadowMap = false;   // 是否使用阴影贴图
    Matrix4x4f lightSpaceMatrix; // 光源空间变换矩阵（视图*投影）

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};

static constexpr const char *_ShadowMap = "shadowmap";
static constexpr const char *_ColorMap = "colormap";
static constexpr const char *_NormalMap = "normalmap";

// 顶点着色器输入
struct VertexAttributes
{
    float3 position; // 顶点位置（模型空间）
    float3 normal;   // 顶点法线（模型空间）
    float4 tangent;  // 顶点切线（模型空间），w分量为符号
    Vec2f texCoord;  // 纹理坐标
    float4 color;    // 顶点颜色(改为float4)
};

// 顶点着色器到片元着色器的传递变量
struct Varyings
{
    float3 position; // 插值后的位置（世界空间）
    float3 normal;   // 插值后的法线（世界空间）
    float4 tangent;  // 插值后的切线（世界空间），w分量为符号
    Vec2f texCoord;  // 插值后的纹理坐标
    float4 color;    // 插值后的颜色(改为float4)
    float depth;     // 深度值（用于深度测试）

    // 阴影映射相关
    float4 positionLightSpace; // 光源空间的位置（用于阴影映射）
};

// 片元着色器输出
struct FragmentOutput
{
    float4 color; // 输出颜色(改为float4)
    bool discard; // 是否丢弃片元

    FragmentOutput() : color(0.0f, 0.0f, 0.0f, 1.0f), discard(false) {}
    explicit FragmentOutput(const float4 &color) : color(color), discard(false) {}
};

// 着色器接口
class IShader : public IResource
{
protected:
    ShaderUniforms uniforms;

public:
    IShader() : IResource(ResourceType::SHADER) {}
    virtual ~IShader() = default;

    // 设置着色器的uniform变量
    virtual void setUniforms(const ShaderUniforms &uniforms)
    {
        this->uniforms = uniforms;
    }

    // 顶点着色器
    // 输入：单个顶点属性
    // 输出：变换后的屏幕空间位置
    virtual float4 vertexShader(const VertexAttributes &attributes, Varyings &output) = 0;

    // 片元着色器
    // 输入：插值后的Varyings
    // 输出：片元颜色
    virtual FragmentOutput fragmentShader(const Varyings &input) = 0;

    // 核心方法：安全地采样纹理
    float4 sampleTexture(const std::string &name, const SamplerState &samplerstate, const float2 &uv) const
    {
        auto it = uniforms.textures.find(name);
        if (it != uniforms.textures.end() && it->second)
        {
            return it->second->sample(uv, samplerstate);
        }
        // 纹理不存在或为空，返回错误颜色（紫色）
        return float4(1.0f, 0.0f, 1.0f, 1.0f);
    }
};

// 基础着色器实现（无光照）
class BasicShader : public IShader
{
public:
    virtual float4 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

// 带Phong光照模型的着色器
class PhongShader : public IShader
{
public:
    virtual float4 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;

protected:
    // 计算阴影因子
    float calculateShadow(const float4 &positionLightSpace,const float NoL) const;
};

// 自定义着色器示例：卡通渲染着色器
class ToonShader : public IShader
{
protected:
    int levels = 4; // 色阶数量

public:
    virtual float4 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

// 阴影贴图生成着色器
class ShadowMapShader : public IShader
{
public:
    virtual float4 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

// 创建着色器的工厂函数
std::shared_ptr<IShader> createBasicShader();
std::shared_ptr<IShader> createPhongShader();
std::shared_ptr<IShader> createToonShader();
// 创建阴影贴图着色器
std::shared_ptr<IShader> createShadowMapShader();
