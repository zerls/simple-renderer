// texture_sampler.h - 纹理采样器实现
#pragma once

#include "sampler_state.h"
#include <memory>

// 前向声明
class Texture;

// 纹理采样器类 - 负责所有纹理采样相关的功能
class TextureSampler {
public:
    TextureSampler() = default;
    ~TextureSampler() = default;

    // 基本采样函数
    static float4 sample(const Texture* texture, float2 uv, const SamplerState& samplerState);
    
    // 带有显式mipmap级别的采样
    static float4 sampleLevel(const Texture* texture, float2 uv, float level, const SamplerState& samplerState);
    
    // 带有导数的采样（用于自动计算mipmap级别）
    static float4 sampleGrad(const Texture* texture, float2 uv, float2 ddx, float2 ddy, const SamplerState& samplerState);
    
    // 阴影贴图专用采样方法
    static float sampleDepth(const Texture* texture, float2 uv, const SamplerState& samplerState);

private:
    // 不同采样模式实现
    static float4 sampleWithFilter(const Texture* texture, float u, float v, float mipLevel, const SamplerState& samplerState);
    static float4 samplePoint(const Texture* texture, float u, float v, int level, const SamplerState& samplerState);
    static float4 sampleBilinear(const Texture* texture, float u, float v, int level, const SamplerState& samplerState);
    static float4 sampleTrilinear(const Texture* texture, float u, float v, float exactMipLevel, const SamplerState& samplerState);

    // 纹理坐标处理函数
    static float applyWrapMode(float coord, TextureWrapMode wrapMode);
    
    // 计算mipmap级别
    static float calculateMipLevel(const Texture* texture, float2 ddx, float2 ddy, const SamplerState& samplerState);
    
    // 通过导数自动计算mipmap级别
    static float calculateMipLevelFromDerivatives(float2 ddx, float2 ddy, int baseWidth, int baseHeight);
    
    // 错误检查和采样辅助函数
    static inline float4 getErrorColor();
};