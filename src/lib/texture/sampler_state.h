// 新文件: sampler_state.h
#pragma once

#include "texture_types.h"

// 采样器状态类
class SamplerState
{
public:
    SamplerState(TextureFilter filter = TextureFilter::LINEAR,
                 TextureWrapMode wrapU = TextureWrapMode::REPEAT,
                 TextureWrapMode wrapV = TextureWrapMode::REPEAT,
                 bool enableMipmaps = true,
                 float mipLodBias = 0.0f,
                 float minLod = 0.0f,
                 float maxLod = 1000.0f,
                 float anisotropy = 1.0f)
        : filter(filter), wrapU(wrapU), wrapV(wrapV), enableMipmaps(enableMipmaps),
          mipLodBias(mipLodBias), minLod(minLod), maxLod(maxLod), anisotropy(anisotropy) {}

    // Getters
    TextureFilter getFilter() const { return filter; }
    TextureWrapMode getWrapU() const { return wrapU; }
    TextureWrapMode getWrapV() const { return wrapV; }
    bool getMipmapsEnabled() const { return enableMipmaps; }
    float getMipLodBias() const { return mipLodBias; }
    float getMinLod() const { return minLod; }
    float getMaxLod() const { return maxLod; }
    float getAnisotropy() const { return anisotropy; }

    // 预定义的常用采样器状态
    static const SamplerState POINT_CLAMP;
    static const SamplerState POINT_REPEAT;
    static const SamplerState POINT_MIRROR;
    static const SamplerState LINEAR_CLAMP;
    static const SamplerState LINEAR_REPEAT;
    static const SamplerState LINEAR_MIRROR;
    static const SamplerState TRILINEAR_CLAMP;
    static const SamplerState TRILINEAR_REPEAT;
    static const SamplerState TRILINEAR_MIRROR;

private:
    TextureFilter filter;
    TextureWrapMode wrapU;
    TextureWrapMode wrapV;
    bool enableMipmaps;
    float mipLodBias;
    float minLod;
    float maxLod;
    float anisotropy;
};

