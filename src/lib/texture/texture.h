// texture.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "common.h"
#include "IResource.h"
#include "texture_utils.h"
// #include "texture_format_utils.h"

// 纹理类型枚举 - 简化分类
enum class TextureType
{
    COLOR,  // 颜色纹理（包括漫反射、镜面反射等）
    NORMAL, // 法线纹理
    MASK,   // 遮罩纹理（如透明度、粗糙度）
    DATA    // 数据纹理（深度图、阴影图等）
};

// 纹理格式枚举
enum class TextureFormat
{
    // 8位格式
    R8_UNORM,       // 无符号8位单通道，规范化到[0,1]
    R8_UINT,        // 无符号8位单通道，整数
    R8G8_UNORM,     // 无符号8位双通道，规范化到[0,1]
    R8G8B8_UNORM,   // 无符号8位三通道，规范化到[0,1]
    R8G8B8A8_UNORM, // 无符号8位四通道，规范化到[0,1]

    // 16位格式
    R16_UNORM, // 无符号16位单通道，规范化到[0,1]
    R16_UINT,  // 无符号16位单通道，整数

    // 32位格式
    R32_FLOAT,         // 32位浮点单通道
    R32G32_FLOAT,      // 32位浮点双通道
    R32G32B32_FLOAT,   // 32位浮点三通道
    R32G32B32A32_FLOAT // 32位浮点四通道
};

// 纹理过滤模式
enum class TextureFilter
{
    POINT,    // 最近点采样
    LINEAR,   // 线性过滤
    TRILINEAR // 三线性过滤（包括多级渐进纹理）
};

// 纹理包裹模式
enum class TextureWrapMode
{
    CLAMP,  // 钳制模式（在边界处重复边缘像素）
    REPEAT, // 重复模式（纹理坐标取模）
    MIRROR  // 镜像模式（纹理坐标取模后在0.5-1.0范围内反转）
};

// 文件格式枚举
enum class TextureFileFormat
{
    TGA, // TGA文件格式
    PPM, // PPM/PGM文件格式
    AUTO // 自动检测（根据文件扩展名）
};

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
        : filter(filter), wrapU(wrapU), wrapV(wrapV),
          enableMipmaps(enableMipmaps), mipLodBias(mipLodBias),
          minLod(minLod), maxLod(maxLod), anisotropy(anisotropy) {}

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

// 纹理访问模式
enum class TextureAccess
{
    READ_ONLY, // 只读模式
    READ_WRITE // 读写模式
};

// Mipmap级别数据 - 使用智能指针优化内存管理
struct MipmapLevel
{
    int width;
    int height;
    std::shared_ptr<std::vector<uint8_t>> data;

    MipmapLevel() : width(0), height(0), data(std::make_shared<std::vector<uint8_t>>()) {}

    MipmapLevel(int w, int h)
        : width(w), height(h), data(std::make_shared<std::vector<uint8_t>>(w * h * 4)) {}

    // 拷贝构造函数
    MipmapLevel(const MipmapLevel &other)
        : width(other.width), height(other.height),
          data(std::make_shared<std::vector<uint8_t>>(*other.data)) {}

    // 拷贝赋值运算符
    MipmapLevel &operator=(const MipmapLevel &other)
    {
        if (this != &other)
        {
            width = other.width;
            height = other.height;
            data = std::make_shared<std::vector<uint8_t>>(*other.data);
        }
        return *this;
    }

    // 移动构造和移动赋值同之前
    MipmapLevel(MipmapLevel &&other) noexcept
        : width(other.width), height(other.height), data(std::move(other.data)) {}

