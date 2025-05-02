#include "renderer.h"
#include <algorithm>
#include <cstring> // 为memset添加
#include <thread>
// 定义 MSAA 常量
constexpr int MSAA_SAMPLES = 4;
constexpr float EPSILON = 1e-6f;
const Vec2f MSAA_OFFSETS[MSAA_SAMPLES] = {
    {0.25f, 0.25f}, {0.75f, 0.25f}, 
    {0.25f, 0.75f}, {0.75f, 0.75f}
};

FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height), msaaEnabled(false)
{
    // 直接分配内存
    colorBuffer = new uint8_t[width * height * 4];
    depthBuffer = new float[width * height];
    
    // 初始化缓冲区
    std::memset(colorBuffer, 0, width * height * 4);
    std::fill_n(depthBuffer, width * height, 1.0f);
    
    // MSAA缓冲区初始为nullptr
    msaaDepthBuffer = nullptr;
    msaaSampleCount = nullptr;
}

FrameBuffer::~FrameBuffer() {
    delete[] colorBuffer;
    delete[] depthBuffer;
    
    if (msaaDepthBuffer) {
        delete[] msaaDepthBuffer;
        delete[] msaaSampleCount;
    }
}

void FrameBuffer::enableMSAA(bool enable)
{
    if (msaaEnabled == enable) return;
    
    msaaEnabled = enable;
    
    if (msaaEnabled) {
        // 分配 MSAA 缓冲区
        msaaDepthBuffer = new float[width * height * MSAA_SAMPLES];
        msaaSampleCount = new int[width * height];
        
        // 初始化
        std::fill_n(msaaDepthBuffer, width * height * MSAA_SAMPLES, 1.0f);
        std::memset(msaaSampleCount, 0, width * height * sizeof(int));
    } else {
        // 释放 MSAA 缓冲区内存
        if (msaaDepthBuffer) {
            delete[] msaaDepthBuffer;
            delete[] msaaSampleCount;
            msaaDepthBuffer = nullptr;
            msaaSampleCount = nullptr;
        }
    }
}

int FrameBuffer::calcMSAAIndex(int x, int y, int sampleIndex) const
{
    return (y * width + x) * MSAA_SAMPLES + sampleIndex;
}

void FrameBuffer::setPixel(int x, int y, float depth, const Vec4f &color)
{
    if (!isValidCoord(x, y)) return;
    
    int index = calcIndex(x, y);
    
    // 更新深度缓冲区
    depthBuffer[index] = depth;
    
    // 更新颜色缓冲区
    int colorIndex = index * 4;
    
    // 颜色值限制在[0,1]范围内，然后映射到[0,255]
    colorBuffer[colorIndex]     = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    colorBuffer[colorIndex + 1] = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    colorBuffer[colorIndex + 2] = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    colorBuffer[colorIndex + 3] = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);
}

float FrameBuffer::getDepth(int x, int y) const
{
    if (!isValidCoord(x, y)) return 1.0f;
    return depthBuffer[calcIndex(x, y)];
}

float FrameBuffer::getMSAADepth(int x, int y, int sampleIndex) const
{
    if (!isValidCoord(x, y) || !msaaEnabled) return 1.0f;
    return msaaDepthBuffer[calcMSAAIndex(x, y, sampleIndex)];
}

bool FrameBuffer::depthTest(int x, int y, float depth) const
{
    if (!isValidCoord(x, y)) return false;
    return depth < depthBuffer[calcIndex(x, y)];
}

