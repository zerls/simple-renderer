// 优化后的 FrameBuffer 实现
#include "renderer.h"
#include <algorithm>

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
    // 初始化基本缓冲区
    frameData.resize(width * height * 4, 0);
    depthBuffer.resize(width * height, 1.0f);
}

void FrameBuffer::enableMSAA(bool enable)
{
    if (msaaEnabled == enable) return;
    
    msaaEnabled = enable;
    
    if (msaaEnabled) {
        // 分配 MSAA 缓冲区
        msaaDepthBuffer.resize(width * height * MSAA_SAMPLES, 1.0f);
        msaaSampleCount.resize(width * height, 0);
    } else {
        // 释放 MSAA 缓冲区内存
        std::vector<float>().swap(msaaDepthBuffer);
        std::vector<int>().swap(msaaSampleCount);
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
    
    // 更新颜色缓冲区（向量化操作）
    int colorIndex = index * 4;
    auto clamp = [](float v) { return static_cast<uint8_t>(std::min(std::max(v, 0.0f), 1.0f) * 255); };
    
    frameData[colorIndex]     = clamp(color.x);
    frameData[colorIndex + 1] = clamp(color.y);
    frameData[colorIndex + 2] = clamp(color.z);
    frameData[colorIndex + 3] = clamp(color.w);
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

void FrameBuffer::accumulateMSAAColor(int x, int y, int sampleIndex, float depth, const Vec4f& color)
{
    if (!isValidCoord(x, y) || !msaaEnabled) return;
    
    int pixelIndex = calcIndex(x, y);
    int msaaIndex = calcMSAAIndex(x, y, sampleIndex);
    
    // 更新 MSAA 样本深度
    msaaDepthBuffer[msaaIndex] = depth;
    
    // 累积颜色（优化的混合操作）
    int colorIndex = pixelIndex * 4;
    
    // 获取当前像素颜色
    Vec4f currentColor(
        frameData[colorIndex] / 255.0f,
        frameData[colorIndex + 1] / 255.0f,
        frameData[colorIndex + 2] / 255.0f,
        frameData[colorIndex + 3] / 255.0f
    );
    
    // 计算混合系数和新颜色
    msaaSampleCount[pixelIndex]++;
    float blendFactor = 1.0f / msaaSampleCount[pixelIndex];
    Vec4f blendedColor = currentColor * (1.0f - blendFactor) + color * blendFactor;
    
    // 更新帧缓冲区
    auto clamp = [](float v) { return static_cast<uint8_t>(std::min(std::max(v, 0.0f), 1.0f) * 255); };
    frameData[colorIndex]     = clamp(blendedColor.x);
    frameData[colorIndex + 1] = clamp(blendedColor.y);
    frameData[colorIndex + 2] = clamp(blendedColor.z);
    frameData[colorIndex + 3] = clamp(blendedColor.w);
    
    // 更新主深度缓冲区（取最小深度）
    if (depth < depthBuffer[pixelIndex]) {
        depthBuffer[pixelIndex] = depth;
    }
}

void FrameBuffer::clear(const Vec4f &color, float depth)
{
    // 预计算颜色值（避免在循环中重复计算）
    uint8_t r = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    uint8_t g = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    uint8_t b = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    uint8_t a = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);

    // 使用更高效的循环
    int totalPixels = width * height;
    // #pragma omp parallel for
    for (int i = 0; i < totalPixels; ++i) {
        // 清除颜色缓冲区
        int offset = i * 4;
        frameData[offset]     = r;
        frameData[offset + 1] = g;
        frameData[offset + 2] = b;
        frameData[offset + 3] = a;
        
        // 清除深度缓冲区
        depthBuffer[i] = depth;
        
        // 清除 MSAA 计数器
        if (msaaEnabled) {
            msaaSampleCount[i] = 0;
        }
    }

    // 清除 MSAA 深度缓冲区（如果启用）
    if (msaaEnabled) {
        std::fill(msaaDepthBuffer.begin(), msaaDepthBuffer.end(), depth);
    }
}