// 对phong_shader.cpp添加阴影相关代码
#include "shader.h"
#include <algorithm>
#include <cmath>

// 实现阴影计算函数
float PhongShader::calculateShadow(const float4& positionLightSpace) const
{
    if (!uniforms.useShadowMap || !uniforms.shadowMap) {
        return 1.0f; // 无阴影，完全亮
    }
    
    // 执行透视除法
    float3 projCoords = float3(
        positionLightSpace.x / positionLightSpace.w,
        positionLightSpace.y / positionLightSpace.w,
        positionLightSpace.z / positionLightSpace.w
    );
    
    // 变换到[0,1]范围
    projCoords = float3(
        (projCoords.x + 1.0f) * 0.5f,
        (projCoords.y + 1.0f) * 0.5f,
        projCoords.z
    );
    
    // 获取最近深度值
    float closestDepth = uniforms.shadowMap->sample(float2(projCoords.x, projCoords.y)).x;
    
    // 获取当前片段在光源视角下的深度
    float currentDepth = projCoords.z;
    
    // 应用阴影偏移，避免阴影痤疮
    const float bias = 0.005f;
    
    // 执行深度比较
    return (currentDepth - bias > closestDepth) ? 0.5f : 1.0f;
}

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
    output.normal = normalize(transformNormal(uniforms.modelMatrix, attributes.normal));
    
    // 变换切线到世界空间并归一化
    float3 tangentWS = normalize(transformDir(uniforms.modelMatrix, float3(attributes.tangent.x, attributes.tangent.y, attributes.tangent.z)));
    output.tangent = float4(tangentWS.x, tangentWS.y, tangentWS.z, attributes.tangent.w);
    
    // 传递纹理坐标和颜色
    output.texCoord = attributes.texCoord;
    output.color = attributes.color;
    
    // 存储深度用于深度测试
    output.depth = positionClip.z;
    
    // 如果启用了阴影映射，计算顶点在光源空间的位置
    if (uniforms.useShadowMap) {
        // output.positionLightSpace = uniforms.lightSpaceMatrix * Vec4f(output.position.x, output.position.y, output.position.z, 1.0f);
        output.positionLightSpace = uniforms.lightSpaceMatrix.transform(Vec4f(output.position,1.0f));
    }
    
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
    
    // 计算阴影因子
    float shadow = 1.0f;
    if (uniforms.useShadowMap) {
        shadow = calculateShadow(input.positionLightSpace);
    }
    
    // 合并所有光照分量
    float3 result = ambient + (diffuse + specular) * shadow  ;
    
    // 确保结果在 [0,1] 范围内
    result.x = std::min(result.x, 1.0f);
    result.y = std::min(result.y, 1.0f);
    result.z = std::min(result.z, 1.0f);
    
    // 应用顶点颜色
    output.color = float4(
        result.x * input.color.x,
        result.y * input.color.y,
        result.z * input.color.z,
        input.color.w);
    
    return output;
}

// 创建Phong着色器
std::shared_ptr<IShader> createPhongShader()
{
    return std::make_shared<PhongShader>();
}