bool FrameBuffer::msaaDepthTest(int x, int y, int sampleIndex, float depth) const
{
    if (!isValidCoord(x, y) || !msaaEnabled) return false;
    return depth < msaaDepthBuffer[calcMSAAIndex(x, y, sampleIndex)];
}
//TODO 需要深度排序，如果先渲染深度大的物体，后渲染深度小的物体，可能会导致半透明重叠渲染
void FrameBuffer::accumulateMSAAColor(int x, int y, int sampleIndex, float depth, const Vec4f& color)
{
    if (!isValidCoord(x, y) || !msaaEnabled) return;
    
    int pixelIndex = calcIndex(x, y);
    int msaaIndex = calcMSAAIndex(x, y, sampleIndex);
    
    // 更新 MSAA 样本深度
    msaaDepthBuffer[msaaIndex] = depth;
    
    // 累积颜色
    int colorIndex = pixelIndex * 4;
    
    // 限制颜色值
    float r = std::min(std::max(color.x, 0.0f), 1.0f);
    float g = std::min(std::max(color.y, 0.0f), 1.0f);
    float b = std::min(std::max(color.z, 0.0f), 1.0f);
    float a = std::min(std::max(color.w, 0.0f), 1.0f);
    
    // 获取当前颜色
    float current_r = colorBuffer[colorIndex] / 255.0f;
    float current_g = colorBuffer[colorIndex + 1] / 255.0f;
    float current_b = colorBuffer[colorIndex + 2] / 255.0f;
    float current_a = colorBuffer[colorIndex + 3] / 255.0f;
    
    // 增加样本计数
    msaaSampleCount[pixelIndex]++;
    float blendFactor = 1.0f / msaaSampleCount[pixelIndex];
    
    // 进行颜色混合
    float blended_r = current_r * (1.0f - blendFactor) + r * blendFactor;
    float blended_g = current_g * (1.0f - blendFactor) + g * blendFactor;
    float blended_b = current_b * (1.0f - blendFactor) + b * blendFactor;
    float blended_a = current_a * (1.0f - blendFactor) + a * blendFactor;
    
    // 更新颜色缓冲区
    colorBuffer[colorIndex]     = static_cast<uint8_t>(blended_r * 255);
    colorBuffer[colorIndex + 1] = static_cast<uint8_t>(blended_g * 255);
    colorBuffer[colorIndex + 2] = static_cast<uint8_t>(blended_b * 255);
    colorBuffer[colorIndex + 3] = static_cast<uint8_t>(blended_a * 255);
    
    // 更新主深度缓冲区（取最小深度）
    if (depth < depthBuffer[pixelIndex]) {
        depthBuffer[pixelIndex] = depth;
    }
}

void FrameBuffer::clear(const Vec4f &color, float depth)
{
    // 预计算颜色值
    uint8_t r = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    uint8_t g = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    uint8_t b = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    uint8_t a = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);

    // 清除颜色缓冲区
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; ++i) {
        int offset = i * 4;
        colorBuffer[offset]     = r;
        colorBuffer[offset + 1] = g;
        colorBuffer[offset + 2] = b;
        colorBuffer[offset + 3] = a;
    }
    
    // 清除深度缓冲区
    std::fill_n(depthBuffer, totalPixels, depth);
    
    // 清除 MSAA 相关缓冲区（如果启用）
    if (msaaEnabled) {
        std::fill_n(msaaDepthBuffer, totalPixels * MSAA_SAMPLES, depth);
        std::memset(msaaSampleCount, 0, totalPixels * sizeof(int));
    }
}

// 获取帧缓冲区数据
const uint8_t* FrameBuffer::getData() const {
    return colorBuffer;
}

// 将帧缓冲区复制到平台层
void FrameBuffer::copyToPlatform(uint32_t* dst) const
{
    // 使用多线程并行处理
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(numThreads);
    
    // 按块处理以提高缓存命中率
    const int BLOCK_SIZE = 32; // 可以根据实际缓存大小调整
    
    auto processChunk = [&](int startRow, int endRow) {
        for (int blockY = startRow; blockY < endRow; blockY += BLOCK_SIZE) {
            for (int blockX = 0; blockX < width; blockX += BLOCK_SIZE) {
                // 处理当前块
                for (int y = blockY; y < std::min(blockY + BLOCK_SIZE, endRow); ++y) {
                    for (int x = blockX; x < std::min(blockX + BLOCK_SIZE, width); ++x) {
                        int i = y * width + x;
                        int srcIndex = i * 4; // RGBA 格式
                        
                        // 转换为 ARGB 格式 (SDL 使用)
                        dst[i] = 
                            (static_cast<uint32_t>(colorBuffer[srcIndex + 3]) << 24) | // A
                            (static_cast<uint32_t>(colorBuffer[srcIndex + 0]) << 16) | // R
                            (static_cast<uint32_t>(colorBuffer[srcIndex + 1]) << 8) |  // G
                            (static_cast<uint32_t>(colorBuffer[srcIndex + 2]));        // B
                    }
                }
            }
        }
    };
    
    // 划分工作区域
    int rowsPerThread = height / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processChunk, startRow, endRow);
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
}