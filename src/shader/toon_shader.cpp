#include "shader.h"

void ToonShader::setUniforms(const ShaderUniforms &uniforms)
{
    this->uniforms = uniforms;
}

float4 ToonShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
{
    // 将顶点变换到世界空间（用于片元着色器）
    output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

    // 传递法线和纹理坐标
    output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
    output.texCoord = attributes.texCoord;
    output.color = attributes.color;

    // 变换到裁剪空间，然后到NDC空间
    float4 clipPos = uniforms.mvpMatrix * float4( attributes.position,1.0f);

    // 将z保存用于深度测试
    output.depth = clipPos.z/clipPos.w;

    return clipPos;
}

FragmentOutput ToonShader::fragmentShader(const Varyings &input)
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

    float edgeThreshold = 0.02f;
    // 如果法线与视线方向几乎垂直，则是边缘
    if (edgeFactor < edgeThreshold)
    {
        // 平滑过渡
        float edgeIntensity = smoothstep(0.0f, edgeThreshold, edgeFactor);
        edge = edgeIntensity;
    }

    // 计算基础颜色（不包括边缘）
    float baseR = diffuse * (input.color.x);
    float baseG = diffuse * (input.color.y);
    float baseB = diffuse * (input.color.z);

    float4 baseColor = input.color * diffuse;
    // 应用边缘因子
    baseColor = baseColor * edge;
    // 计算最终颜色
    output.color = float4(baseColor.xyz(), 1.0f);
    return output;
}

// 创建Toon着色器
std::shared_ptr<IShader> createToonShader()
{
    return std::make_shared<ToonShader>();
}
