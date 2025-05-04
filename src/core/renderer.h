#pragma once

#include "maths.h"
#include "shader.h"
#include "material.h"
#include "texture.h"
#include "texture_sampler.h"
#include "framebuffer.h"  // 引入独立的framebuffer头文件
#include "profiler.h"     // 引入性能分析模块
#include <memory>
#include <vector>
#include <array>

// 前向声明
class Mesh;
class IShader;
struct ShaderUniforms;
struct Varyings;

// 处理后的顶点数据
struct ProcessedVertex
{
    Vec4f clipPosition;   // 裁剪空间位置
    Vec3f screenPosition; // 屏幕空间位置
    Varyings varying;     // 插值用的顶点属性
};

// 边缘函数计算相关
struct EdgeFunction {
    float dx, dy, c;
    float rowIncrement, colIncrement;
};

// 三角形设置阶段数据
struct TriangleSetupData {
    std::array<ProcessedVertex, 3> vertices;  // 处理后的顶点
    std::array<EdgeFunction, 3> edges;        // 边缘函数
    int minX, minY, maxX, maxY;              // 边界框
    bool valid;                              // 三角形是否有效(通过背面剔除等)
};

// 光栅化渲染器类
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer() = default;

    //--------------------
    // 性能分析相关方法
    //--------------------
    void enableProfiling(bool enable) { profilingEnabled = enable; }
    bool isProfilingEnabled() const { return profilingEnabled; }
    void resetProfilingData() { if (profilingEnabled) PROFILE_RESET(); }
    void printProfilingReport() const { if (profilingEnabled) PROFILE_REPORT(); }

    //--------------------
    // 基础状态设置
    //--------------------
    void setShader(std::shared_ptr<IShader> shader) { this->shader = shader; }
    std::shared_ptr<IShader> getShader() const { return shader; }
    
    void enableMSAA(bool enable);
    void clear(const Vec4f &color = Vec4f(0.0f));

    //--------------------
    // 变换矩阵操作
    //--------------------
    void setModelMatrix(const Matrix4x4f &matrix) { modelMatrix = matrix; }
    void setViewMatrix(const Matrix4x4f &matrix) { viewMatrix = matrix; }
    void setProjMatrix(const Matrix4x4f &matrix) { projMatrix = matrix; }

    Matrix4x4f getModelMatrix() const { return modelMatrix; }
    Matrix4x4f getViewMatrix() const { return viewMatrix; }
    Matrix4x4f getProjMatrix() const { return projMatrix; }
    Matrix4x4f getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }
    
    //--------------------
    // 光照和相机设置
    //--------------------
    void setLight(const Light &light) { this->light = light; }
    Light getLight() const { return light; }
    void setEye(const Vec3f eyePosWS) { this->eyePosWS = eyePosWS; }
    Vec3f getEye() const { return eyePosWS; }

    //--------------------
    // 阴影渲染
    //--------------------
    std::shared_ptr<Texture> createShadowMap(int width, int height);
    void shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters);

    //--------------------
    // 主渲染流程
    //--------------------
    void drawMeshPass(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader);
    void rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader);

    //--------------------
    // 工具方法
    //--------------------
    Vec3f screenMapping(const Vec3f &ndcPos);
    const FrameBuffer &getFrameBuffer() const { return *frameBuffer; }

    //--------------------
    // 三角形设置与遍历
    //--------------------
    // 三角形设置封装方法
    TriangleSetupData setupTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader);
    
    // 三角形遍历封装方法
    void traverseTriangle(const TriangleSetupData &setup, std::shared_ptr<IShader> shader);
    
    // 处理三角形顶点
    void processTriangleVertices(
        const Triangle &triangle,
        std::shared_ptr<IShader> shader,
        std::array<ProcessedVertex, 3> &processedVertices);



