/**
 * @file renderer.cpp
 * @brief 简易版软件光栅器实现，专注于核心光栅化流程
 */

#include "maths.h"
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h"
#include <omp.h>

// 全局常量定义
constexpr int MSAA_SAMPLES = 4;
constexpr float EPSILON = 1e-6f;

// MSAA 采样偏移
const Vec2f MSAA_OFFSETS[MSAA_SAMPLES] = {
    {0.25f, 0.25f}, {0.75f, 0.25f}, {0.25f, 0.75f}, {0.75f, 0.75f}};

// 计算重心坐标
inline Vec3f Renderer::computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v)
{
    const float x0 = v[0].x, y0 = v[0].y;
    const float x1 = v[1].x, y1 = v[1].y;
    const float x2 = v[2].x, y2 = v[2].y;

    // 使用面积法计算重心坐标
    const float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    if (std::abs(area) < EPSILON)
        return Vec3f(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);

    const float invArea = 1.0f / area;
    const float c1 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) * invArea;
    const float c2 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) * invArea;
    const float c3 = 1.0f - c1 - c2;

    return Vec3f(c1, c2, c3);
}

// 判断点是否在三角形内部
inline bool Renderer::isInsideTriangle(const Vec3f &barycentric)
{
    // 简单判断所有重心坐标是否都大于-EPSILON
    return barycentric.x > -EPSILON && barycentric.y > -EPSILON && barycentric.z > -EPSILON;
}

// 执行背面剔除
inline bool Renderer::faceCull(const std::array<ProcessedVertex, 3> &vertices, float reverseFactor)
{
    const float edge1x = vertices[1].screenPosition.x - vertices[0].screenPosition.x;
    const float edge1y = vertices[1].screenPosition.y - vertices[0].screenPosition.y;
    const float edge2x = vertices[2].screenPosition.x - vertices[0].screenPosition.x;
    const float edge2y = vertices[2].screenPosition.y - vertices[0].screenPosition.y;

    // 计算叉积确定朝向
    const float area = edge1x * edge2y - edge1y * edge2x;
    return (area * reverseFactor) <= EPSILON;
}

// 计算透视校正权重
inline Vec4f calculatePerspectiveWeights(
    const Vec3f &barycentric,
    const std::array<ProcessedVertex, 3> &vertices)
{
    // 计算透视校正的权重
    const float invW0 = 1.0f / vertices[0].clipPosition.w;
    const float invW1 = 1.0f / vertices[1].clipPosition.w;
    const float invW2 = 1.0f / vertices[2].clipPosition.w;

    const float interpolated_invW = barycentric.x * invW0 + barycentric.y * invW1 + barycentric.z * invW2;

    return Vec4f(invW0, invW1, invW2, 1.0f / interpolated_invW);
}

// 计算片段深度
inline float calculateFragmentDepth(
    const Vec3f &barycentric,
    const Vec4f &weights,
    const std::array<ProcessedVertex, 3> &vertices)
{
    return (barycentric.x * vertices[0].screenPosition.z * weights.x +
            barycentric.y * vertices[1].screenPosition.z * weights.y +
            barycentric.z * vertices[2].screenPosition.z * weights.z) *
           weights.w;
}

// 插值顶点属性
inline void interpolateVaryings(
    Varyings &output,
    const std::array<Varyings, 3> &v,
    const Vec3f &barycentric,
    const Vec4f &weights,
    float fragmentDepth)
{
    const float correction = weights.w;
    const float w0 = weights.x * barycentric.x;
    const float w1 = weights.y * barycentric.y;
    const float w2 = weights.z * barycentric.z;

    // 位置
    output.position = (v[0].position * w0 + v[1].position * w1 + v[2].position * w2) * correction;

    // 纹理坐标
    output.texCoord = (v[0].texCoord * w0 + v[1].texCoord * w1 + v[2].texCoord * w2) * correction;

    // 颜色
    output.color = (v[0].color * w0 + v[1].color * w1 + v[2].color * w2) * correction;

    // 法线
    output.normal = normalize((v[0].normal * w0 + v[1].normal * w1 + v[2].normal * w2) * correction);

    // 切线
    output.tangent = (v[0].tangent * w0 + v[1].tangent * w1 + v[2].tangent * w2) * correction;
    output.tangent.w = v[0].tangent.w; // 保持w分量不变

    // 深度
    output.depth = fragmentDepth;

    // 光源空间位置（如果有）
    if (v[0].positionLightSpace.w != 0)
    {
        output.positionLightSpace = (v[0].positionLightSpace * w0 +
                                     v[1].positionLightSpace * w1 +
                                     v[2].positionLightSpace * w2) *
                                    correction;
    }
}

