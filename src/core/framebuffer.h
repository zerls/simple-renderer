#pragma once

#include "maths.h"
#include <cstdint>

class FrameBuffer
{
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer();

    // 核心操作
    void setPixel(int x, int y, float depth, const Vec4f &color);
    void clear(const Vec4f &color = Vec4f(0.0f), float depth = 1.0f);

    // MSAA 相关
    void enableMSAA(bool enable);
    void accumulateMSAAColor(int x, int y, int sampleIndex, float depth, const Vec4f &color);

    // 深度测试
    bool depthTest(int x, int y, float depth) const;
    bool msaaDepthTest(int x, int y, int sampleIndex, float depth) const;

    // Getters
    float getDepth(int x, int y) const;
    float getMSAADepth(int x, int y, int sampleIndex) const;
    const uint8_t *getData() const;
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isMSAAEnabled() const { return msaaEnabled; }
    void copyToPlatform(uint32_t* dst) const;

private:
    // 核心数据
    int width, height;
    uint8_t* colorBuffer;  // 颜色缓冲区
    float* depthBuffer;    // 深度缓冲区

    // MSAA 相关
    bool msaaEnabled;
    float* msaaDepthBuffer; // MSAA深度缓冲区
    int* msaaSampleCount;   // MSAA样本计数

    // 辅助方法
    bool isValidCoord(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    int calcIndex(int x, int y) const { return y * width + x; }
    int calcMSAAIndex(int x, int y, int sampleIndex) const;
};
