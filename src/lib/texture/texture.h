// texture.h - 核心纹理类
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "common.h"
#include "IResource.h"
#include "texture_types.h"
#include "texture_io.h"
#include "texture_sampler.h"
#include "texture_utils.h"

// 纹理类
class Texture : public IResource
{
public:
    Texture();
    ~Texture() = default;

    // 从文件加载纹理
    bool loadFromFile(const std::string &filename, TextureFileFormat format = TextureFileFormat::AUTO);

    // 创建空白纹理
    bool create(int width, int height, TextureFormat format, TextureAccess access = TextureAccess::READ_ONLY, bool generateMipmaps = false);

    // 手动生成mipmap
    bool generateMipmaps();

    // 采样接口 - 委托给TextureSampler
    float4 sample(float2 uv, const SamplerState &samplerState) const;
    float4 sampleLevel(float2 uv, float level, const SamplerState &samplerState) const;
    float4 sampleGrad(float2 uv, float2 ddx, float2 ddy, const SamplerState &samplerState) const;
    float sampleDepth(float2 uv, const SamplerState &samplerState) const;

    // 直接像素访问
    float4 read(int x, int y, int level = 0) const;
    void write(int x, int y, const float4 &color, int level = 0);

    // 数据访问接口
    template <typename T>
    T *getData(int level = 0);

    template <typename T>
    const T *getData(int level = 0) const;

    // Getters
    int getWidth(int level = 0) const;
    int getHeight(int level = 0) const;
    int getChannels() const;
    int getBytesPerPixel() const;
    TextureFormat getFormat() const { return format; }
    TextureType getType() const { return textureType; }
    TextureAccess getAccess() const { return access; }
    int getMipmapCount() const { return static_cast<int>(mipLevels.size()); }
    bool hasMipmaps() const { return mipLevels.size() > 1; }

    // 保存到文件
    bool saveToFile(const std::string &filename, int level = 0, TextureFileFormat format = TextureFileFormat::TGA) const;
    bool saveDepthToFile(const std::string &filename,
                         float minDepth = 0.0f, float maxDepth = 1.0f, TextureFileFormat format = TextureFileFormat::TGA) const;

    // 允许TextureSampler访问私有成员
    friend class TextureSampler;

private:
    TextureFormat format = TextureFormat::R8G8B8A8_UNORM; // 默认格式
    TextureType textureType = TextureType::COLOR;         // 默认为颜色纹理
    TextureAccess access = TextureAccess::READ_ONLY;      // 默认为只读
    std::vector<MipmapLevel> mipLevels;                   // Mipmap级别数据

    // 获取直接像素索引
    size_t getPixelIndex(int x, int y, int width) const;
};

// 工厂函数
std::shared_ptr<Texture> loadTexture(const std::string &filename, TextureType type = TextureType::COLOR, TextureFileFormat format = TextureFileFormat::AUTO);
std::shared_ptr<Texture> createTexture(int width, int height, TextureFormat format, TextureAccess access = TextureAccess::READ_ONLY, bool generateMipmaps = false);