// 片段处理逻辑
inline FragmentOutput processFragment(
    const Varyings &interpolatedVaryings,
    std::shared_ptr<IShader> shader)
{
    return shader->fragmentShader(interpolatedVaryings);
}

// 计算边界框
inline std::tuple<int, int, int, int> calculateBoundingBox(
    const std::array<Vec3f, 3> &screenPositions,
    const int screenWidth,
    const int screenHeight)
{
    float minX = std::min(std::min(screenPositions[0].x, screenPositions[1].x), screenPositions[2].x);
    float minY = std::min(std::min(screenPositions[0].y, screenPositions[1].y), screenPositions[2].y);
    float maxX = std::max(std::max(screenPositions[0].x, screenPositions[1].x), screenPositions[2].x);
    float maxY = std::max(std::max(screenPositions[0].y, screenPositions[1].y), screenPositions[2].y);

    // 确保边界在屏幕范围内
    const int boundMinX = std::max(0, static_cast<int>(std::floor(minX)));
    const int boundMinY = std::max(0, static_cast<int>(std::floor(minY)));
    const int boundMaxX = std::min(screenWidth - 1, static_cast<int>(std::ceil(maxX)));
    const int boundMaxY = std::min(screenHeight - 1, static_cast<int>(std::ceil(maxY)));

    return {boundMinX, boundMinY, boundMaxX, boundMaxY};
}

// 构造函数
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      shader(nullptr),
      msaaEnabled(false)
{
    light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f), 1.0f, 0.2f);
    // 设置OpenMP线程数
    // 获取系统可用的处理器核心数
    int max_threads = omp_get_max_threads();
    // 使用所有核心
    omp_set_num_threads(max_threads);
    
    // 设置OpenMP嵌套并行
    omp_set_nested(1);
    
    // 设置动态线程调度
    omp_set_dynamic(1);
}

void Renderer::enableMSAA(bool enable)
{
    msaaEnabled = enable;
    frameBuffer->enableMSAA(enable);
}

void Renderer::clear(const Vec4f &color)
{
    frameBuffer->clear(color);
}

Vec3f Renderer::screenMapping(const Vec3f &ndcPos)
{
    return Vec3f(
        (ndcPos.x + 1.0f) * 0.5f * frameBuffer->getWidth(),
        (1.0f - ndcPos.y) * 0.5f * frameBuffer->getHeight(),
        ndcPos.z);
}

// 处理三角形顶点
void Renderer::processTriangleVertices(
    const Triangle &triangle,
    std::shared_ptr<IShader> shader,
    std::array<ProcessedVertex, 3> &vertices)
{
    VertexAttributes attributes;

    for (int i = 0; i < 3; ++i)
    {
        // 初始化顶点属性
        attributes.position = triangle.vertices[i].position;
        attributes.normal = triangle.vertices[i].normal;
        attributes.tangent = triangle.vertices[i].tangent;
        attributes.texCoord = triangle.vertices[i].texCoord;
        attributes.color = triangle.vertices[i].color;

        // 执行顶点着色器
        const Vec4f &clipPos = shader->vertexShader(attributes, vertices[i].varying);
        vertices[i].clipPosition = clipPos;

        // 透视除法和屏幕映射
        const float invW = 1.0f / clipPos.w;
        vertices[i].screenPosition = screenMapping(Vec3f(
            clipPos.x * invW,
            clipPos.y * invW,
            clipPos.z * invW));
    }
}

