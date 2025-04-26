// renderer.h的修改
#pragma once

#include "maths.h"
#include "shader.h"
#include "material.h"
#include "texture.h"      // 使用新的纹理库
#include "texture_sampler.h" // 使用新的纹理采样器

// 前向声明
class Mesh;
class IShader;
struct ShaderUniforms;
struct Varyings;

// 帧缓冲类
class FrameBuffer
{
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer() = default;

    // 设置像素（将Color替换为float4）
    void setPixel(int x, int y, float depth, const float4 &color);
    void setPixel(int x, int y, const float4 &color);
    
    // 获取深度值
    float getDepth(int x, int y) const;
    
    // 深度测试（返回是否通过测试）
    bool depthTest(int x, int y, float depth) const;
    
    // 清除缓冲区
    void clear(const float4 &color = float4(0, 0, 0, 1), float depth = 1.0f);
    
    // 获取帧缓冲区数据
    const uint8_t *getData() const { return frameData.data(); }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    std::vector<uint8_t> frameData; // 按 RGBA 格式存储
    std::vector<float> depthBuffer; // 深度缓冲区

    bool isValidCoord(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    int calcIndex(int x, int y) const { return y * width + x; }
};

// 光栅化渲染器类
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer() = default;

    void setShader(std::shared_ptr<IShader> shader) { this->shader = shader; }
    std::shared_ptr<IShader> getShader() const { return shader; }

    // 渲染三角形（使用指定的着色器）
    void rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader);

    // 清除屏幕
    void clear(const float4 &color = float4(0, 0, 0, 1));

    // 设置变换矩阵
    void setModelMatrix(const Matrix4x4f &matrix) { modelMatrix = matrix; }
    void setViewMatrix(const Matrix4x4f &matrix) { viewMatrix = matrix; }
    void setProjMatrix(const Matrix4x4f &matrix) { projMatrix = matrix; }

    // 获取当前变换矩阵
    Matrix4x4f getModelMatrix() const { return modelMatrix; }
    Matrix4x4f getViewMatrix() const { return viewMatrix; }
    Matrix4x4f getProjMatrix() const { return projMatrix; }
    Matrix4x4f getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }

    // 顶点变换和屏幕映射
    Vec3f screenMapping(const Vec3f &clipPos);

    // 获取帧缓冲
    const FrameBuffer &getFrameBuffer() const { return *frameBuffer; }
    
    // 创建阴影贴图
    std::shared_ptr<Texture> createShadowMap(int width, int height);
    
    // 渲染阴影贴图
    void shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters);

    // 绘制网格
    void drawMesh(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader);

    // 光照相关方法
    void setLight(const Light &light) { this->light = light; }
    Light getLight() const { return light; }
    
    // 世界视角相关方法
    void setEye(const Vec3f eyePosWS) { this->eyePosWS = eyePosWS; }
    Vec3f getEye() const { return eyePosWS; }

private:
    std::unique_ptr<FrameBuffer> frameBuffer;
    Matrix4x4f modelMatrix; // 模型变换
    Matrix4x4f viewMatrix;  // 视图变换
    Matrix4x4f projMatrix;  // 投影变换

    std::shared_ptr<IShader> shader; // 当前着色器
    Light light;                     // 光源
    Vec3f eyePosWS;
    
    // 阴影贴图相关
    std::shared_ptr<Texture> shadowMap;
    std::unique_ptr<FrameBuffer> shadowFrameBuffer;
};