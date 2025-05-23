// 对phong_shader.cpp添加阴影相关代码
#include "shader.h"
#include <algorithm>
#include <cmath>

// 实现阴影计算函数
float PhongShader::calculateShadow(const float4& positionLightSpace, const float NoL) const
{
    auto shadowMap = uniforms.textures.find(_ShadowMap)->second;
    if (!uniforms.useShadowMap || !shadowMap) {
        return 1.0f; // 无阴影，完全亮
    }
    
    // 执行透视除法
    float3 projCoords = positionLightSpace.xyz()/positionLightSpace.w;
    
    // 变换到[0,1]范围
    projCoords = float3(
        (projCoords.x + 1.0f) * 0.5f,
        (projCoords.y + 1.0f) * 0.5f,
        (projCoords.z)
    );
    
    // 获取最近深度值
    float closestDepth = shadowMap->sample(projCoords.xy(),SamplerState::LINEAR_CLAMP).x;
    
    // 获取当前片段在光源视角下的深度
    float currentDepth = projCoords.z;
    
    // 方案1: 固定z-offset bias
    // const float constantBias = 0.005f;
    
    // 方案2: 根据法线和光照方向动态调整bias
    float cosAngle = std::max(NoL, 0.0f);
    float dynamicBias = std::max(0.005f * (1.0f - cosAngle), 0.005f);
    
    // 选择使用哪种bias (这里使用动态bias作为示例)
    float bias = dynamicBias;
    
    // 执行深度比较
    return (currentDepth - bias > closestDepth) ? 0.5f : 1.0f;
}


float4 PhongShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
{
    // 变换顶点位置到裁剪空间
    float4 positionClip = uniforms.mvpMatrix * float4(attributes.position,1.0f);
    
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
    output.depth = positionClip.z/positionClip.w;
    
    // 如果启用了阴影映射，计算顶点在光源空间的位置
    if (uniforms.useShadowMap) {
        output.positionLightSpace = uniforms.lightSpaceMatrix * Vec4f(output.position.x, output.position.y, output.position.z, 1.0f);
        // output.positionLightSpace = uniforms.lightSpaceMatrix.transform(Vec4f(output.position,1.0f));
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
    
       // 如果有法线贴图，使用法线贴图计算法线
       auto normalMap = uniforms.textures.find(_NormalMap)->second;
       if (normalMap) {
        // printf("normalMap is not null\n");
        // 从法线贴图中获取切线空间法线
        float3 normalColor = normalMap->sample(input.texCoord, SamplerState::LINEAR_CLAMP).xyz();
        // normalColor =srgbToLinear(normalColor);
        
        // 将 [0,255] 范围转换为 [-1,1] 范围
        float3 tangentNormal = float3(
            (normalColor.x) * 2.0f - 1.0f,
            (normalColor.y ) * 2.0f - 1.0f,
            (normalColor.z) * 2.0f - 1.0f
        );
        
        // 使用切线、副切线和法线构建TBN矩阵
        float3 tangentWS = normalize(float3(input.tangent.x, input.tangent.y, input.tangent.z));
        float3 bitangentWS = normalize(cross(normal, tangentWS) * input.tangent.w);
        
        // TBN矩阵将切线空间法线转换到世界空间
        normal = normalize(
            tangentWS * tangentNormal.x +
            bitangentWS * tangentNormal.y +
            normal * tangentNormal.z
        );
    }

    // float NoV = dot(normal, viewDir);
    float NoL = dot(normal, lightDir);
    float NoH = dot(normal, halfwayDir);

    // 计算环境光分量
    float3 ambient = uniforms.surface.ambient * uniforms.light.color * uniforms.light.ambientIntensity;
    
    // 计算漫反射分量
    // float3 basecolor = uniforms.surface.diffuse;
    float3 basecolor =srgbToLinear( uniforms.surface.diffuse);
    float diff = std::max(NoL, 0.0f);
    auto colorMap = uniforms.textures.find(_ColorMap)->second;
    if (colorMap)
    {
        basecolor = colorMap->sample(input.texCoord, SamplerState::LINEAR_REPEAT).xyz();
        // 如果纹理是 sRGB 格式，转换到线性空间
        basecolor = srgbToLinear(basecolor);
    }
    // float3 basecolor = sampleTexture(_ColorMap,SamplerState::LINEAR_REPEAT, input.texCoord).xyz();
    // 计算漫反射分量
    float3 diffuse;
    diffuse = basecolor * uniforms.light.color * (diff * uniforms.light.intensity);


    // 计算镜面反射分量
    // Blinn-Phong 镜面反射模型
    // 使用半角向量和法线的点积计算反射强度
    float spec = std::pow(std::max(NoH, 0.0f), uniforms.surface.shininess);
    float3 specular = uniforms.surface.specular * uniforms.light.color * spec * uniforms.light.intensity;
    
    // 计算阴影因子
    float shadow = 1.0f;
    if (uniforms.useShadowMap) {
        shadow = calculateShadow(input.positionLightSpace,NoL);
    }
    
    // 合并所有光照分量
    float3 result = ambient + (diffuse + specular) * shadow  ;
    
    // 确保结果在 [0,1] 范围内
    result.x = std::min(result.x, 1.0f);
    result.y = std::min(result.y, 1.0f);
    result.z = std::min(result.z, 1.0f);
    

    result = linearToSrgb(result);
    // 结果转换为输出颜色
    output.color=float4(result,1.0f);
    return output;
}

// 创建Phong着色器
std::shared_ptr<IShader> createPhongShader()
{
    return std::make_shared<PhongShader>();
}