// 处理标准模式下的单个像素
void Renderer::rasterizeStandardPixel(
    int x, int y,
    const std::array<ProcessedVertex, 3> &vertices,
    std::shared_ptr<IShader> shader)
{
    // 计算像素中心坐标
    const float pixelCenterX = x + 0.5f;
    const float pixelCenterY = y + 0.5f;

    // 计算重心坐标
    const Vec3f barycentric = computeBarycentric2D(
        pixelCenterX, pixelCenterY,
        {vertices[0].screenPosition, vertices[1].screenPosition, vertices[2].screenPosition});

    // 检查像素是否在三角形内
    if (!isInsideTriangle(barycentric))
        return;

    // 计算透视校正权重
    const Vec4f weights = calculatePerspectiveWeights(barycentric, vertices);

    // 计算深度值
    const float depth = calculateFragmentDepth(barycentric, weights, vertices);

    // 深度测试
    if (!frameBuffer->depthTest(x, y, depth))
        return;

    // 计算插值的顶点属性
    Varyings interpolatedVaryings;
    interpolateVaryings(
        interpolatedVaryings,
        {vertices[0].varying, vertices[1].varying, vertices[2].varying},
        barycentric, weights, depth);

    // 执行片段着色器
    const FragmentOutput output = processFragment(interpolatedVaryings, shader);

    // 跳过被丢弃的片段
    if (output.discard)
        return;

    // 写入输出
    frameBuffer->setPixel(x, y, depth, output.color);
}

// 处理MSAA模式下的单个像素
void Renderer::rasterizeMSAAPixel(
    int x, int y,
    const std::array<ProcessedVertex, 3> &vertices,
    std::shared_ptr<IShader> shader)
{
    // 使用边缘函数优化
    auto edges = setupEdgeFunctions(vertices);
    
    std::array<Vec3f, 3> screenPos = {
        vertices[0].screenPosition,
        vertices[1].screenPosition,
        vertices[2].screenPosition
    };
    
    // 对每个MSAA采样点进行测试
    for (int i = 0; i < 4; ++i) {
        if (isSampleInTriangle(edges, x, y, i)) {
            float sampleX = x + msaaSampleOffsets[i].x;
            float sampleY = y + msaaSampleOffsets[i].y;
            
            // 计算重心坐标
            Vec3f barycentric = computeBarycentric2D(sampleX, sampleY, screenPos);
            
            // 计算透视校正权重
            const Vec4f weights = calculatePerspectiveWeights(barycentric, vertices);
            
            // 计算深度值
            const float depth = calculateFragmentDepth(barycentric, weights, vertices);
            
            // 深度测试
            if (frameBuffer->msaaDepthTest(x, y, i, depth)) {
                // 插值顶点属性
                Varyings interpolatedVaryings;
                interpolateVaryings(
                    interpolatedVaryings,
                    {vertices[0].varying, vertices[1].varying, vertices[2].varying},
                    barycentric, weights, depth);
                
                // 执行片段着色器
                const FragmentOutput output = processFragment(interpolatedVaryings, shader);
                
                // 如果片段没有被丢弃，则写入颜色
                if (!output.discard) {
                    // 将颜色累加到MSAA缓冲区
                    frameBuffer->accumulateMSAAColor(x, y, i, depth, output.color);
                }
            }
        }
    }
}

