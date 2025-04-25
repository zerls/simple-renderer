// texture.cpp
// 纹理类和文件加载的实现 - 优化版本

#include "texture.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "maths.h"
#include <cstring>
#include <limits>
#include <filesystem>

// 预计算常量，提高性能
namespace {
    constexpr float INV_255 = 1.0f / 255.0f;
    constexpr float INV_65535 = 1.0f / 65535.0f;
}

// 在纹理构造函数中初始化
Texture::Texture() : IResource(ResourceType::TEXTURE), textureType(TextureType::COLOR) {
    // 创建一个空的基本mipmap级别
    mipLevels.push_back(MipmapLevel());
}

// 错误检查和采样辅助函数
inline float4 Texture::getErrorColor() const {
    return float4(1.0f, 0.0f, 1.0f, 1.0f); // 返回粉色表示错误
}

// 根据过滤模式选择合适的采样方法
inline float4 Texture::sampleWithFilter(float u, float v, float mipLevel, const SamplerState& samplerState) const {
    switch (samplerState.getFilter()) {
        case TextureFilter::POINT:
            return samplePoint(u, v, static_cast<int>(mipLevel), samplerState);
        case TextureFilter::LINEAR:
            return sampleBilinear(u, v, static_cast<int>(mipLevel), samplerState);
        case TextureFilter::TRILINEAR:
            return sampleTrilinear(u, v, mipLevel, samplerState);
        default:
            return sampleBilinear(u, v, static_cast<int>(mipLevel), samplerState);
    }
}

// 采样纹理
float4 Texture::sample(float2 uv, const SamplerState& samplerState) const {
    if (mipLevels.empty()) {
        return getErrorColor();
    }
    
    // 对于三线性过滤，我们需要估计mipmap级别
    float mipLevel = 0.0f;
    if (samplerState.getFilter() == TextureFilter::TRILINEAR && 
        samplerState.getMipmapsEnabled() && mipLevels.size() > 1) {
        mipLevel = std::clamp(samplerState.getMipLodBias(), 
                             samplerState.getMinLod(), 
                             std::min(samplerState.getMaxLod(), static_cast<float>(mipLevels.size() - 1)));
    }
    
    return sampleWithFilter(uv.x, uv.y, mipLevel, samplerState);
}

// 带有显式mipmap级别的采样
float4 Texture::sampleLevel(float2 uv, float level, const SamplerState& samplerState) const {
    if (mipLevels.empty()) {
        return getErrorColor();
    }
    
    // 确保级别在有效范围内
    float mipLevel = 0.0f;
    if (samplerState.getMipmapsEnabled() && mipLevels.size() > 1) {
        mipLevel = std::clamp(level + samplerState.getMipLodBias(), 
                             samplerState.getMinLod(), 
                             std::min(samplerState.getMaxLod(), static_cast<float>(mipLevels.size() - 1)));
    }
    
    return sampleWithFilter(uv.x, uv.y, mipLevel, samplerState);
}

// 带有导数的采样（自动计算mipmap级别）
float4 Texture::sampleGrad(float2 uv, float2 ddx, float2 ddy, const SamplerState& samplerState) const {
    if (mipLevels.empty()) {
        return getErrorColor();
    }
    
    // 计算合适的mipmap级别
    float mipLevel = 0.0f;
    if (samplerState.getMipmapsEnabled() && mipLevels.size() > 1) {
        mipLevel = calculateMipLevel(ddx, ddy, samplerState);
    }
    
    return sampleWithFilter(uv.x, uv.y, mipLevel, samplerState);
}

// 阴影贴图专用采样方法
float Texture::sampleDepth(float2 uv, const SamplerState& samplerState) const {
    // 对于深度/阴影图，通常我们只关心第一个通道
    return sample(uv, samplerState).x;
}

// 从特定mipmap级别直接读取像素
float4 Texture::read(int x, int y, int level) const {
    // 检查mipmap级别是否有效
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size()) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    const MipmapLevel& mipLevel = mipLevels[level];
    
    // 检查坐标是否在有效范围内
    if (x < 0 || x >= mipLevel.width || y < 0 || y >= mipLevel.height) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    // 获取像素数据并转换为float4
    size_t index = getPixelIndex(x, y, mipLevel.width);
    return convertToFloat4(&mipLevel.data[index]);
}

