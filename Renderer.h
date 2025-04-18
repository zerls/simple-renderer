// Renderer.h
// 基本光栅化渲染器的头文件

#pragma once

#include <vector>
#include <memory>
#include <array>

#include "maths.h"

// 前向声明
class Mesh;

// 光照结构体
struct Light
{
    Vec3f position;          // 光源位置
    Vec3f color;             // 光源颜色
    float intensity;        // 光源强度
    float ambientIntensity; // 环境光强度

    Light() : position(0, 0, 0), color(1, 1, 1), intensity(1.0f), ambientIntensity(0.1f) {}
    Light(const Vec3f &pos, const Vec3f &col, float intens, float ambIntens)
        : position(pos), color(col), intensity(intens), ambientIntensity(ambIntens) {}
};

// 材质结构体
struct Material
{
    Vec3f ambient;    // 环境光反射系数
    Vec3f diffuse;    // 漫反射系数
    Vec3f specular;   // 镜面反射系数
    float shininess; // 光泽度（用于镜面反射计算）

    Material() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.7f, 0.7f, 0.7f),
                 specular(0.2f, 0.2f, 0.2f), shininess(32.0f) {}
    Material(const Vec3f &amb, const Vec3f &diff, const Vec3f &spec, float shin)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shin) {}
};

// 顶点结构体
struct Vertex
{
    Vec3f position; // 位置
    Vec3f normal;   // 法线
    Vec2f texCoord; // 纹理坐标
    Color color;   // 顶点颜色

    Vertex() = default;
    Vertex(const Vec3f &pos, const Color &col) : position(pos), color(col) {}
    Vertex(const Vec3f &pos, const Vec3f &norm, const Vec2f &tex, const Color &col) : position(pos), normal(norm), texCoord(tex), color(col) {}
};

// 三角形结构体
struct Triangle
{
    std::array<Vertex, 3> vertices;

    Triangle() = default;
    Triangle(const Vertex &v1, const Vertex &v2, const Vertex &v3)
    {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
    }
};


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

    bool lightingEnabled;
    Light light;
    // 绘制变换后的三角形
    void drawTriangle(const Triangle &triangle, const Matrix4x4f &mvpMatrix);

    // 绘制未变换的三角形(原始方法)
    void drawTriangle(const Triangle &triangle);

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

    // 获取帧缓冲
    const FrameBuffer &getFrameBuffer() const { return *frameBuffer; }

    // void drawCube(const Cube &cube);

    // 绘制网格（新增）
    void drawMesh(const std::shared_ptr<Mesh> &mesh);

    // 启用光照
    void enableLighting(bool b) { this->lightingEnabled = b; };
    Color calculateLighting(const Vertex &vertex, const Material &material, const Vec3f &eyePos);
    void setLight(Light &light) { this->light = light; };

    // 保存深度图到PPM文件
    void saveDepthMap(const std::string &filename, float nearPlane, float farPlane);

private:
    std::unique_ptr<FrameBuffer> frameBuffer;
    Matrix4x4f modelMatrix; // 模型变换
    Matrix4x4f viewMatrix;  // 视图变换
    Matrix4x4f projMatrix;  // 投影变换
};
