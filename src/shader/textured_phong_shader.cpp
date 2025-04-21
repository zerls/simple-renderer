#include "shader.h"
#include <algorithm>
#include <cmath>

FragmentOutput TexturedPhongShader::fragmentShader(const Varyings &input) {
    FragmentOutput output;

    // 获取纹理颜色
    Color baseColor = Color(255, 255, 255);
    if (diffuseMap) {
        baseColor = diffuseMap->sample(input.texCoord);
    }
    
    // 计算法线
    float3 normal = normalize(input.normal);
    
    // 如果有法线贴图，使用法线贴图计算法线
    if (normalMap) {
        // 从法线贴图中获取切线空间法线
        Color normalColor = normalMap->sample(input.texCoord);
        
        // 将 [0,255] 范围转换为 [-1,1] 范围
        float3 tangentNormal = float3(
            (normalColor.r / 255.0f) * 2.0f - 1.0f,
            (normalColor.g / 255.0f) * 2.0f - 1.0f,
            (normalColor.b / 255.0f) * 2.0f - 1.0f
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
    
    // 计算光照向量
    float3 lightDir = normalize(uniforms.light.position - input.position);
    float3 viewDir = normalize(uniforms.eyePosition - input.position);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float NoV = std::max(dot(normal, viewDir), 0.0f);
    float NoL = std::max(dot(normal, lightDir), 0.0f);
    float NoH = std::max(dot(normal, halfwayDir), 0.0f);

    // 计算环境光分量
    float3 ambient = uniforms.surface.ambient * uniforms.light.color * uniforms.light.ambientIntensity;

    // 计算漫反射分量
    float3 diffuse;
    if (diffuseMap) {
        // 如果有漫反射贴图，使用纹理颜色作为基础色
        diffuse = uniforms.light.color * (NoL * uniforms.light.intensity);
    } else {
        // 否则使用材质的漫反射系数
        diffuse = uniforms.surface.diffuse * uniforms.light.color * (NoL * uniforms.light.intensity);
    }

    // 计算镜面反射分量
    float spec = std::pow(NoH, uniforms.surface.shininess);
    float3 specular = uniforms.surface.specular * uniforms.light.color * spec * uniforms.light.intensity;

    // 合并所有光照分量
    float3 result = ambient + diffuse + specular;

    // 确保结果在 [0,1] 范围内
    result.x = std::min(result.x, 1.0f);
    result.y = std::min(result.y, 1.0f);
    result.z = std::min(result.z, 1.0f);

    // 应用基础颜色
    result = result * float3(baseColor.r / 255.0f, baseColor.g / 255.0f, baseColor.b / 255.0f);
    output.color = Color(
        static_cast<uint8_t>(result.x * 255),
        static_cast<uint8_t>(result.y * 255),
        static_cast<uint8_t>(result.z * 255),
        input.color.a);
    
    return output;
}

std::shared_ptr<IShader> createTexturedPhongShader(
    std::shared_ptr<Texture> diffuseMap,
    std::shared_ptr<Texture> normalMap) {
    
    auto shader = std::make_shared<TexturedPhongShader>();
    if (diffuseMap) {
        shader->setDiffuseMap(diffuseMap);
    }
    if (normalMap) {
        shader->setNormalMap(normalMap);
    }
    return shader;
}