// 边缘函数计算
std::array<Renderer::EdgeFunction, 3> Renderer::setupEdgeFunctions(
    const std::array<ProcessedVertex, 3> &vertices) 
{
    std::array<EdgeFunction, 3> edges;
    
    // 计算三条边的边缘函数系数
    edges[0].dy = vertices[1].screenPosition.y - vertices[0].screenPosition.y;
    edges[0].dx = vertices[0].screenPosition.x - vertices[1].screenPosition.x;
    edges[0].c = -(edges[0].dx * vertices[0].screenPosition.y + edges[0].dy * vertices[0].screenPosition.x);
    
    edges[1].dy = vertices[2].screenPosition.y - vertices[1].screenPosition.y;
    edges[1].dx = vertices[1].screenPosition.x - vertices[2].screenPosition.x;
    edges[1].c = -(edges[1].dx * vertices[1].screenPosition.y + edges[1].dy * vertices[1].screenPosition.x);
    
    edges[2].dy = vertices[0].screenPosition.y - vertices[2].screenPosition.y;
    edges[2].dx = vertices[2].screenPosition.x - vertices[0].screenPosition.x;
    edges[2].c = -(edges[2].dx * vertices[2].screenPosition.y + edges[2].dy * vertices[2].screenPosition.x);
    
    // 计算行增量和列增量，用于扫描线算法
    for (int i = 0; i < 3; ++i) {
        edges[i].rowIncrement = edges[i].dx;  // 当y增加1时的增量
        edges[i].colIncrement = edges[i].dy;  // 当x增加1时的增量
    }
    
    return edges;
}

// 检查点是否在三角形内部（使用边缘函数）
bool Renderer::isPointInTriangle(
    const std::array<EdgeFunction, 3> &edges, 
    float x, float y) 
{
    // 对所有三条边进行测试
    for (int i = 0; i < 3; ++i) {
        float edgeValue = edges[i].dx * y + edges[i].dy * x + edges[i].c;
        if (edgeValue < -EPSILON)
            return false;
    }
    return true;
}

// 检查MSAA采样点是否在三角形内
bool Renderer::isSampleInTriangle(
    const std::array<EdgeFunction, 3> &edges, 
    float x, float y, 
    int sampleIndex) 
{
    float sampleX = x + msaaSampleOffsets[sampleIndex].x;
    float sampleY = y + msaaSampleOffsets[sampleIndex].y;
    return isPointInTriangle(edges, sampleX, sampleY);
}

// 并行处理三角形（用于较大三角形）
void Renderer::processTriangleParallel(
    const std::array<ProcessedVertex, 3> &vertices,
    int minX, int minY, int maxX, int maxY,
    std::shared_ptr<IShader> shader) 
{
    if (msaaEnabled) {
        #pragma omp parallel for collapse(2) schedule(guided)
        for (int y = minY; y <= maxY; ++y) 
            for (int x = minX; x <= maxX; ++x)
                rasterizeMSAAPixel(x, y, vertices, shader);
    } else {
        // 使用块状处理提高缓存命中率
        const int BLOCK_SIZE = 16; // 可以根据实际缓存大小调整
        
        std::array<EdgeFunction, 3> edges = setupEdgeFunctions(vertices);
        
        #pragma omp parallel for collapse(2) schedule(guided)
        for (int blockY = minY; blockY <= maxY; blockY += BLOCK_SIZE) {
            for (int blockX = minX; blockX <= maxX; blockX += BLOCK_SIZE) {
                // 处理当前块
                int maxBlockY = std::min(blockY + BLOCK_SIZE - 1, maxY);
                int maxBlockX = std::min(blockX + BLOCK_SIZE - 1, maxX);
                
                // 准备边缘函数计算所需参数
                float edgeParams[9] = {
                    edges[0].dx, edges[0].dy, edges[0].c,
                    edges[1].dx, edges[1].dy, edges[1].c,
                    edges[2].dx, edges[2].dy, edges[2].c
                };
                
                processBlockPixels(vertices, blockX, blockY, maxBlockX, maxBlockY, 
                                   edgeParams, shader);
            }
        }
    }
}

