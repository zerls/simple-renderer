#include "platform.h"
#include "renderer.h"

// 将渲染器的帧缓冲区复制到平台帧缓冲区
void copyFrameBufferToPlatform(const Renderer &renderer)
{
    // 获取平台帧缓冲区
    uint32_t *dst = static_cast<uint32_t *>(platform_get_framebuffer());

    // 直接使用帧缓冲区内置的复制方法
    renderer.getFrameBuffer().copyToPlatform(dst);
}

// 将帧缓冲区保存为PPM格式图像（向后兼容）
void saveToPPM(const std::string &filename, const FrameBuffer &frameBuffer,bool debugMode)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        if (debugMode)
        {
            std::cerr << "无法创建文件：" << filename << std::endl;
        }
        return;
    }

    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();

    // 写入 PPM 文件头
    file << "P6\n"
         << width << " " << height << "\n255\n";

    // 写入像素数据（注意：PPM 只支持 RGB，不支持 alpha 通道）
    const uint8_t *data = frameBuffer.getData();
    for (int i = 0; i < width * height; ++i)
    {
        int index = i * 4;                                           // RGBA 格式
        file.write(reinterpret_cast<const char *>(&data[index]), 3); // 只写入 RGB
    }

    if (debugMode)
    {
        std::cout << "图像已保存到 " << filename << std::endl;
    }
}
