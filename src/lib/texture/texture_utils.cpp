// texture_utils.cpp - 纹理工具函数实现
#include "texture_utils.h"
#include "texture.h"
#include "texture_io.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>

namespace TextureUtils {

// 获取纹理文件格式对应的文件扩展名
std::string getFileExtension(TextureFileFormat format) {
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
TextureFileFormat getFormatFromFilename(const std::string& filename) {
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

// 将像素数据转换为float4
float4 convertToFloat4(const void* pixelData, TextureFormat format) {
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

// 将float4转换为当前格式的数据
void convertFromFloat4(void* pixelData, const float4& color, TextureFormat format) {
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

// 生成下一级mipmap
bool generateNextMipLevel(const MipmapLevel& source, MipmapLevel& destination, TextureFormat format) {
    // 计算下一级的尺寸 (除以2，但至少为1)
    int nextWidth = std::max(1, source.width / 2);
    int nextHeight = std::max(1, source.height / 2);
    
    int channels = getChannelsFromFormat(format);
    
    // 创建目标级别
    destination.width = nextWidth;
    destination.height = nextHeight;
    
    // 使用Box过滤方法生成下一级mipmap数据
    destination.data = std::make_shared<std::vector<uint8_t>>(TextureIO::resizeImageBoxFilter(
        *source.data, source.width, source.height, channels, nextWidth, nextHeight));
    
    return !destination.data->empty();
}

// 获取格式的通道数
int getChannelsFromFormat(TextureFormat format) {
    struct TextureFormatInfo {
        uint8_t channels;      // 通道数
        uint8_t bytesPerPixel; // 每像素字节数
    };

    // 格式信息表
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

    const int index = static_cast<int>(format);
    return (index >= 0 && index < sizeof(formatInfoTable) / sizeof(formatInfoTable[0]))
               ? formatInfoTable[index].channels
               : 4; // 默认值
}

// 获取格式的每像素字节数
int getBytesPerPixelFromFormat(TextureFormat format) {
    struct TextureFormatInfo {
        uint8_t channels;      // 通道数
        uint8_t bytesPerPixel; // 每像素字节数
    };

    // 格式信息表
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

    const int index = static_cast<int>(format);
    return (index >= 0 && index < sizeof(formatInfoTable) / sizeof(formatInfoTable[0]))
               ? formatInfoTable[index].bytesPerPixel
               : 4; // 默认值
}

} // namespace TextureUtils