// 直接写入像素到特定mipmap级别
void Texture::write(int x, int y, const float4& color, int level) {
    // 检查是否可写入
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size() || access != TextureAccess::READ_WRITE) {
        return;
    }
    
    MipmapLevel& mipLevel = mipLevels[level];
    
    // 检查坐标是否在有效范围内
    if (x < 0 || x >= mipLevel.width || y < 0 || y >= mipLevel.height) {
        return;
    }
    
    // 转换float4并写入像素数据
    size_t index = getPixelIndex(x, y, mipLevel.width);
    convertFromFloat4(&mipLevel.data[index], color);
}

// 应用纹理包裹模式
float Texture::applyWrapMode(float coord, TextureWrapMode wrapMode) const {
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

// 获取直接像素索引
size_t Texture::getPixelIndex(int x, int y, int width) const {
    return (y * width + x) * getBytesPerPixelFromFormat(format);
}

// 最近点采样
float4 Texture::samplePoint(float u, float v, int level, const SamplerState& samplerState) const {
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size()) {
        return getErrorColor();
    }
    
    const MipmapLevel& mipLevel = mipLevels[level];
    
    if (mipLevel.data->empty() || mipLevel.width <= 0 || mipLevel.height <= 0) {
        return getErrorColor();
    }
    
    // 应用纹理包裹模式
    u = applyWrapMode(u, samplerState.getWrapU());
    v = applyWrapMode(v, samplerState.getWrapV());
    
    // 计算纹理坐标
    int x = static_cast<int>(u * mipLevel.width);
    int y = static_cast<int>(v * mipLevel.height);
    
    // 限制在有效范围内（由于前面已经应用了包裹模式，这里是保险措施）
    x = std::max(0, std::min(x, mipLevel.width - 1));
    y = std::max(0, std::min(y, mipLevel.height - 1));
    
    // 直接获取像素
    size_t index = getPixelIndex(x, y, mipLevel.width);
    return convertToFloat4(&mipLevel.data[index]);
}

// 双线性插值采样
float4 Texture::sampleBilinear(float u, float v, int level, const SamplerState& samplerState) const {
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size()) {
        return getErrorColor();
    }
    
    const MipmapLevel& mipLevel = mipLevels[level];
    
    if (mipLevel.data->empty() || mipLevel.width <= 0 || mipLevel.height <= 0) {
        return getErrorColor();
    }
    
    // 应用纹理包裹模式
    u = applyWrapMode(u, samplerState.getWrapU());
    v = applyWrapMode(v, samplerState.getWrapV());
    
    // 计算纹理坐标和小数部分
    float xf = u * mipLevel.width - 0.5f;
    float yf = v * mipLevel.height - 0.5f;
    
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
    int width = mipLevel.width;
    int height = mipLevel.height;
    
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
    float4 c00 = read(x0, y0, level);
    float4 c10 = read(x1, y0, level);
    float4 c01 = read(x0, y1, level);
    float4 c11 = read(x1, y1, level);
    
    // 双线性插值
    float4 c0 = c00 * (1.0f - fx) + c10 * fx;
    float4 c1 = c01 * (1.0f - fx) + c11 * fx;
    return c0 * (1.0f - fy) + c1 * fy;
}

// 三线性插值采样
float4 Texture::sampleTrilinear(float u, float v, float exactMipLevel, const SamplerState& samplerState) const {
    if (mipLevels.empty()) {
        return getErrorColor();
    }
    
    // 如果没有mipmap或禁用了mipmap，则使用基本级别的双线性采样
    if (mipLevels.size() <= 1 || !samplerState.getMipmapsEnabled()) {
        return sampleBilinear(u, v, 0, samplerState);
    }
    
    // 确保级别在有效范围内
    float clampedLevel = std::clamp(exactMipLevel,
                                   samplerState.getMinLod(),
                                   std::min(samplerState.getMaxLod(), 
                                          static_cast<float>(mipLevels.size() - 1)));
    
    // 找到两个最接近的mipmap级别
    int level0 = static_cast<int>(std::floor(clampedLevel));
    int level1 = std::min(level0 + 1, static_cast<int>(mipLevels.size() - 1));
    
    // 获取级别之间的混合因子
    float blend = clampedLevel - level0;
    
    // 对两个级别进行双线性采样
    float4 result0 = sampleBilinear(u, v, level0, samplerState);
    
    // 如果两个级别相同，不需要第二次采样
    if (level0 == level1) {
        return result0;
    }
    
    float4 result1 = sampleBilinear(u, v, level1, samplerState);
    
    // 在两个mipmap级别之间进行线性插值
    return result0 * (1.0f - blend) + result1 * blend;
}