// 串行处理三角形（用于较小三角形）
void Renderer::processTriangleSerial(
    const std::array<ProcessedVertex, 3> &vertices,
    int minX, int minY, int maxX, int maxY,
    std::shared_ptr<IShader> shader) 
{
    if (msaaEnabled) {
        for (int y = minY; y <= maxY; ++y) 
            for (int x = minX; x <= maxX; ++x)
                rasterizeMSAAPixel(x, y, vertices, shader);
    } else {
        // 设置边缘函数
        std::array<EdgeFunction, 3> edges = setupEdgeFunctions(vertices);
        
        // 对于小三角形，使用更简单的扫描线方法
        for (int y = minY; y <= maxY; ++y) {
            // 计算当前扫描线的起始边函数值
            float pixelY = y + 0.5f;
            float startX = minX + 0.5f;
            
            float e1_start = edges[0].dx * pixelY + edges[0].dy * startX + edges[0].c;
            float e2_start = edges[1].dx * pixelY + edges[1].dy * startX + edges[1].c;
            float e3_start = edges[2].dx * pixelY + edges[2].dy * startX + edges[2].c;
            
            for (int x = minX; x <= maxX; ++x) {
                // 使用增量更新边函数值
                float e1 = e1_start + edges[0].dy * (x - minX);
                float e2 = e2_start + edges[1].dy * (x - minX);
                float e3 = e3_start + edges[2].dy * (x - minX);
                
                if (e1 >= 0 && e2 >= 0 && e3 >= 0) {
                    rasterizeStandardPixel(x, y, vertices, shader);
                }
            }
        }
    }
}

// 处理像素块
void Renderer::processBlockPixels(
    const std::array<ProcessedVertex, 3> &vertices,
    int blockX, int blockY, int maxBlockX, int maxBlockY,
    const float* edgeParams, std::shared_ptr<IShader> shader) 
{
    // 提取边缘函数参数
    float e1_dx = edgeParams[0], e1_dy = edgeParams[1], e1_c = edgeParams[2];
    float e2_dx = edgeParams[3], e2_dy = edgeParams[4], e2_c = edgeParams[5];
    float e3_dx = edgeParams[6], e3_dy = edgeParams[7], e3_c = edgeParams[8];
    
    // 计算块的起始点的边函数值
    float startX = blockX + 0.5f;
    float startY = blockY + 0.5f;
    
    float e1_start = e1_dx * startY + e1_dy * startX + e1_c;
    float e2_start = e2_dx * startY + e2_dy * startX + e2_c;
    float e3_start = e3_dx * startY + e3_dy * startX + e3_c;
    
    // 检查块的四个角是否都在三角形外部
    bool anyCornerInside = false;
    
    // 左上角
    if (e1_start >= 0 && e2_start >= 0 && e3_start >= 0)
        anyCornerInside = true;
    
    // 右上角
    float e1_right = e1_start + e1_dy * (maxBlockX - blockX);
    float e2_right = e2_start + e2_dy * (maxBlockX - blockX);
    float e3_right = e3_start + e3_dy * (maxBlockX - blockX);
    if (e1_right >= 0 && e2_right >= 0 && e3_right >= 0)
        anyCornerInside = true;
    
    // 左下角
    float e1_bottom = e1_start + e1_dx * (maxBlockY - blockY);
    float e2_bottom = e2_start + e2_dx * (maxBlockY - blockY);
    float e3_bottom = e3_start + e3_dx * (maxBlockY - blockY);
    if (e1_bottom >= 0 && e2_bottom >= 0 && e3_bottom >= 0)
        anyCornerInside = true;
    
    // 右下角
    float e1_bottom_right = e1_bottom + e1_dy * (maxBlockX - blockX);
    float e2_bottom_right = e2_bottom + e2_dy * (maxBlockX - blockX);
    float e3_bottom_right = e3_bottom + e3_dy * (maxBlockX - blockX);
    if (e1_bottom_right >= 0 && e2_bottom_right >= 0 && e3_bottom_right >= 0)
        anyCornerInside = true;
    
    // 如果所有角都在三角形外部，检查是否有任何边穿过块
    if (!anyCornerInside) {
        // 简单地选择继续处理，以防边界穿过块内部
        // 这个检查可以进一步优化
    }
    
    // 处理块内的像素
    for (int y = blockY; y <= maxBlockY; ++y) {
        // 计算当前行的起始边函数值
        float e1_row = e1_start + e1_dx * (y - blockY);
        float e2_row = e2_start + e2_dx * (y - blockY);
        float e3_row = e3_start + e3_dx * (y - blockY);
        
        for (int x = blockX; x <= maxBlockX; ++x) {
            // 增量更新边函数值
            float e1 = e1_row + e1_dy * (x - blockX);
            float e2 = e2_row + e2_dy * (x - blockX);
            float e3 = e3_row + e3_dy * (x - blockX);
            
            // 如果所有边函数值都大于等于0，则点在三角形内
            if (e1 >= 0 && e2 >= 0 && e3 >= 0) {
                // 计算重心坐标（仅在需要时）
                float pixelX = x + 0.5f;
                float pixelY = y + 0.5f;
                Vec3f barycentric = computeBarycentric2D(pixelX, pixelY, 
                    {vertices[0].screenPosition, vertices[1].screenPosition, vertices[2].screenPosition});
                
                // 后续处理与原来相同
                const Vec4f weights = calculatePerspectiveWeights(barycentric, vertices);
                const float depth = calculateFragmentDepth(barycentric, weights, vertices);
                
                if (frameBuffer->depthTest(x, y, depth)) {
                    Varyings interpolatedVaryings;
                    interpolateVaryings(
                        interpolatedVaryings,
                        {vertices[0].varying, vertices[1].varying, vertices[2].varying},
                        barycentric, weights, depth);
                    
                    const FragmentOutput output = processFragment(interpolatedVaryings, shader);
                    if (!output.discard) {
                        frameBuffer->setPixel(x, y, depth, output.color);
                    }
                }
            }
        }
    }
}