    MipmapLevel &operator=(MipmapLevel &&other) noexcept
    {
        if (this != &other)
        {
            width = other.width;
            height = other.height;
            data = std::move(other.data);
        }
        return *this;
    }
};

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

    // 获取纹理颜色（使用UV坐标和采样器状态）
    float4 sample(float2 uv, const SamplerState &samplerState = SamplerState::LINEAR_REPEAT) const;

    // 带有显式mipmap级别的采样
    float4 sampleLevel(float2 uv, float level, const SamplerState &samplerState = SamplerState::LINEAR_REPEAT) const;

    // 带有导数的采样（用于自动计算mipmap级别）
    float4 sampleGrad(float2 uv, float2 ddx, float2 ddy, const SamplerState &samplerState = SamplerState::LINEAR_REPEAT) const;

    // 阴影贴图专用采样方法
    float sampleDepth(float2 uv, const SamplerState &samplerState = SamplerState::LINEAR_CLAMP) const;

    // 从特定mipmap级别读取像素
    float4 read(int x, int y, int level = 0) const;

    // 写入像素到特定mipmap级别
    void write(int x, int y, const float4 &color, int level = 0);

    // 获取特定格式的原始数据指针（用于底层操作）- 适配智能指针结构
    template <typename T>
    T *getData(int level = 0)
    {
        if (level < 0 || level >= mipLevels.size())
            return nullptr;
        return reinterpret_cast<T *>(mipLevels[level].data->data());
    }

    template <typename T>
    const T *getData(int level = 0) const
    {
        if (level < 0 || level >= mipLevels.size())
            return nullptr;
        return reinterpret_cast<const T *>(mipLevels[level].data->data());
    }

    // Getters
    int getWidth(int level = 0) const
    {
        return (level >= 0 && level < mipLevels.size()) ? mipLevels[level].width : 0;
    }

    int getHeight(int level = 0) const
    {
        return (level >= 0 && level < mipLevels.size()) ? mipLevels[level].height : 0;
    }

    int getChannels() const { return getChannelsFromFormat(format); }
    int getBytesPerPixel() const { return getBytesPerPixelFromFormat(format); }
    TextureFormat getFormat() const { return format; }
    TextureType getType() const { return textureType; }
    TextureAccess getAccess() const { return access; }
    int getMipmapCount() const { return static_cast<int>(mipLevels.size()); }
    bool hasMipmaps() const { return mipLevels.size() > 1; }

    // 保存到文件
    bool saveToFile(const std::string &filename, TextureFileFormat format = TextureFileFormat::AUTO, int level = 0) const;

    // 专用于深度/数据纹理的保存方法
    bool saveDepthToFile(const std::string &filename, TextureFileFormat format = TextureFileFormat::AUTO,
                         float minDepth = 0.0f, float maxDepth = 1.0f, bool pseudocolor = false) const;

