#include "shader.h"

void BasicShader::setUniforms(const ShaderUniforms &uniforms)
{
    this->uniforms = uniforms;
}

float3 BasicShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
{
    // 将顶点变换到世界空间（用于片元着色器）
    output.position = transformNoDiv(uniforms.modelMatrix, attributes.position);

    // 传递法线和纹理坐标
    output.normal = transformNormal(uniforms.modelMatrix, attributes.normal);
    output.texCoord = attributes.texCoord;
    output.color = attributes.color;

    // 变换到裁剪空间，然后到NDC空间
    float3 clipPos =transform(uniforms.mvpMatrix ,attributes.position);

    // 将z保存用于深度测试
    output.depth = clipPos.z;

    return clipPos;
}

FragmentOutput BasicShader::fragmentShader(const Varyings &input)
{
    FragmentOutput output;
    output.color = input.color;
    return output;
}

// 创建基础着色器
std::shared_ptr<IShader> createBasicShader()
{
    return std::make_shared<BasicShader>();
}