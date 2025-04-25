// texture_utils.cpp - 纹理格式读写工具实现
#include "texture_io.h"
#include "texture.h"
#include <algorithm>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <cmath>

namespace TextureIO
{

    //----------------------------------------
    // TGA文件格式工具实现
    //----------------------------------------

    bool TgaFormat::loadFromFile(const std::string &filename,
                                 std::vector<uint8_t> &data,
                                 int &width,
                                 int &height,
                                 int &channels)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "无法打开TGA文件：" << filename << std::endl;
            return false;
        }

        // TGA文件头信息
        TgaHeader header;

        // 读取TGA文件头
        file.read(reinterpret_cast<char *>(&header.idLength), 1);
        file.read(reinterpret_cast<char *>(&header.colorMapType), 1);
        file.read(reinterpret_cast<char *>(&header.imageType), 1);
        file.read(reinterpret_cast<char *>(&header.colorMapStart), 2);
        file.read(reinterpret_cast<char *>(&header.colorMapLength), 2);
        file.read(reinterpret_cast<char *>(&header.colorMapDepth), 1);
        file.read(reinterpret_cast<char *>(&header.xOrigin), 2);
        file.read(reinterpret_cast<char *>(&header.yOrigin), 2);
        file.read(reinterpret_cast<char *>(&header.width), 2);
        file.read(reinterpret_cast<char *>(&header.height), 2);
        file.read(reinterpret_cast<char *>(&header.bitsPerPixel), 1);
        file.read(reinterpret_cast<char *>(&header.imageDescriptor), 1);

        // 跳过ID和色彩映射数据
        if (header.idLength > 0)
        {
            file.seekg(header.idLength, std::ios::cur);
        }

        if (header.colorMapType > 0 && header.colorMapLength > 0)
        {
            int colorMapSize = header.colorMapLength * (header.colorMapDepth / 8);
            file.seekg(colorMapSize, std::ios::cur);
        }

        // 支持的格式：未压缩的RGB/RGBA或灰度图像
        if (header.imageType != 2 && header.imageType != 3)
        {
            std::cerr << "不支持的TGA图像类型: " << static_cast<int>(header.imageType) << std::endl;
            return false;
        }

        // 设置返回值
        width = header.width;
        height = header.height;
        channels = header.bitsPerPixel / 8;

        // 准备数据数组
        size_t dataSize = width * height * channels;
        data.resize(dataSize);

        // 读取像素数据
        file.read(reinterpret_cast<char *>(data.data()), dataSize);

        // 如果是RGB/RGBA，需要将BGR/BGRA转换为RGB/RGBA
        if (header.imageType == 2 && channels >= 3)
        {
            for (size_t i = 0; i < dataSize; i += channels)
            {
                std::swap(data[i], data[i + 2]); // 交换B和R
            }
        }

        // 检查是否需要垂直翻转 (bit 5 of imageDescriptor控制垂直翻转)
        bool flipVertically = !(header.imageDescriptor & 0x20);
        if (flipVertically)
        {
            // 垂直翻转图像
            std::vector<uint8_t> flippedData(dataSize);
            int rowSize = width * channels;

            for (int y = 0; y < height; y++)
            {
                int srcRow = height - 1 - y;
                std::memcpy(flippedData.data() + y * rowSize,
                            data.data() + srcRow * rowSize,
                            rowSize);
            }

            data = std::move(flippedData);
        }

        std::cout << "已加载TGA纹理: " << filename << " (" << width << "x" << height
                  << ", " << static_cast<int>(header.bitsPerPixel) << "位每像素)" << std::endl;

        return true;
    }

    bool TgaFormat::saveToFile(const std::string &filename,
                               const std::vector<uint8_t> &rgbData,
                               int width,
                               int height,
                               int channels)
    {
        if (rgbData.empty() || width <= 0 || height <= 0)
        {
            std::cerr << "无效的数据或尺寸" << std::endl;
            return false;
        }

        if (channels != 1 && channels != 3 && channels != 4)
        {
            std::cerr << "TGA格式要求1, 3或4通道，当前通道数：" << channels << std::endl;
            return false;
        }

        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "无法创建TGA文件：" << filename << std::endl;
            return false;
        }

        // 写入TGA文件头
        if (channels == 1)
        {
            // 灰度图
            uint8_t header[18] = {0};
            header[2] = 3; // 未压缩的灰度图像
            header[12] = width & 0xFF;
            header[13] = (width >> 8) & 0xFF;
            header[14] = height & 0xFF;
            header[15] = (height >> 8) & 0xFF;
            header[16] = 8;    // 8位颜色深度
            header[17] = 0x20; // 设置垂直方向正确

            file.write(reinterpret_cast<const char *>(header), 18);

            // 直接写入像素数据（TGA灰度图不需要颜色转换）
            file.write(reinterpret_cast<const char *>(rgbData.data()), rgbData.size());
        }
        else
        {
            // RGB/RGBA
            writeHeader(file, width, height, channels);

            // 准备临时缓冲区用于BGR/BGRA数据
            std::vector<uint8_t> tgaData(width * height * channels);

            // 将RGB/RGBA转换为BGR/BGRA
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    int srcIndex = (y * width + x) * channels;
                    int destIndex = (y * width + x) * channels;

                    if (channels >= 3)
                    {
                        tgaData[destIndex] = rgbData[srcIndex + 2];     // B <- R
                        tgaData[destIndex + 1] = rgbData[srcIndex + 1]; // G <- G
                        tgaData[destIndex + 2] = rgbData[srcIndex];     // R <- B

                        if (channels == 4)
                        {
                            tgaData[destIndex + 3] = rgbData[srcIndex + 3]; // A <- A
                        }
                    }
                    else
                    {
                        // 灰度数据直接复制
                        tgaData[destIndex] = rgbData[srcIndex];
                    }
                }
            }

            // 写入像素数据
            file.write(reinterpret_cast<const char *>(tgaData.data()), tgaData.size());
        }

        // 写入TGA尾部
        writeFooter(file);

        std::cout << "成功保存到TGA文件：" << filename << " (" << width << "x" << height
                  << ", " << (channels * 8) << "位每像素)" << std::endl;

        return true;
    }

    bool TgaFormat::saveGrayscaleToFile(const std::string &filename,
                                        const std::vector<uint8_t> &grayData,
                                        int width,
                                        int height)
    {
        if (grayData.empty() || width <= 0 || height <= 0 || grayData.size() < width * height)
        {
            std::cerr << "无效的灰度数据或尺寸" << std::endl;
            return false;
        }

        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "无法创建TGA文件：" << filename << std::endl;
            return false;
        }

        // TGA文件头
        uint8_t header[18] = {0};
        header[2] = 3; // 未压缩的灰度图像
        header[12] = width & 0xFF;
        header[13] = (width >> 8) & 0xFF;
        header[14] = height & 0xFF;
        header[15] = (height >> 8) & 0xFF;
        header[16] = 8;    // 8位颜色深度（灰度）
        header[17] = 0x20; // 设置垂直方向正确

        // 写入文件头
        file.write(reinterpret_cast<const char *>(header), sizeof(header));

        // 写入像素数据（TGA存储方向是从下到上）
        file.write(reinterpret_cast<const char *>(grayData.data()), grayData.size());

        // 写入TGA尾部
        writeFooter(file);

        std::cout << "成功保存灰度数据到TGA文件：" << filename << " (" << width << "x" << height
                  << ", 8位每像素)" << std::endl;

        return true;
    }

    bool TgaFormat::saveDepthToFile(const std::string &filename,
                                    const std::vector<float> &depthData,
                                    int width,
                                    int height,
                                    float minDepth,
                                    float maxDepth)
    {
        // 转换深度数据
        std::vector<uint8_t> imageData;

        // 使用灰度图
        imageData = convertDepthToGrayscale(depthData, width, height, minDepth, maxDepth, false);
        return saveGrayscaleToFile(filename, imageData, width, height);
    }

    //----------------------------------------
    // 统一接口实现
    //----------------------------------------

    // 统一的文件加载接口
    bool load(const std::string &filename,
              std::vector<uint8_t> &data,
              int &width,
              int &height,
              int &channels,
              TextureFileFormat format)
    {
        return TgaFormat::loadFromFile(filename, data, width, height, channels);
    }

    // 统一的文件保存接口
    bool save(const std::string &filename,
              const std::vector<uint8_t> &pixelData,
              int width,
              int height,
              int channels,
              TextureFileFormat format)
    {

        return TgaFormat::saveToFile(filename, pixelData, width, height, channels);
    }

    // 统一的深度数据保存接口
    bool saveDepthToFile(const std::string &filename,
                         const std::vector<float> &depthData,
                         int width,
                         int height,
                         float minDepth,
                         float maxDepth,
                         TextureFileFormat format)
    {
        return TgaFormat::saveDepthToFile(filename, depthData, width, height, minDepth, maxDepth);
    }

    //----------------------------------------
    // 工具函数实现
    //----------------------------------------

    std::vector<uint8_t> convertDepthToGrayscale(
        const std::vector<float> &depthData,
        int width,
        int height,
        float minDepth,
        float maxDepth,
        bool flipVertically)
    {

        if (depthData.empty() || width <= 0 || height <= 0)
        {
            std::cerr << "无效的深度数据或尺寸" << std::endl;
            return std::vector<uint8_t>();
        }

        // 计算深度范围
        float depthRange = maxDepth - minDepth;
        if (depthRange <= 0.0f)
        {
            depthRange = 1.0f;
        }

        // 创建灰度数据
        std::vector<uint8_t> grayData(width * height);

        // 转换深度值为灰度
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int srcIndex = (flipVertically ? (height - 1 - y) : y) * width + x;
                int destIndex = y * width + x;

                // 获取深度值，确保索引在范围内
                float depth = 0.0f;
                if (srcIndex < depthData.size())
                {
                    depth = depthData[srcIndex];
                }

                // 归一化到[0,1]范围
                float normalizedDepth = (depth - minDepth) / depthRange;
                normalizedDepth = std::clamp(normalizedDepth, 0.0f, 1.0f);

                // 转换为灰度值
                grayData[destIndex] = static_cast<uint8_t>(normalizedDepth * 255.0f);
            }
        }

        return grayData;
    }

    // Box过滤缩小图像
    std::vector<uint8_t> resizeImageBoxFilter(
        const std::vector<uint8_t> &srcData,
        int srcWidth,
        int srcHeight,
        int channels,
        int destWidth,
        int destHeight)
    {

        if (srcData.empty() || srcWidth <= 0 || srcHeight <= 0 || channels <= 0 ||
            destWidth <= 0 || destHeight <= 0 || srcData.size() < srcWidth * srcHeight * channels)
        {
            std::cerr << "无效的源数据或尺寸" << std::endl;
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> destData(destWidth * destHeight * channels);

        // 计算缩放比例
        float xRatio = static_cast<float>(srcWidth) / destWidth;
        float yRatio = static_cast<float>(srcHeight) / destHeight;

        // 对于每一个目标像素
        for (int y = 0; y < destHeight; y++)
        {
            for (int x = 0; x < destWidth; x++)
            {
                // 计算源图像中的区域
                int x1 = static_cast<int>(x * xRatio);
                int y1 = static_cast<int>(y * yRatio);
                int x2 = static_cast<int>((x + 1) * xRatio);
                int y2 = static_cast<int>((y + 1) * yRatio);

                // 确保在有效范围内
                x1 = std::min(x1, srcWidth - 1);
                y1 = std::min(y1, srcHeight - 1);
                x2 = std::min(x2, srcWidth);
                y2 = std::min(y2, srcHeight);

                // 计算区域内的像素平均值
                std::vector<float> sum(channels, 0.0f);
                int count = 0;

                for (int sy = y1; sy < y2; sy++)
                {
                    for (int sx = x1; sx < x2; sx++)
                    {
                        int srcIndex = (sy * srcWidth + sx) * channels;

                        for (int c = 0; c < channels; c++)
                        {
                            sum[c] += srcData[srcIndex + c];
                        }

                        count++;
                    }
                }

                // 计算平均值并设置目标像素
                int destIndex = (y * destWidth + x) * channels;

                if (count > 0)
                {
                    for (int c = 0; c < channels; c++)
                    {
                        destData[destIndex + c] = static_cast<uint8_t>(sum[c] / count);
                    }
                }
                else
                {
                    // 如果没有源像素（理论上不应该发生），使用临近点采样
                    int srcIndex = (y1 * srcWidth + x1) * channels;
                    for (int c = 0; c < channels; c++)
                    {
                        destData[destIndex + c] = srcData[srcIndex + c];
                    }
                }
            }
        }

        return destData;
    }

    void TgaFormat::writeHeader(std::ofstream &file, int width, int height, int channels)
    {
        uint8_t header[18] = {0};
        header[2] = 2; // 未压缩的RGB/RGBA
        header[12] = width & 0xFF;
        header[13] = (width >> 8) & 0xFF;
        header[14] = height & 0xFF;
        header[15] = (height >> 8) & 0xFF;
        header[16] = channels * 8;                // 位深度
        header[17] = channels == 4 ? 0x28 : 0x20; // 32位时设置alpha位 + 垂直方向标志

        file.write(reinterpret_cast<const char *>(header), 18);
    }

    void TgaFormat::writeFooter(std::ofstream &file)
    {
        const char footer[] = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
        file.write(footer, 26);
    }

    // Add these implementations to the TextureIO namespace
    bool TextureIO::loadTextureFromFile(const std::string &filename,
                                        std::vector<uint8_t> &data,
                                        int &width,
                                        int &height,
                                        int &channels,
                                        TextureFileFormat format)
    {
        return load(filename, data, width, height, channels, format);
    }

    bool TextureIO::saveTextureToFile(const std::string &filename,
                                      const std::vector<uint8_t> &pixelData,
                                      int width,
                                      int height,
                                      int channels,
                                      TextureFileFormat format)
    {
            return TgaFormat::saveToFile(filename, pixelData, width, height, channels);
    }
}