// 并行处理MSAA三角形
void Renderer::processMSAATriangleParallel(
    const std::array<ProcessedVertex, 3> &vertices,
    int minX, int minY, int maxX, int maxY,
    std::shared_ptr<IShader> shader)
{
    // 每块像素的尺寸
    static const int BLOCK_SIZE = 8;
    
    // 设置边缘函数
    auto edges = setupEdgeFunctions(vertices);
    
    // 计算块的边界
    int startBlockX = minX / BLOCK_SIZE;
    int startBlockY = minY / BLOCK_SIZE;
    int endBlockX = (maxX + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int endBlockY = (maxY + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    // 预计算边缘函数参数（类似扫描线优化）
    float edgeParams[3 * 4]; // 存储三条边的四个角点的参数
    
    // 块级并行处理
    #pragma omp parallel for collapse(2)
    for (int blockY = startBlockY; blockY < endBlockY; blockY++) {
        for (int blockX = startBlockX; blockX < endBlockX; blockX++) {
            processMSAABlockPixels(vertices, blockX, blockY, endBlockX, endBlockY, edgeParams, shader);
        }
    }
}

// 处理MSAA像素块
void Renderer::processMSAABlockPixels(
    const std::array<ProcessedVertex, 3> &vertices,
    int blockX, int blockY, int maxBlockX, int maxBlockY,
    const float* edgeParams, std::shared_ptr<IShader> shader)
{
    static const int BLOCK_SIZE = 8;
    int startX = blockX * BLOCK_SIZE;
    int startY = blockY * BLOCK_SIZE;
    int endX = std::min(startX + BLOCK_SIZE, frameBuffer->getWidth());
    int endY = std::min(startY + BLOCK_SIZE, frameBuffer->getHeight());
    
    auto edges = setupEdgeFunctions(vertices);
    
    // 对块内每个像素进行MSAA处理
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            // 使用优化的MSAA像素光栅化函数
            rasterizeMSAAPixel(x, y, vertices, shader);
        }
    }
}

