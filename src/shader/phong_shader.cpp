// phong_shader.cpp 中的修改部分
// 更新顶点着色器以支持切线变换

#include "shader.h"
#include <algorithm>
#include <cmath>

void PhongShader::setUniforms(const ShaderUniforms &uniforms)
{
    this->uniforms = uniforms;
}

float3 PhongShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
{
    // 变换顶点位置到裁剪空间
    float3 positionClip = transform(uniforms.mvpMatrix, attributes.position);
    
    // 变换顶点位置到世界空间（用于光照计算）
    output.position = transform(uniforms.modelMatrix, attributes.position);
    
    // 变换法线到世界空间并归一化
    // 注意：如果模型矩阵包含非均匀缩放，这里应该使用逆转置矩阵
    output.normal = normalize(transformNormal(uniforms.modelMatrix, attributes.normal));
    
    // 变换切线到世界空间并归一化
    float3 tangentWS = normalize(transformDir(uniforms.modelMatrix, float3(attributes.tangent.x, attributes.tangent.y, attributes.tangent.z)));
    output.tangent = float4(tangentWS.x, tangentWS.y, tangentWS.z, attributes.tangent.w);
    
    // 传递纹理坐标和颜色
    output.texCoord = attributes.texCoord;
    output.color = attributes.color;
    
    // 存储深度用于深度测试
    output.depth = positionClip.z;
    
    return positionClip;
}

FragmentOutput PhongShader::fragmentShader(const Varyings &input)
{
    FragmentOutput output;
    
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(uniforms.light.position - input.position);
    float3 viewDir = normalize(uniforms.eyePosition - input.position);
    float3 halfwayDir = normalize(lightDir + viewDir);
    
    float NoV = dot(normal, viewDir);
    float NoL = dot(normal, lightDir);
    float NoH = dot(normal, halfwayDir);
    
    // 计算环境光分量
    float3 ambient = uniforms.surface.ambient * uniforms.light.color * uniforms.light.ambientIntensity;
    
    // 计算漫反射分量
    float diff = std::max(NoL, 0.0f);
    float3 diffuse = uniforms.surface.diffuse * uniforms.light.color * (diff * uniforms.light.intensity);
    
    // 计算镜面反射分量
    float spec = std::pow(std::max(NoH, 0.0f), uniforms.surface.shininess);
    float3 specular = uniforms.surface.specular * uniforms.light.color * spec * uniforms.light.intensity;
    
    // 合并所有光照分量
    float3 result = ambient + diffuse + specular;
    
    // 确保结果在 [0,1] 范围内
    result.x = std::min(result.x, 1.0f);
    result.y = std::min(result.y, 1.0f);
    result.z = std::min(result.z, 1.0f);
    
    // 应用顶点颜色
    output.color = Color(
        static_cast<uint8_t>(result.x * 255 * (input.color.r / 255.0f)),
        static_cast<uint8_t>(result.y * 255 * (input.color.g / 255.0f)),
        static_cast<uint8_t>(result.z * 255 * (input.color.b / 255.0f)),
        input.color.a);
    
    return output;
}

std::shared_ptr<IShader> createPhongShader() {
    return std::make_shared<PhongShader>();
}