// 计算应该使用的mipmap级别
float Texture::calculateMipLevel(float2 ddx, float2 ddy, const SamplerState& samplerState) const {
    if (mipLevels.empty() || mipLevels.size() <= 1 || !samplerState.getMipmapsEnabled()) {
        return 0.0f;
    }
    
    // 根据导数计算基础mipmap级别
    float baseLevel = calculateMipLevelFromDerivatives(ddx, ddy, mipLevels[0].width, mipLevels[0].height);
    
    // 应用LOD偏差
    float biasedLevel = baseLevel + samplerState.getMipLodBias();
    
    // 应用各向异性过滤（简化实现）
    if (samplerState.getAnisotropy() > 1.0f) {
        biasedLevel -= std::log2(samplerState.getAnisotropy());
        biasedLevel = std::max(0.0f, biasedLevel); // 不要让各向异性过滤导致级别小于0
    }
    
    // 限制为有效范围
    return std::clamp(biasedLevel, 
                     samplerState.getMinLod(), 
                     std::min(samplerState.getMaxLod(), static_cast<float>(mipLevels.size() - 1)));
}

// 通过导数自动计算mipmap级别
float Texture::calculateMipLevelFromDerivatives(float2 ddx, float2 ddy, int baseWidth, int baseHeight) const {
    // 计算纹理坐标的梯度大小
    float dxLength = ddx.x * baseWidth * ddx.x * baseWidth + ddx.y * baseHeight * ddx.y * baseHeight;
    float dyLength = ddy.x * baseWidth * ddy.x * baseWidth + ddy.y * baseHeight * ddy.y * baseHeight;
    
    // 选择较大的梯度
    float d = std::max(dxLength, dyLength);
    
    // 计算对应的mipmap级别（log2(d) / 2）
    // 如果梯度为0或非常小，使用级别0
    return d <= 1.0f ? 0.0f : std::log2(d) * 0.5f;
}

// 保存到文件（统一接口）
bool Texture::saveToFile(const std::string& filename, TextureFileFormat format, int level) const {
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size()) {
        std::cerr << "没有有效的纹理数据可保存" << std::endl;
        return false;
    }
    
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (format == TextureFileFormat::AUTO) {
        format = getFormatFromFilename(filename);
    }
    
    const MipmapLevel& mipLevel = mipLevels[level];
    
    // 使用工具类保存文件
    if (!TextureUtils::saveTextureToFile(
        filename, *mipLevel.data, mipLevel.width, mipLevel.height, getChannels(), format)) {
        std::cerr << "保存纹理文件失败: " << filename << std::endl;
        return false;
    }
    
    std::cout << "已保存纹理到文件: " << filename << " (" << mipLevel.width << "x" << mipLevel.height 
              << ", 级别: " << level << ")" << std::endl;
    
    return true;
}

// 保存深度数据到文件
bool Texture::saveDepthToFile(const std::string& filename, TextureFileFormat format, 
                             float minDepth, float maxDepth, bool pseudocolor) const {
    if (mipLevels.empty() || textureType != TextureType::DATA) {
        std::cerr << "没有有效的深度纹理数据可保存" << std::endl;
        return false;
    }
    
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (format == TextureFileFormat::AUTO) {
        format = getFormatFromFilename(filename);
    }
    
    const MipmapLevel& baseLevel = mipLevels[0];
    
    // 提取深度数据
    std::vector<float> depthData;
    depthData.reserve(baseLevel.width * baseLevel.height);
    
    for (int y = 0; y < baseLevel.height; ++y) {
        for (int x = 0; x < baseLevel.width; ++x) {
            float4 pixel = read(x, y, 0);
            depthData.push_back(pixel.x); // 只使用R通道作为深度值
        }
    }
    
    // 使用工具类保存深度数据
    if (!TextureUtils::saveDepthToFile(
        filename, depthData, baseLevel.width, baseLevel.height, minDepth, maxDepth, pseudocolor, format)) {
        std::cerr << "保存深度数据到文件失败: " << filename << std::endl;
        return false;
    }
    
    std::cout << "已保存深度数据到文件: " << filename << " (" << baseLevel.width << "x" << baseLevel.height << ")" << std::endl;
    
    return true;
}

