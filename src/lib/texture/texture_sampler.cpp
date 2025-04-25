// texture_sampler.cpp - 纹理采样器实现
#include "texture_sampler.h"
#include "texture.h"
#include <algorithm>
#include <cmath>


// 错误检查和采样辅助函数
inline float4 TextureSampler::getErrorColor() {
    return float4(1.0f, 0.0f, 1.0f, 1.0f); // 返回粉色表示错误
}

// 基本采样函数
float4 TextureSampler::sample(const Texture* texture, float2 uv, const SamplerState& samplerState) {
    if (!texture || texture->getMipmapCount() == 0) {
        return getErrorColor();
    }
    
    // 对于三线性过滤，我们需要估计mipmap级别
    float mipLevel = 0.0f;
    if (samplerState.getFilter() == TextureFilter::TRILINEAR && 
        samplerState.getMipmapsEnabled() && texture->getMipmapCount() > 1) {
        mipLevel = std::clamp(samplerState.getMipLodBias(), 
                             samplerState.getMinLod(), 
                             std::min(samplerState.getMaxLod(), static_cast<float>(texture->getMipmapCount() - 1)));
    }
    
    return sampleWithFilter(texture, uv.x, uv.y, mipLevel, samplerState);
}

// 带有显式mipmap级别的采样
float4 TextureSampler::sampleLevel(const Texture* texture, float2 uv, float level, const SamplerState& samplerState) {
    if (!texture || texture->getMipmapCount() == 0) {
        return getErrorColor();
    }
    
    // 确保级别在有效范围内
    float mipLevel = 0.0f;
    if (samplerState.getMipmapsEnabled() && texture->getMipmapCount() > 1) {
        mipLevel = std::clamp(level + samplerState.getMipLodBias(), 
                             samplerState.getMinLod(), 
                             std::min(samplerState.getMaxLod(), static_cast<float>(texture->getMipmapCount() - 1)));
    }
    
    return sampleWithFilter(texture, uv.x, uv.y, mipLevel, samplerState);
}

// 带有导数的采样（自动计算mipmap级别）
float4 TextureSampler::sampleGrad(const Texture* texture, float2 uv, float2 ddx, float2 ddy, const SamplerState& samplerState) {
    if (!texture || texture->getMipmapCount() == 0) {
        return getErrorColor();
    }
    
    // 计算合适的mipmap级别
    float mipLevel = 0.0f;
    if (samplerState.getMipmapsEnabled() && texture->getMipmapCount() > 1) {
        mipLevel = calculateMipLevel(texture, ddx, ddy, samplerState);
    }
    
    return sampleWithFilter(texture, uv.x, uv.y, mipLevel, samplerState);
}

// 阴影贴图专用采样方法
float TextureSampler::sampleDepth(const Texture* texture, float2 uv, const SamplerState& samplerState) {
    // 对于深度/阴影图，通常我们只关心第一个通道
    return sample(texture, uv, samplerState).x;
}

// 根据过滤模式选择合适的采样方法
float4 TextureSampler::sampleWithFilter(const Texture* texture, float u, float v, float mipLevel, const SamplerState& samplerState) {
    switch (samplerState.getFilter()) {
        case TextureFilter::POINT:
            return samplePoint(texture, u, v, static_cast<int>(mipLevel), samplerState);
        case TextureFilter::LINEAR:
            return sampleBilinear(texture, u, v, static_cast<int>(mipLevel), samplerState);
        case TextureFilter::TRILINEAR:
            return sampleTrilinear(texture, u, v, mipLevel, samplerState);
        default:
            return sampleBilinear(texture, u, v, static_cast<int>(mipLevel), samplerState);
    }
}

// 应用纹理包裹模式
float TextureSampler::applyWrapMode(float coord, TextureWrapMode wrapMode) {
    switch (wrapMode) {
        case TextureWrapMode::CLAMP:
            return std::clamp(coord, 0.0f, 1.0f);
        case TextureWrapMode::REPEAT:
            return coord - std::floor(coord);
        case TextureWrapMode::MIRROR: {
            float temp = coord - std::floor(coord);
            int integer = static_cast<int>(std::floor(coord));
            // 奇数整数部分时反转
            return (integer & 1) ? 1.0f - temp : temp;
        }
        default:
            return coord - std::floor(coord); // 默认为REPEAT
    }
}

// 最近点采样
float4 TextureSampler::samplePoint(const Texture* texture, float u, float v, int level, const SamplerState& samplerState) {
    if (!texture || level < 0 || level >= texture->getMipmapCount()) {
        return getErrorColor();
    }
    
    // 应用纹理包裹模式
    u = applyWrapMode(u, samplerState.getWrapU());
    v = applyWrapMode(v, samplerState.getWrapV());
    
    // 计算纹理坐标
    int width = texture->getWidth(level);
    int height = texture->getHeight(level);
    
    int x = static_cast<int>(u * width);
    int y = static_cast<int>(v * height);
    
    // 限制在有效范围内（由于前面已经应用了包裹模式，这里是保险措施）
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    
    // 直接获取像素
    return texture->read(x, y, level);
}