private:
    //--------------------
    // 核心数据和状态
    //--------------------
    std::unique_ptr<FrameBuffer> frameBuffer;
    Matrix4x4f modelMatrix;
    Matrix4x4f viewMatrix;
    Matrix4x4f projMatrix;
    std::shared_ptr<IShader> shader;
    
    Light light;
    Vec3f eyePosWS;
    bool msaaEnabled;
    bool profilingEnabled = false; // 性能分析开关

    // 阴影相关
    std::shared_ptr<Texture> shadowMap;
    std::unique_ptr<FrameBuffer> shadowFrameBuffer;

    //--------------------
    // 光栅化核心方法
    //--------------------
    // void processTriangleVertices(
    //     const Triangle &triangle,
    //     std::shared_ptr<IShader> shader,
    //     std::array<ProcessedVertex, 3> &processedVertices);

    void rasterizeStandardPixel(
        int x, int y,
        const std::array<ProcessedVertex, 3> &vertices,
        std::shared_ptr<IShader> shader);

    void rasterizeMSAAPixel(
        int x, int y,
        const std::array<ProcessedVertex, 3> &vertices,
        std::shared_ptr<IShader> shader);

    // 新增的MSAA优化方法
    void processMSAATriangleParallel(
        const std::array<ProcessedVertex, 3> &vertices,
        int minX, int minY, int maxX, int maxY,
        std::shared_ptr<IShader> shader);
        
    void processMSAABlockPixels(
        const std::array<ProcessedVertex, 3> &vertices,
        int blockX, int blockY, int maxBlockX, int maxBlockY,
        const float* edgeParams, std::shared_ptr<IShader> shader);

    // 新增 MSAA 采样点检测
    static const Vec2f msaaSampleOffsets[4];
    bool isSampleInTriangle(
        const std::array<EdgeFunction, 3> &edges, 
        float x, float y, 
        int sampleIndex);

    // 新增封装方法
    void processTriangleParallel(
        const std::array<ProcessedVertex, 3> &vertices,
        int minX, int minY, int maxX, int maxY,
        std::shared_ptr<IShader> shader);
    
    void processTriangleSerial(
        const std::array<ProcessedVertex, 3> &vertices,
        int minX, int minY, int maxX, int maxY,
        std::shared_ptr<IShader> shader);
        
    void processBlockPixels(
        const std::array<ProcessedVertex, 3> &vertices,
        int blockX, int blockY, int maxBlockX, int maxBlockY,
        const float* edgeParams, std::shared_ptr<IShader> shader);

    // 优化的三角形遍历方法
    void traverseTriangleParallel(const TriangleSetupData &setup, std::shared_ptr<IShader> shader);
    void traverseTriangleSerial(const TriangleSetupData &setup, std::shared_ptr<IShader> shader);
    void traverseTriangleBlock(
        const TriangleSetupData &setup,
        int blockX, int blockY, 
        int maxBlockX, int maxBlockY,
        std::shared_ptr<IShader> shader);

    // 添加缺失的函数声明（移除inline关键字）
    Vec4f calculatePerspectiveWeights(
        const Vec3f &barycentric,
        const std::array<ProcessedVertex, 3> &vertices);
        
    float calculateFragmentDepth(
        const Vec3f &barycentric,
        const Vec4f &weights,
        const std::array<ProcessedVertex, 3> &vertices);
        
    void interpolateVaryings(
        Varyings &output,
        const std::array<Varyings, 3> &v,
        const Vec3f &barycentric,
        const Vec4f &weights,
        float fragmentDepth);
        
    FragmentOutput processFragment(
        const Varyings &interpolatedVaryings,
        std::shared_ptr<IShader> shader);
        
    std::tuple<int, int, int, int> calculateBoundingBox(
        const std::array<Vec3f, 3> &screenPositions,
        const int screenWidth,
        const int screenHeight);
    
    std::array<EdgeFunction, 3> setupEdgeFunctions(const std::array<ProcessedVertex, 3> &vertices);
    bool isPointInTriangle(const std::array<EdgeFunction, 3> &edges, float x, float y);

    //--------------------
    // 几何计算辅助方法
    //--------------------
    bool faceCull(const std::array<ProcessedVertex, 3> &vertices, float reverseFactor);
    Vec3f computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v);
    bool isInsideTriangle(const Vec3f &barycentric);
};

