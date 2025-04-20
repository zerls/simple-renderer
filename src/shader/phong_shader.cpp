#include "shader.h"

// 带Phong光照模型的着色器

void PhongShader::setUniforms(const ShaderUniforms &uniforms)
{
    this->uniforms = uniforms;
}

float3 PhongShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
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

FragmentOutput PhongShader::fragmentShader(const Varyings &input)
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
    float3 ambient = uniforms.surface.ambient * uniforms.light.color * uniforms.light.ambientIntensity;

    // 计算漫反射分量
    float diff = std::max(NoL, 0.0f);
    float3 diffuse = uniforms.surface.diffuse * uniforms.light.color * (diff * uniforms.light.intensity);

    // 计算镜面反射分量
    float spec = std::pow(std::max(NoH, 0.0f), uniforms.surface.shininess);
    float3 specular = uniforms.surface.specular * uniforms.light.color * spec * uniforms.light.intensity;

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

// 创建Phong着色器
std::shared_ptr<IShader> createPhongShader()
{
    return std::make_shared<PhongShader>();
}