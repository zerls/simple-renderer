#pragma once

#include "maths.h"
#include "shader.h"
#include "material.h"
#include "texture.h"
#include "texture_sampler.h"
#include <memory>
#include <vector>
#include <array>

// 前向声明
class Mesh;
class IShader;
struct ShaderUniforms;
struct Varyings;

// FrameBuffer帧缓冲类
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

// 处理后的顶点数据
struct ProcessedVertex
{
    Vec4f clipPosition;   // 裁剪空间位置
    Vec3f screenPosition; // 屏幕空间位置
    Varyings varying;     // 插值用的顶点属性
};

// 光栅化渲染器类
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer() = default;

    // 基本设置
    void setShader(std::shared_ptr<IShader> shader) { this->shader = shader; }
    std::shared_ptr<IShader> getShader() const { return shader; }
    void enableMSAA(bool enable);
    void clear(const Vec4f &color = Vec4f(0.0f));

    // 矩阵操作
    void setModelMatrix(const Matrix4x4f &matrix) { modelMatrix = matrix; }
    void setViewMatrix(const Matrix4x4f &matrix) { viewMatrix = matrix; }
    void setProjMatrix(const Matrix4x4f &matrix) { projMatrix = matrix; }

    // Getters
    Matrix4x4f getModelMatrix() const { return modelMatrix; }
    Matrix4x4f getViewMatrix() const { return viewMatrix; }
    Matrix4x4f getProjMatrix() const { return projMatrix; }
    Matrix4x4f getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }
    const FrameBuffer &getFrameBuffer() const { return *frameBuffer; }

    // 绘制方法
    void drawMeshPass(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader);
    void rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader);

    // 阴影相关
    std::shared_ptr<Texture> createShadowMap(int width, int height);
    void shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters);

    // 工具方法
    Vec3f screenMapping(const Vec3f &ndcPos);

    // 光照和视角
    void setLight(const Light &light) { this->light = light; }
    Light getLight() const { return light; }
    void setEye(const Vec3f eyePosWS) { this->eyePosWS = eyePosWS; }
    Vec3f getEye() const { return eyePosWS; }

private:
    // 核心数据
    std::unique_ptr<FrameBuffer> frameBuffer;
    Matrix4x4f modelMatrix;
    Matrix4x4f viewMatrix;
    Matrix4x4f projMatrix;
    std::shared_ptr<IShader> shader;

    // 渲染状态
    Light light;
    Vec3f eyePosWS;
    bool msaaEnabled;

    // 阴影相关
    std::shared_ptr<Texture> shadowMap;
    std::unique_ptr<FrameBuffer> shadowFrameBuffer;

    // 核心光栅化方法
    void processTriangleVertices(
        const Triangle &triangle,
        std::shared_ptr<IShader> shader,
        std::array<ProcessedVertex, 3> &processedVertices);

    void rasterizeStandardPixel(
        int x, int y,
        const std::array<ProcessedVertex, 3> &vertices,
        std::shared_ptr<IShader> shader);

    void rasterizeMSAAPixel(
        int x, int y,
        const std::array<ProcessedVertex, 3> &vertices,
        std::shared_ptr<IShader> shader);

    // 辅助方法
    inline bool faceCull(const std::array<ProcessedVertex, 3> &vertices, float reverseFactor);
    inline Vec3f computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v);
    inline bool isInsideTriangle(const Vec3f &barycentric);
};

// 工厂函数
// std::shared_ptr<IShader> createShadowMapShader();