// 光栅化三角形 - 重构版本
void Renderer::rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader) {
    if (!shader) {
        std::cerr << "Error: No valid shader provided for triangle rendering.\n";
        return;
    }
    
    // 处理三角形顶点
    std::array<ProcessedVertex, 3> vertices;
    processTriangleVertices(triangle, shader, vertices);
    
    // 执行背面剔除
    float sign = (projMatrix.m11 < 0) ? 1.0f : -1.0f;
    if (faceCull(vertices, sign))
        return;
    
    // 计算边界框
    auto [minX, minY, maxX, maxY] = calculateBoundingBox(
        {vertices[0].screenPosition, vertices[1].screenPosition, vertices[2].screenPosition},
        frameBuffer->getWidth(), frameBuffer->getHeight());
    
    // 检查边界框是否有效
    if (minX > maxX || minY > maxY)
        return;
    
    // 计算三角形大小
    int pixelCount = (maxX - minX + 1) * (maxY - minY + 1);
    
    // 根据三角形大小决定是并行还是串行处理
    if (pixelCount > 1024) {
        // 并行处理较大的三角形
        processTriangleParallel(vertices, minX, minY, maxX, maxY, shader);
    } else {
        // 串行处理较小的三角形
        processTriangleSerial(vertices, minX, minY, maxX, maxY, shader);
    }
}

// 创建阴影贴图
std::shared_ptr<Texture> Renderer::createShadowMap(int width, int height)
{
    shadowFrameBuffer = std::make_unique<FrameBuffer>(width, height);
    shadowFrameBuffer->clear(Vec4f(1.0f), 1.0f);

    auto shadowMap = textures::createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);
    this->shadowMap = shadowMap;

    return shadowMap;
}

// 阴影渲染过程
void Renderer::shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters)
{
    if (!shadowFrameBuffer || !shadowMap)
    {
        std::cerr << "Cannot render shadow map: shadow buffer or texture not initialized" << std::endl;
        return;
    }

    // 保存当前渲染状态
    auto originalMsaaEnabled = msaaEnabled;
    auto originalFrameBuffer = std::move(frameBuffer);

    // 切换到阴影渲染状态
    msaaEnabled = false;
    frameBuffer = std::move(shadowFrameBuffer);
    clear(Vec4f(1.0f));

    auto shadowShader = createShadowMapShader();

    ShaderUniforms uniforms;
    uniforms.viewMatrix = getViewMatrix();
    uniforms.projMatrix = getProjMatrix();
    uniforms.lightSpaceMatrix = uniforms.projMatrix * uniforms.viewMatrix;

    // 渲染阴影投射者
    for (const auto &[mesh, modelMatrix] : shadowCasters)
    {
        uniforms.modelMatrix = modelMatrix;
        shadowShader->setUniforms(uniforms);

        for (const auto &triangle : mesh->getTriangles())
        {
            rasterizeTriangle(triangle, shadowShader);
        }
    }

    // 将阴影帧缓冲复制到阴影贴图纹理
    const int width = frameBuffer->getWidth();
    const int height = frameBuffer->getHeight();

#pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = frameBuffer->getDepth(x, y);
            shadowMap->write(x, y, Vec4f(depth));
        }
    }

    // 恢复原始渲染状态
    shadowFrameBuffer = std::move(frameBuffer);
    frameBuffer = std::move(originalFrameBuffer);
    msaaEnabled = originalMsaaEnabled;
}

// 网格绘制过程
void Renderer::drawMeshPass(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader)
{
    if (!mesh || !activeShader)
    {
        if (!activeShader)
        {
            std::cerr << "Error: No shader set for material, cannot render mesh." << std::endl;
        }
        return;
    }

    // 直接渲染所有三角形
    for (const Triangle &triangle : mesh->getTriangles())
    {
        rasterizeTriangle(triangle, activeShader);
    }
}

// 定义MSAA采样点偏移
const Vec2f Renderer::msaaSampleOffsets[4] = {
    {0.25f, 0.25f}, {0.75f, 0.25f},
    {0.25f, 0.75f}, {0.75f, 0.75f}
};