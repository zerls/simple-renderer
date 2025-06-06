// texture_utils.h - 纹理格式读写工具类
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "common.h"
#include "texture_types.h"

// 前向声明
enum class TextureFileFormat;

// 纹理文件格式工具类
namespace texture_io {

    // TGA文件读写工具
    class TgaFormat {
    public:
        // TGA文件头结构
        struct TgaHeader {
            uint8_t idLength = 0;         // ID信息长度
            uint8_t colorMapType = 0;     // 色彩映射表类型
            uint8_t imageType = 2;        // 图像类型 (2=未压缩RGB, 3=未压缩灰度)
            uint16_t colorMapStart = 0;   // 色彩映射起始索引
            uint16_t colorMapLength = 0;  // 色彩映射长度
            uint8_t colorMapDepth = 0;    // 色彩映射深度
            uint16_t xOrigin = 0;         // X起点
            uint16_t yOrigin = 0;         // Y起点
            uint16_t width = 0;           // 宽度
            uint16_t height = 0;          // 高度
            uint8_t bitsPerPixel = 24;    // 像素位数
            uint8_t imageDescriptor = 0;  // 图像描述符 (bit 5: 垂直翻转)
        };

        // 从TGA文件加载数据
        static bool loadFromFile(const std::string& filename, 
                                std::vector<uint8_t>& data, 
                                int& width, 
                                int& height, 
                                int& channels);

        // 保存RGB/RGBA数据到TGA文件
        static bool saveToFile(const std::string& filename, 
                              const std::vector<uint8_t>& rgbData, 
                              int width, 
                              int height, 
                              int channels);

        // 保存灰度数据到TGA文件
        static bool saveGrayscaleToFile(const std::string& filename, 
                                       const std::vector<uint8_t>& grayData, 
                                       int width, 
                                       int height);

        // 保存深度数据到TGA文件 (灰度或)
        static bool saveDepthToFile(const std::string& filename,
                                   const std::vector<float>& depthData,
                                   int width,
                                   int height,
                                   float minDepth = 0.0f,
                                   float maxDepth = 1.0f);

    private:
        // 写入TGA文件头
        static void writeHeader(std::ofstream& file, int width, int height, int channels);
        
        // 写入TGA文件尾
        static void writeFooter(std::ofstream& file);
    };

    // 定义函数指针类型
    using LoadTextureFunc = bool (*)(const std::string&, std::vector<uint8_t>&, int&, int&, int&);
    using SaveTextureFunc = bool (*)(const std::string&, const std::vector<uint8_t>&, int, int, int);
    using SaveDepthFunc = bool (*)(const std::string&, const std::vector<float>&, int, int, float, float);

    // 格式处理器结构
    struct FormatHandler {
        LoadTextureFunc loadFunc;
        SaveTextureFunc saveFunc;
        SaveDepthFunc saveDepthFunc;
    };

    // 获取指定格式的处理器
    const FormatHandler& getFormatHandler(TextureFileFormat format);

    // 统一的文件加载接口
    bool loadTextureFromFile(const std::string& filename, 
                            std::vector<uint8_t>& data, 
                            int& width, 
                            int& height, 
                            int& channels,
                            TextureFileFormat format=TextureFileFormat::TGA);

    // 统一的文件保存接口
    bool saveTextureToFile(const std::string& filename, 
                          const std::vector<uint8_t>& pixelData, 
                          int width, 
                          int height, 
                          int channels,
                          TextureFileFormat format=TextureFileFormat::TGA);

    // 统一的深度数据保存接口
    bool saveDepthToFile(const std::string& filename,
                        const std::vector<float>& depthData,
                        int width,
                        int height,
                        float minDepth = 0.0f,
                        float maxDepth = 1.0f,
                        TextureFileFormat format=TextureFileFormat::TGA);

    // 工具函数 - 转换深度数据为灰度
    std::vector<uint8_t> convertDepthToGrayscale(
        const std::vector<float>& depthData,
        int width, 
        int height,
        float minDepth = 0.0f,
        float maxDepth = 1.0f,
        bool flipVertically = true);
        
    // 工具函数 - Box过滤缩小图像
    std::vector<uint8_t> resizeImageBoxFilter(
        const std::vector<uint8_t>& srcData,
        int srcWidth,
        int srcHeight,
        int channels,
        int destWidth,
        int destHeight);
};
