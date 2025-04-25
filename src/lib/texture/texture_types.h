// texture_types.h - 纹理系统基础类型定义
#pragma once

#include <memory>
#include <vector>
#include "common.h"

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

// 文件格式枚举
enum class TextureFileFormat
{
    TGA, // TGA文件格式
    // PPM, // PPM/PGM文件格式 //移除了PPM/PGM格式的支持
    AUTO // 自动检测（根据文件扩展名）
};

// 纹理访问模式
enum class TextureAccess
{
    READ_ONLY, // 只读模式
    READ_WRITE // 读写模式
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

    // 移动构造和移动赋值
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

// 纹理格式信息结构体
struct TextureFormatInfo
{
    uint8_t channels;      // 通道数
    uint8_t bytesPerPixel; // 每像素字节数
};