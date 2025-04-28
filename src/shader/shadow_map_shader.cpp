// 新建文件：shadow_map_shader.cpp
#include "shader.h"
#include <algorithm>
#include <cmath>

float4 ShadowMapShader::vertexShader(const VertexAttributes &attributes, Varyings &output)
{
    // 只需要将顶点变换到光源空间
    float4 positionClip = uniforms.lightSpaceMatrix * uniforms.modelMatrix *float4(attributes.position,1.0f);

    // 保存深度值
    output.depth = positionClip.z/positionClip.w;
    
    return positionClip;
}

FragmentOutput ShadowMapShader::fragmentShader(const Varyings &input)
{
    // 阴影贴图生成着色器只需要输出深度，不需要颜色
    FragmentOutput output;
    output.color = float4(input.depth, input.depth, input.depth, 1.0f);
    return output;
}

std::shared_ptr<IShader> createShadowMapShader() {
    return std::make_shared<ShadowMapShader>();
}