// 定义着色器接口和基本着色器类型
// 在 shader 编写中，参考 HLSL 的命名方案，VecXf ->floatX，数据结构体为Uniforms，Attributes，Varyings
#ifndef SHADER_H
#define SHADER_H

#include "maths.h"
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
    Material material;      // 材质信息
};

// 顶点着色器输入
struct VertexAttributes
{
    float3 position; // 顶点位置（模型空间）
    float3 normal;   // 顶点法线（模型空间）
    Vec2f texCoord;  // 纹理坐标
    Color color;     // 顶点颜色
};

// 顶点着色器到片元着色器的传递变量
struct Varyings
{
    float3 position; // 插值后的位置（世界空间）
    float3 normal;   // 插值后的法线（世界空间）
    Vec2f texCoord;  // 插值后的纹理坐标
    Color color;     // 插值后的颜色
    float depth;     // 深度值（用于深度测试）
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
    virtual void setUniforms(const ShaderUniforms &uniforms) override;
    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

// 带Phong光照模型的着色器
class PhongShader : public IShader
{
protected:
    ShaderUniforms uniforms;

public:
    virtual void setUniforms(const ShaderUniforms &uniforms) override;
    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

//  自定义着色器示例：卡通渲染着色器
class ToonShader : public IShader
{
protected:
    ShaderUniforms uniforms;
    int levels = 4; // 色阶数量

public:
    virtual void setUniforms(const ShaderUniforms &uniforms) override;
    virtual float3 vertexShader(const VertexAttributes &attributes, Varyings &output) override;
    virtual FragmentOutput fragmentShader(const Varyings &input) override;
};

// 创建着色器的工厂函数
std::shared_ptr<IShader> createBasicShader();
std::shared_ptr<IShader> createPhongShader();
std::shared_ptr<IShader> createToonShader();

#endif