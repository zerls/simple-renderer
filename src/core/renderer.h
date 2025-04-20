// Renderer.h
// 基本光栅化渲染器的头文件 - 更新版本

#pragma once

#include <vector>
#include <memory>
#include <array>

#include "maths.h"
#include "shader.h"
#include "material.h"

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

    // 设置像素（带深度测试）
    void setPixel(int x, int y, float depth, const Color &color);
    // 设置像素（不带深度测试）
    void setPixel(int x, int y, const Color &color);
    // 获取深度值
    float getDepth(int x, int y) const;
    // 清除缓冲区
    void clear(const Color &color = Color(0, 0, 0, 255), float depth = 1.0f);
    // 获取帧缓冲区数据
    const uint8_t *getData() const { return frameData.data(); }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    std::vector<uint8_t> frameData; // 按 RGBA 格式存储
    std::vector<float> depthBuffer; // 深度缓冲区

    // 检查坐标是否在有效范围内
    bool isValidCoord(int x, int y) const
    {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    // 计算索引
    int calcIndex(int x, int y) const
    {
        return y * width + x;
    }
};

// 光栅化渲染器类
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer() = default;

    // 设置当前着色器（全局默认着色器）
    void setShader(std::shared_ptr<IShader> shader) { this->shader = shader; }
    std::shared_ptr<IShader> getShader() const { return shader; }

    // 栅格化三角形（使用指定的着色器）
    void rasterizeTriangle(const Triangle& triangle, std::shared_ptr<IShader> shader);
    
    // 清除屏幕
    void clear(const Color &color = Color(0, 0, 0, 255));

    // 设置变换矩阵
    void setModelMatrix(const Matrix4x4f &matrix) { modelMatrix = matrix; }
    void setViewMatrix(const Matrix4x4f &matrix) { viewMatrix = matrix; }
    void setProjMatrix(const Matrix4x4f &matrix) { projMatrix = matrix; }

    // 获取当前变换矩阵
    Matrix4x4f getModelMatrix() const { return modelMatrix; }
    Matrix4x4f getViewMatrix() const { return viewMatrix; }
    Matrix4x4f getProjMatrix() const { return projMatrix; }
    Matrix4x4f getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }

    // 变换顶点位置
    Vec3f transformVertex(const Vec3f &position, const Matrix4x4f &mvpMatrix);
    
    // 屏幕映射函数
    Vec3f screenMapping(const Vec3f& clipPos);

    // 获取帧缓冲
    const FrameBuffer &getFrameBuffer() const { return *frameBuffer; }

    // 绘制网格
    void drawMesh(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<Material> material);

    // 光照相关方法
    void setLight(const Light &light) { this->light = light; }
    Light getLight() const { return light; }
    //世界视角相关方法
    void setEye(const Vec3f eyePosWS) { this->eyePosWS = eyePosWS; }
    Vec3f getEye() const{return eyePosWS;}

private:
    std::unique_ptr<FrameBuffer> frameBuffer;
    Matrix4x4f modelMatrix; // 模型变换
    Matrix4x4f viewMatrix;  // 视图变换
    Matrix4x4f projMatrix;  // 投影变换
    
    std::shared_ptr<IShader> shader; // 当前着色器
    Light light; // 光源
    Vec3f eyePosWS;
};

