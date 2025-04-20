#include "shader.h"

// 在相应的 cpp 文件中实现
FragmentOutput TexturedPhongShader::fragmentShader(const Varyings &input) {
    FragmentOutput output;

    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(uniforms.light.position - input.position);
    float3 viewDir = normalize(uniforms.eyePosition - input.position);
    float3 halfwayDir = normalize(lightDir + viewDir);

    float NoV = dot(normal, viewDir);
    float NoL = dot(normal, lightDir);
    float NoH = dot(normal, halfwayDir);


        // 获取纹理颜色
    Color baseColor = Color(255, 255, 255);
    if (diffuseMap) {
        baseColor = diffuseMap->sample(input.texCoord);
        // std::cout << "Color: " << baseColor.r <<  " " << baseColor.g <<" " << baseColor.b << std::endl;
    }
    // std::cout << diffuseMap->getWidth() << baseColor.r <<  " " << baseColor.g <<" " << baseColor.b << std::endl;
    // 计算环境光分量
    float3 ambient = uniforms.surface.ambient * uniforms.light.color * uniforms.light.ambientIntensity;

    // 计算漫反射分量
    float diff = std::max(NoL, 0.0f);
    float3 diffuse;
    if (diffuseMap)
    {
        //float3(baseColor.r / 255,baseColor.g / 255,baseColor.b  / 255)  *
       diffuse =  uniforms.light.color * (diff * uniforms.light.intensity);
    }else{
        diffuse = uniforms.surface.diffuse * uniforms.light.color * (diff * uniforms.light.intensity);
    }
    
   

    // 计算镜面反射分量
    float spec = std::pow(std::max(NoH, 0.0f), uniforms.surface.shininess);
    float3 specular = uniforms.surface.specular * uniforms.light.color * spec * uniforms.light.intensity;

    // 合并所有光照分量
    float3 result =ambient + diffuse +specular;

    // 确保结果在 [0,1] 范围内
    result.x = std::min(result.x, 1.0f);
    result.y = std::min(result.y, 1.0f);
    result.z = std::min(result.z, 1.0f);

    result =result *float3(baseColor.r ,baseColor.g ,baseColor.b );
    output.color = Color(
        static_cast<uint8_t>( result.x ),
        static_cast<uint8_t>( result.y),
        static_cast<uint8_t>( result.z),
        input.color.a);
    return output;
//TODO 将 Color与float3 合并，定义 half3
    
    // // 获取法线（如果有法线贴图，则使用法线贴图）
    // float3 normal = input.normal;
    // if (normalMap) {
    //     // 从法线贴图中获取切线空间法线
    //     Color normalColor = normalMap->sample(input.texCoord.x, input.texCoord.y);
        
    //     // 将 [0,255] 范围转换为 [-1,1] 范围
    //     float3 tangentNormal = float3(
    //         (normalColor.r / 255.0f) * 2.0f - 1.0f,
    //         (normalColor.g / 255.0f) * 2.0f - 1.0f,
    //         (normalColor.b / 255.0f) * 2.0f - 1.0f
    //     );
        
    //     // 计算切线和副切线（简化版）
    //     float3 tangent = normalize(cross(input.normal, float3(0, 1, 0)));
    //     float3 bitangent = normalize(cross(input.normal, tangent));
        
    //     // 将切线空间法线转换为世界空间
    //     normal = normalize(
    //         tangent * tangentNormal.x +
    //         bitangent * tangentNormal.y +
    //         input.normal * tangentNormal.z
    //     );
    // }

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