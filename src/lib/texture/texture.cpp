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
#include "texture_utils.h"

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
    const void* pixelData = &mipLevel.data->data()[index];
    return TextureUtils::convertToFloat4(pixelData, format);
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
    void* pixelData = &mipLevel.data->data()[index];
    TextureUtils::convertFromFloat4(pixelData, color, format);
}

// 获取直接像素索引
size_t Texture::getPixelIndex(int x, int y, int width) const {
    return (y * width + x) * TextureUtils::getBytesPerPixelFromFormat(format);
}

// 保存到文件（统一接口）
bool Texture::saveToFile(const std::string& filename, TextureFileFormat format, int level) const {
    if (mipLevels.empty() || level < 0 || level >= mipLevels.size()) {
        std::cerr << "没有有效的纹理数据可保存" << std::endl;
        return false;
    }
    
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (format == TextureFileFormat::AUTO) {
        format = TextureUtils::getFormatFromFilename(filename);
    }
    
    const MipmapLevel& mipLevel = mipLevels[level];
    
    // 使用工具类保存文件
    if (!TextureIO::saveTextureToFile(
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
                             float minDepth, float maxDepth) const {
    if (mipLevels.empty() ) {
        std::cerr << "没有有效的深度纹理数据可保存" << std::endl;
        return false;
    }
    
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (format == TextureFileFormat::AUTO) {
        format = TextureUtils::getFormatFromFilename(filename);
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
    if (!TextureIO::saveDepthToFile(
        filename, depthData, baseLevel.width, baseLevel.height, minDepth, maxDepth, format)) {
        std::cerr << "保存深度数据到文件失败: " << filename << std::endl;
        return false;
    }
    
    std::cout << "已保存深度数据到文件: " << filename << " (" << baseLevel.width << "x" << baseLevel.height << ")" << std::endl;
    
    return true;
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
    int bytesPerPixel = TextureUtils::getBytesPerPixelFromFormat(format);
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


// 从文件加载纹理（统一接口）- 优化版本
bool Texture::loadFromFile(const std::string& filename, TextureFileFormat fileformat) {
    // 如果是自动检测格式，根据文件扩展名确定格式
    if (fileformat == TextureFileFormat::AUTO) {
        fileformat = TextureUtils::getFormatFromFilename(filename);
    }
    
    // 清空现有数据
    mipLevels.clear();
    mipLevels.push_back(MipmapLevel());
    
    // 加载纹理数据
    std::vector<uint8_t>& fileData = *mipLevels[0].data;
    int fileWidth = 0, fileHeight = 0, fileChannels = 0;
    
    // 使用工具类加载文件
    if (!TextureIO::loadTextureFromFile(filename, fileData, fileWidth, fileHeight, fileChannels, fileformat)) {
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
        if (!TextureUtils::generateNextMipLevel(currentLevel, nextLevel, format)) {
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

int Texture::getBytesPerPixel() const {
    return TextureUtils::getBytesPerPixelFromFormat(format);
}

int Texture::getWidth(int level) const {
    if (level >= 0 && level < mipLevels.size()) {
        return mipLevels[level].width;
    }
    return 0;
}

int Texture::getHeight(int level) const {
    if (level >= 0 && level < mipLevels.size()) {
        return mipLevels[level].height;
    }
    return 0;
}

int Texture::getChannels() const {
    return TextureUtils::getChannelsFromFormat(format);
}

float4 Texture::sample(float2 uv, const SamplerState &samplerState) const {
    return TextureSampler::sample(this, uv, samplerState);
}