// 双线性插值采样
float4 TextureSampler::sampleBilinear(const Texture* texture, float u, float v, int level, const SamplerState& samplerState) {
    if (!texture || level < 0 || level >= texture->getMipmapCount()) {
        return getErrorColor();
    }
    
    // 应用纹理包裹模式
    u = applyWrapMode(u, samplerState.getWrapU());
    v = applyWrapMode(v, samplerState.getWrapV());
    
    // 计算纹理坐标和小数部分
    int width = texture->getWidth(level);
    int height = texture->getHeight(level);
    
    float xf = u * width - 0.5f;
    float yf = v * height - 0.5f;
    
    int x = static_cast<int>(std::floor(xf));
    int y = static_cast<int>(std::floor(yf));
    
    float fx = xf - x;
    float fy = yf - y;
    
    // 获取四个角的坐标
    int x0 = x;
    int y0 = y;
    int x1 = x + 1;
    int y1 = y + 1;
    
    // 处理边界
    TextureWrapMode wrapU = samplerState.getWrapU();
    TextureWrapMode wrapV = samplerState.getWrapV();
    
    // 处理X坐标边界
    if (x0 < 0) {
        if (wrapU == TextureWrapMode::CLAMP) {
            x0 = 0;
        } else if (wrapU == TextureWrapMode::REPEAT) {
            x0 = (width + x0 % width) % width;
        } else if (wrapU == TextureWrapMode::MIRROR) {
            int times = (-x0 / width) + ((-x0 % width) != 0 ? 1 : 0);
            x0 = (times & 1) ? -x0 % width : width - (-x0 % width) - 1;
        }
    }
    
    if (x1 >= width) {
        if (wrapU == TextureWrapMode::CLAMP) {
            x1 = width - 1;
        } else if (wrapU == TextureWrapMode::REPEAT) {
            x1 = x1 % width;
        } else if (wrapU == TextureWrapMode::MIRROR) {
            int times = (x1 / width);
            x1 = (times & 1) ? width - (x1 % width) - 1 : x1 % width;
        }
    }
    
    // 处理Y坐标边界
    if (y0 < 0) {
        if (wrapV == TextureWrapMode::CLAMP) {
            y0 = 0;
        } else if (wrapV == TextureWrapMode::REPEAT) {
            y0 = (height + y0 % height) % height;
        } else if (wrapV == TextureWrapMode::MIRROR) {
            int times = (-y0 / height) + ((-y0 % height) != 0 ? 1 : 0);
            y0 = (times & 1) ? -y0 % height : height - (-y0 % height) - 1;
        }
    }
    
    if (y1 >= height) {
        if (wrapV == TextureWrapMode::CLAMP) {
            y1 = height - 1;
        } else if (wrapV == TextureWrapMode::REPEAT) {
            y1 = y1 % height;
        } else if (wrapV == TextureWrapMode::MIRROR) {
            int times = (y1 / height);
            y1 = (times & 1) ? height - (y1 % height) - 1 : y1 % height;
        }
    }
    
    // 获取四个最近的像素
    float4 c00 = texture->read(x0, y0, level);
    float4 c10 = texture->read(x1, y0, level);
    float4 c01 = texture->read(x0, y1, level);
    float4 c11 = texture->read(x1, y1, level);
    
    // 双线性插值
    float4 c0 = c00 * (1.0f - fx) + c10 * fx;
    float4 c1 = c01 * (1.0f - fx) + c11 * fx;
    return c0 * (1.0f - fy) + c1 * fy;
}

// 三线性插值采样
float4 TextureSampler::sampleTrilinear(const Texture* texture, float u, float v, float exactMipLevel, const SamplerState& samplerState) {
    if (!texture || texture->getMipmapCount() <= 1) {
        // 如果没有mipmap，退化为双线性过滤
        return sampleBilinear(texture, u, v, 0, samplerState);
    }
    
    // 计算两个相邻的mipmap级别
    int level0 = static_cast<int>(std::floor(exactMipLevel));
    int level1 = level0 + 1;
    
    // 确保级别在有效范围内
    level0 = std::max(0, std::min(level0, texture->getMipmapCount() - 1));
    level1 = std::max(0, std::min(level1, texture->getMipmapCount() - 1));
    
    // 如果两个级别相同，退化为双线性过滤
    if (level0 == level1) {
        return sampleBilinear(texture, u, v, level0, samplerState);
    }
    
    // 计算mipmap级别之间的插值因子
    float factor = exactMipLevel - level0;
    
    // 对两个mipmap级别分别进行双线性采样
    float4 color0 = sampleBilinear(texture, u, v, level0, samplerState);
    float4 color1 = sampleBilinear(texture, u, v, level1, samplerState);
    
    // 在两个mipmap级别之间进行线性插值
    return color0 * (1.0f - factor) + color1 * factor;
}

// 计算mipmap级别
float TextureSampler::calculateMipLevel(const Texture* texture, float2 ddx, float2 ddy, const SamplerState& samplerState) {
    if (!texture || texture->getMipmapCount() <= 1) {
        return 0.0f;
    }
    
    // 计算基于导数的mipmap级别
    float level = calculateMipLevelFromDerivatives(ddx, ddy, texture->getWidth(), texture->getHeight());
    
    // 应用LOD偏移
    level += samplerState.getMipLodBias();
    
    // 应用各向异性过滤（简化版）
    if (samplerState.getAnisotropy() > 1.0f) {
        level -= std::log2(samplerState.getAnisotropy());
    }
    
    // 限制在有效范围内
    return std::clamp(level, samplerState.getMinLod(), 
                     std::min(samplerState.getMaxLod(), static_cast<float>(texture->getMipmapCount() - 1)));
}

// 通过导数自动计算mipmap级别
float TextureSampler::calculateMipLevelFromDerivatives(float2 ddx, float2 ddy, int baseWidth, int baseHeight) {
    // 计算纹理坐标的导数长度
    float dxLength = std::sqrt(ddx.x * ddx.x + ddx.y * ddx.y) * baseWidth;
    float dyLength = std::sqrt(ddy.x * ddy.x + ddy.y * ddy.y) * baseHeight;
    
    // 取最大的导数长度来确定mipmap级别
    float maxLength = std::max(dxLength, dyLength);
    
    // 计算对应的mipmap级别
    return std::max(0.0f, std::log2(maxLength));
}