// 将像素数据转换为float4 - 优化版本
template<>
float4 Texture::convertToFloat4(const void* pixelData) const {
    float4 result(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (!pixelData) {
        return result;
    }
    
    // 基于格式进行不同的转换
    switch (format) {
        case TextureFormat::R8_UNORM: {
            const uint8_t* data8 = static_cast<const uint8_t*>(pixelData);
            result.x = static_cast<float>(data8[0]) * INV_255;
            break;
        }
        case TextureFormat::R8_UINT: {
            const uint8_t* data8 = static_cast<const uint8_t*>(pixelData);
            result.x = static_cast<float>(data8[0]);
            break;
        }
        case TextureFormat::R8G8_UNORM: {
            const uint8_t* data8 = static_cast<const uint8_t*>(pixelData);
            result.x = static_cast<float>(data8[0]) * INV_255;
            result.y = static_cast<float>(data8[1]) * INV_255;
            break;
        }
        case TextureFormat::R8G8B8_UNORM: {
            const uint8_t* data8 = static_cast<const uint8_t*>(pixelData);
            result.x = static_cast<float>(data8[0]) * INV_255;
            result.y = static_cast<float>(data8[1]) * INV_255;
            result.z = static_cast<float>(data8[2]) * INV_255;
            break;
        }
        case TextureFormat::R8G8B8A8_UNORM: {
            const uint8_t* data8 = static_cast<const uint8_t*>(pixelData);
            result.x = static_cast<float>(data8[0]) * INV_255;
            result.y = static_cast<float>(data8[1]) * INV_255;
            result.z = static_cast<float>(data8[2]) * INV_255;
            result.w = static_cast<float>(data8[3]) * INV_255;
            break;
        }
        case TextureFormat::R16_UNORM: {
            const uint16_t* data16 = static_cast<const uint16_t*>(pixelData);
            result.x = static_cast<float>(data16[0]) * INV_65535;
            break;
        }
        case TextureFormat::R16_UINT: {
            const uint16_t* data16 = static_cast<const uint16_t*>(pixelData);
            result.x = static_cast<float>(data16[0]);
            break;
        }
        case TextureFormat::R32_FLOAT: {
            const float* dataFloat = static_cast<const float*>(pixelData);
            result.x = dataFloat[0];
            break;
        }
        case TextureFormat::R32G32_FLOAT: {
            const float* dataFloat = static_cast<const float*>(pixelData);
            result.x = dataFloat[0];
            result.y = dataFloat[1];
            break;
        }
        case TextureFormat::R32G32B32_FLOAT: {
            const float* dataFloat = static_cast<const float*>(pixelData);
            result.x = dataFloat[0];
            result.y = dataFloat[1];
            result.z = dataFloat[2];
            break;
        }
        case TextureFormat::R32G32B32A32_FLOAT: {
            const float* dataFloat = static_cast<const float*>(pixelData);
            result.x = dataFloat[0];
            result.y = dataFloat[1];
            result.z = dataFloat[2];
            result.w = dataFloat[3];
            break;
        }
    }
    
    return result;
}

// 将float4转换为当前格式的数据 - 优化版本
template<>
void Texture::convertFromFloat4(void* pixelData, const float4& color) const {
    if (!pixelData) {
        return;
    }
    
    // 基于格式进行不同的转换
    switch (format) {
        case TextureFormat::R8_UNORM: {
            uint8_t* data8 = static_cast<uint8_t*>(pixelData);
            data8[0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
            break;
        }
        case TextureFormat::R8_UINT: {
            uint8_t* data8 = static_cast<uint8_t*>(pixelData);
            data8[0] = static_cast<uint8_t>(std::clamp(color.x, 0.0f, 255.0f));
            break;
        }
        case TextureFormat::R8G8_UNORM: {
            uint8_t* data8 = static_cast<uint8_t*>(pixelData);
            data8[0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
            data8[1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
            break;
        }
        case TextureFormat::R8G8B8_UNORM: {
            uint8_t* data8 = static_cast<uint8_t*>(pixelData);
            data8[0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
            data8[1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
            data8[2] = static_cast<uint8_t>(std::clamp(color.z * 255.0f, 0.0f, 255.0f));
            break;
        }
        case TextureFormat::R8G8B8A8_UNORM: {
            uint8_t* data8 = static_cast<uint8_t*>(pixelData);
            data8[0] = static_cast<uint8_t>(std::clamp(color.x * 255.0f, 0.0f, 255.0f));
            data8[1] = static_cast<uint8_t>(std::clamp(color.y * 255.0f, 0.0f, 255.0f));
            data8[2] = static_cast<uint8_t>(std::clamp(color.z * 255.0f, 0.0f, 255.0f));
            data8[3] = static_cast<uint8_t>(std::clamp(color.w * 255.0f, 0.0f, 255.0f));
            break;
        }
        case TextureFormat::R16_UNORM: {
            uint16_t* data16 = static_cast<uint16_t*>(pixelData);
            data16[0] = static_cast<uint16_t>(std::clamp(color.x * 65535.0f, 0.0f, 65535.0f));
            break;
        }
        case TextureFormat::R16_UINT: {
            uint16_t* data16 = static_cast<uint16_t*>(pixelData);
            data16[0] = static_cast<uint16_t>(std::clamp(color.x, 0.0f, 65535.0f));
            break;
        }
        case TextureFormat::R32_FLOAT: {
            float* dataFloat = static_cast<float*>(pixelData);
            dataFloat[0] = color.x;
            break;
        }
        case TextureFormat::R32G32_FLOAT: {
            float* dataFloat = static_cast<float*>(pixelData);
            dataFloat[0] = color.x;
            dataFloat[1] = color.y;
            break;
        }
        case TextureFormat::R32G32B32_FLOAT: {
            float* dataFloat = static_cast<float*>(pixelData);
            dataFloat[0] = color.x;
            dataFloat[1] = color.y;
            dataFloat[2] = color.z;
            break;
        }
        case TextureFormat::R32G32B32A32_FLOAT: {
            float* dataFloat = static_cast<float*>(pixelData);
            dataFloat[0] = color.x;
            dataFloat[1] = color.y;
            dataFloat[2] = color.z;
            dataFloat[3] = color.w;
            break;
        }
    }
}

// 创建空白纹理
bool Texture::create(int width, int height, TextureFormat format, TextureAccess access, bool generateMipmaps) {
    if (width <= 0 || height <= 0) {
        std::cerr << "无效的纹理尺寸: " << width << "x" << height << std::endl;
        return false;
    }
    
    // 清空现有数据
    mipLevels.clear();
    
    // 设置纹理属性
    this->format = format;
    this->access = access;
    
    // 创建基本级别
    MipmapLevel baseLevel;
    baseLevel.width = width;
    baseLevel.height = height;
    
    // 分配内存
    int bytesPerPixel = getBytesPerPixelFromFormat(format);
    baseLevel.data->resize(width * height * bytesPerPixel, 0);
    
    // 添加到mipmap链
    mipLevels.push_back(baseLevel);
    
    // 如果需要，生成mipmap
    if (generateMipmaps && (width > 1 || height > 1)) {
        this->generateMipmaps();
    }
    
    std::cout << "已创建空白纹理: " << width << "x" << height 
              << ", 格式: " << static_cast<int>(format)
              << ", 访问模式: " << static_cast<int>(access)
              << ", Mipmap级别: " << mipLevels.size() << std::endl;
    
    return true;
}

// 获取纹理文件格式对应的文件扩展名
std::string Texture::getFileExtension(TextureFileFormat format) {
    switch (format) {
        case TextureFileFormat::TGA:
            return ".tga";
        case TextureFileFormat::PPM:
            return ".ppm";
        default:
            return ".tga"; // 默认使用TGA
    }
}

// 根据文件扩展名判断文件格式
TextureFileFormat Texture::getFormatFromFilename(const std::string& filename) {
    std::string extension;
    size_t pos = filename.find_last_of('.');
    
    if (pos != std::string::npos) {
        extension = filename.substr(pos);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == ".tga") {
            return TextureFileFormat::TGA;
        } else if (extension == ".ppm" || extension == ".pgm") {
            return TextureFileFormat::PPM;
        }
    }
    
    // 默认使用TGA
    return TextureFileFormat::TGA;
}

// 从文件加载纹理（统一接口）- 优化版本
bool Texture::loadFromFile(const std::string& filename, TextureFileFormat fileformat) {
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (fileformat == TextureFileFormat::AUTO) {
        fileformat = getFormatFromFilename(filename);
    }
    
    // 清空现有数据
    mipLevels.clear();
    mipLevels.push_back(MipmapLevel());
    
    // 加载纹理数据
    std::vector<uint8_t>& fileData = *mipLevels[0].data;
    int fileWidth = 0, fileHeight = 0, fileChannels = 0;
    
    // 使用工具类加载文件
    if (!TextureUtils::loadTextureFromFile(filename, fileData, fileWidth, fileHeight, fileChannels, fileformat)) {
        std::cerr << "无法加载纹理文件: " << filename << std::endl;
        return false;
    }
    
    // 设置纹理属性
    mipLevels[0].width = fileWidth;
    mipLevels[0].height = fileHeight;
    
    // 根据通道数选择合适的格式
    switch (fileChannels) {
        case 1:
            format = TextureFormat::R8_UNORM;
            break;
        case 2:
            format = TextureFormat::R8G8_UNORM;
            break;
        case 3:
            format = TextureFormat::R8G8B8_UNORM;
            break;
        case 4:
            format = TextureFormat::R8G8B8A8_UNORM;
            break;
        default:
            std::cerr << "不支持的通道数: " << fileChannels << std::endl;
            return false;
    }
    
    std::cout << "已加载纹理: " << filename << " (" << fileWidth << "x" << fileHeight 
              << ", " << fileChannels << "通道)" << std::endl;
    
    return true;
}

// 手动生成mipmap链 - 优化版本
bool Texture::generateMipmaps() {
    if (mipLevels.empty() || mipLevels[0].width <= 1 || mipLevels[0].height <= 1) {
        std::cerr << "无法为太小的纹理生成mipmap" << std::endl;
        return false;
    }
    
    // 保留基本级别（level 0），清除所有其他级别
    MipmapLevel baseLevel = mipLevels[0];
    mipLevels.clear();
    mipLevels.push_back(baseLevel);
    
    // 计算可以生成的mipmap级别数量
    int maxDimension = std::max(baseLevel.width, baseLevel.height);
    int maxLevels = static_cast<int>(std::floor(std::log2(maxDimension))) + 1;
    
    // 预先分配足够的空间给所有mipmap级别
    mipLevels.reserve(maxLevels);
    
    // 生成所有mipmap级别
    MipmapLevel currentLevel = baseLevel;
    MipmapLevel nextLevel;
    
    for (int level = 1; level < maxLevels; ++level) {
        if (currentLevel.width <= 1 && currentLevel.height <= 1) {
            break; // 已达到最小尺寸
        }
        
        // 生成下一级mipmap
        if (!generateNextMipLevel(currentLevel, nextLevel)) {
            std::cerr << "生成第" << level << "级mipmap失败" << std::endl;
            break;
        }
        
        // 添加到mipmap链
        mipLevels.push_back(nextLevel);
        currentLevel = nextLevel;
    }
    
    std::cout << "已生成" << mipLevels.size() << "级mipmap链" << std::endl;
    return true;
}

// 生成下一级mipmap - 优化版本
bool Texture::generateNextMipLevel(const MipmapLevel& source, MipmapLevel& destination) {
    // 计算下一级的尺寸 (除以2，但至少为1)
    int nextWidth = std::max(1, source.width / 2);
    int nextHeight = std::max(1, source.height / 2);
    
    int channels = getChannels();
    
    // 创建目标级别
    destination.width = nextWidth;
    destination.height = nextHeight;
    
    // 使用Box过滤方法生成下一级mipmap数据
    destination.data = std::make_shared<std::vector<uint8_t>> (TextureUtils::resizeImageBoxFilter(
        *source.data, source.width, source.height, channels, nextWidth, nextHeight));
    
    return !destination.data->empty();
}

// 加载纹理工厂函数
std::shared_ptr<Texture> loadTexture(const std::string& filename, TextureType type, TextureFileFormat format) {
    auto texture = std::make_shared<Texture>();
    if (!texture->loadFromFile(filename, format)) {
        return nullptr; // 加载失败返回空指针
    }
    return texture;
}

// 创建纹理工厂函数
std::shared_ptr<Texture> createTexture(int width, int height, TextureFormat format, TextureAccess access, bool generateMipmaps) {
    auto texture = std::make_shared<Texture>();
    if (!texture->create(width, height, format, access, generateMipmaps)) {
        return nullptr; // 创建失败返回空指针
    }
    return texture;
}