private:
    struct TextureFormatInfo
    {
        uint8_t channels;      // 通道数
        uint8_t bytesPerPixel; // 每像素字节数
    };

    // 定义为类静态成员
    static constexpr TextureFormatInfo formatInfoTable[] = {
        {1, 1},  // R8_UNORM
        {1, 1},  // R8_UINT
        {2, 2},  // R8G8_UNORM
        {3, 3},  // R8G8B8_UNORM
        {4, 4},  // R8G8B8A8_UNORM
        {1, 2},  // R16_UNORM
        {1, 2},  // R16_UINT
        {1, 4},  // R32_FLOAT
        {2, 8},  // R32G32_FLOAT
        {3, 12}, // R32G32B32_FLOAT
        {4, 16}  // R32G32B32A32_FLOAT
    };
    TextureFormat format = TextureFormat::R8G8B8A8_UNORM; // 默认格式
    TextureType textureType = TextureType::COLOR;         // 默认为颜色纹理
    TextureAccess access = TextureAccess::READ_ONLY;      // 默认为只读
    std::vector<MipmapLevel> mipLevels;                   // Mipmap级别数据

    // 采样函数指针类型定义 - 采用策略模式减少条件判断
    using SampleFuncPtr = float4 (Texture::*)(float, float, int, const SamplerState &) const;
    using SampleMipFuncPtr = float4 (Texture::*)(float, float, float, const SamplerState &) const;

    // 不同采样模式实现
    float4 sampleWithFilter(float u, float v, float mipLevel, const SamplerState &samplerState) const;
    float4 samplePoint(float u, float v, int level, const SamplerState &samplerState) const;
    float4 sampleBilinear(float u, float v, int level, const SamplerState &samplerState) const;
    float4 sampleTrilinear(float u, float v, float exactMipLevel, const SamplerState &samplerState) const;

    // 采样函数映射表 - 缓存策略函数指针，避免每次采样时的条件判断
    static const std::unordered_map<TextureFilter, SampleFuncPtr> sampleFuncs;
    static const SampleMipFuncPtr trilinearSampleFunc;

    // 辅助函数 - 用于减少冗余代码
    inline float4 getErrorColor() const;

    // 纹理坐标处理函数
    float applyWrapMode(float coord, TextureWrapMode wrapMode) const;

    // 获取直接像素索引
    size_t getPixelIndex(int x, int y, int width) const;

    // 计算mipmap级别
    float calculateMipLevel(float2 ddx, float2 ddy, const SamplerState &samplerState) const;

    // 通过导数自动计算mipmap级别
    float calculateMipLevelFromDerivatives(float2 ddx, float2 ddy, int baseWidth, int baseHeight) const;

    static constexpr int getChannelsFromFormat(TextureFormat format)
    {
        const int index = static_cast<int>(format);
        return (index >= 0 && index < sizeof(formatInfoTable) / sizeof(formatInfoTable[0]))
                   ? formatInfoTable[index].channels
                   : 4; // 默认值
    }

    static constexpr int getBytesPerPixelFromFormat(TextureFormat format)
    {
        const int index = static_cast<int>(format);
        return (index >= 0 && index < sizeof(formatInfoTable) / sizeof(formatInfoTable[0]))
                   ? formatInfoTable[index].bytesPerPixel
                   : 4; // 默认值
    }

    // 使用模板函数优化数据转换，减少重复代码
    template <typename T>
    float4 convertToFloat4(const T *pixelData) const
    {
        // 基础模板，具体实现将在特化中提供
        return float4(0, 0, 0, 1); // 默认返回黑色
    }

    // 特化声明 - 具体实现将在cpp文件中定义
    template <>
    float4 convertToFloat4<uint8_t>(const uint8_t *pixelData) const;
    template <>
    float4 convertToFloat4<uint16_t>(const uint16_t *pixelData) const;
    template <>
    float4 convertToFloat4<float>(const float *pixelData) const;

    // 将float4转换为当前格式的数据 - 使用模板优化
    template <typename T>
    void convertFromFloat4(T *pixelData, const float4 &color) const
    {
        // 基础模板，具体实现将在特化中提供
    }

    // 特化声明
    template <>
    void convertFromFloat4<uint8_t>(uint8_t *pixelData, const float4 &color) const;
    template <>
    void convertFromFloat4<uint16_t>(uint16_t *pixelData, const float4 &color) const;
    template <>
    void convertFromFloat4<float>(float *pixelData, const float4 &color) const;

    // 生成下一级mipmap
    bool generateNextMipLevel(const MipmapLevel &source, MipmapLevel &destination);

    // 获取纹理文件格式对应的文件扩展名
    static std::string getFileExtension(TextureFileFormat format);

    // 根据文件扩展名判断文件格式
    static TextureFileFormat getFormatFromFilename(const std::string &filename);
};

// 预定义常用采样器状态
const SamplerState SamplerState::POINT_CLAMP(TextureFilter::POINT, TextureWrapMode::CLAMP, TextureWrapMode::CLAMP);
const SamplerState SamplerState::POINT_REPEAT(TextureFilter::POINT, TextureWrapMode::REPEAT, TextureWrapMode::REPEAT);
const SamplerState SamplerState::POINT_MIRROR(TextureFilter::POINT, TextureWrapMode::MIRROR, TextureWrapMode::MIRROR);
const SamplerState SamplerState::LINEAR_CLAMP(TextureFilter::LINEAR, TextureWrapMode::CLAMP, TextureWrapMode::CLAMP);
const SamplerState SamplerState::LINEAR_REPEAT(TextureFilter::LINEAR, TextureWrapMode::REPEAT, TextureWrapMode::REPEAT);
const SamplerState SamplerState::LINEAR_MIRROR(TextureFilter::LINEAR, TextureWrapMode::MIRROR, TextureWrapMode::MIRROR);
const SamplerState SamplerState::TRILINEAR_CLAMP(TextureFilter::TRILINEAR, TextureWrapMode::CLAMP, TextureWrapMode::CLAMP);
const SamplerState SamplerState::TRILINEAR_REPEAT(TextureFilter::TRILINEAR, TextureWrapMode::REPEAT, TextureWrapMode::REPEAT);
const SamplerState SamplerState::TRILINEAR_MIRROR(TextureFilter::TRILINEAR, TextureWrapMode::MIRROR, TextureWrapMode::MIRROR);

// 加载/创建纹理工厂函数
std::shared_ptr<Texture> loadTexture(const std::string &filename, TextureType type = TextureType::COLOR, TextureFileFormat format = TextureFileFormat::AUTO);
std::shared_ptr<Texture> createTexture(int width, int height, TextureFormat format, TextureAccess access = TextureAccess::READ_ONLY, bool generateMipmaps = false);