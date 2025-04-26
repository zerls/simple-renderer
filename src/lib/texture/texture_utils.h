// texture_utils.h - 纹理工具函数
#pragma once

#include <string>
#include <vector>
#include "texture_types.h"

namespace texture_utils {
    // 文件格式相关函数
    std::string getFileExtension(TextureFileFormat format);
    TextureFileFormat getFormatFromFilename(const std::string& filename);
    
    // 数据转换函数
    float4 convertToFloat4(const void* pixelData, TextureFormat format);
    void convertFromFloat4(void* pixelData, const float4& color, TextureFormat format);
    
    // Mipmap生成函数
    bool generateNextMipLevel(const MipmapLevel& source, MipmapLevel& destination, TextureFormat format);
    
    // 获取格式信息
    int getChannelsFromFormat(TextureFormat format);
    int getBytesPerPixelFromFormat(TextureFormat format);
    
    // 图像处理工具函数
    std::vector<uint8_t> convertDepthToGrayscale(
        const std::vector<float>& depthData,
        int width, 
        int height,
        float minDepth = 0.0f,
        float maxDepth = 1.0f,
        bool flipVertically = true);
        
    std::vector<uint8_t> resizeImageBoxFilter(
        const std::vector<uint8_t>& srcData,
        int srcWidth,
        int srcHeight,
        int channels,
        int destWidth,
        int destHeight);
    
    // 辅助常量
    constexpr float INV_255 = 1.0f / 255.0f;
    constexpr float INV_65535 = 1.0f / 65535.0f;
}