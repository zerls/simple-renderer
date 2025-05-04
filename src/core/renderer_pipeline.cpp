/**
 * @file renderer_pipeline.cpp
 * @brief 渲染器光栅化和三角形处理实现
 */
#include "maths.h"
#include "renderer.h"
#include <omp.h>

// 全局常量定义
constexpr int MSAA_SAMPLES = 4;
constexpr float EPSILON = 1e-6f;

// MSAA 采样偏移
const Vec2f MSAA_OFFSETS[MSAA_SAMPLES] = {
    {0.25f, 0.25f}, {0.75f, 0.25f}, {0.25f, 0.75f}, {0.75f, 0.75f}};

// 光栅化三角形 - 主入口
void Renderer::rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader) {
    if (!shader) {
        std::cerr << "Error: No valid shader provided for triangle rendering.\n";
        return;
    }
    
    // 使用封装的三角形设置阶段
    TriangleSetupData setup = setupTriangle(triangle, shader);
    
    // 如果三角形无效，直接返回
    if (!setup.valid) {
        return;
    }
    
    // 使用封装的三角形遍历阶段
    traverseTriangle(setup, shader);
}

// 三角形设置阶段封装
TriangleSetupData Renderer::setupTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader) {
    TriangleSetupData setup;
    setup.valid = false;
    
    // 处理三角形顶点
    processTriangleVertices(triangle, shader, setup.vertices);
    
    // 执行背面剔除
    float sign = (projMatrix.m11 < 0) ? 1.0f : -1.0f;
    if (faceCull(setup.vertices, sign)) {
        return setup; // 返回无效的设置数据
    }
    
    // 计算边界框
    auto [minX, minY, maxX, maxY] = calculateBoundingBox(
        {setup.vertices[0].screenPosition, setup.vertices[1].screenPosition, setup.vertices[2].screenPosition},
        frameBuffer->getWidth(), frameBuffer->getHeight());
    
    // 检查边界框是否有效
    if (minX > maxX || minY > maxY) {
        return setup; // 返回无效的设置数据
    }
    
    // 设置边界框
    setup.minX = minX;
    setup.minY = minY;
    setup.maxX = maxX;
    setup.maxY = maxY;
    
    // 设置边缘函数
    setup.edges = setupEdgeFunctions(setup.vertices);
    
    // 标记为有效
    setup.valid = true;
    
    return setup;
}

// 三角形遍历阶段封装
void Renderer::traverseTriangle(const TriangleSetupData &setup, std::shared_ptr<IShader> shader) {
    // 计算三角形大小
    int pixelCount = (setup.maxX - setup.minX + 1) * (setup.maxY - setup.minY + 1);
    
    // 根据三角形大小决定是并行还是串行处理
    if (pixelCount > 1024) {
        // 并行处理较大的三角形
        traverseTriangleParallel(setup, shader);
    } else {
        // 串行处理较小的三角形
        traverseTriangleSerial(setup, shader);
    }
}

// 并行遍历三角形
void Renderer::traverseTriangleParallel(const TriangleSetupData &setup, std::shared_ptr<IShader> shader) {
    if (msaaEnabled) {
        #pragma omp parallel for collapse(2) schedule(guided)
        for (int y = setup.minY; y <= setup.maxY; ++y) 
            for (int x = setup.minX; x <= setup.maxX; ++x)
                rasterizeMSAAPixel(x, y, setup.vertices, shader);
    } else {
        // 使用块状处理提高缓存命中率
        const int BLOCK_SIZE = 16; // 可以根据实际缓存大小调整
        
        #pragma omp parallel for collapse(2) schedule(guided)
        for (int blockY = setup.minY; blockY <= setup.maxY; blockY += BLOCK_SIZE) {
            for (int blockX = setup.minX; blockX <= setup.maxX; blockX += BLOCK_SIZE) {
                // 处理当前块
                int maxBlockY = std::min(blockY + BLOCK_SIZE - 1, setup.maxY);
                int maxBlockX = std::min(blockX + BLOCK_SIZE - 1, setup.maxX);
                
                traverseTriangleBlock(setup, blockX, blockY, maxBlockX, maxBlockY, shader);
            }
        }
    }
}

// 串行遍历三角形
void Renderer::traverseTriangleSerial(const TriangleSetupData &setup, std::shared_ptr<IShader> shader) {
    if (msaaEnabled) {
        for (int y = setup.minY; y <= setup.maxY; ++y) 
            for (int x = setup.minX; x <= setup.maxX; ++x)
                rasterizeMSAAPixel(x, y, setup.vertices, shader);
    } else {
        const auto &edges = setup.edges;
        
        // 对于小三角形，使用更简单的扫描线方法
        for (int y = setup.minY; y <= setup.maxY; ++y) {
            // 计算当前扫描线的起始边函数值
            float pixelY = y + 0.5f;
            float startX = setup.minX + 0.5f;
            
            float e1_start = edges[0].dx * pixelY + edges[0].dy * startX + edges[0].c;
            float e2_start = edges[1].dx * pixelY + edges[1].dy * startX + edges[1].c;
            float e3_start = edges[2].dx * pixelY + edges[2].dy * startX + edges[2].c;
            
            for (int x = setup.minX; x <= setup.maxX; ++x) {
                // 使用增量更新边函数值
                float e1 = e1_start + edges[0].dy * (x - setup.minX);
                float e2 = e2_start + edges[1].dy * (x - setup.minX);
                float e3 = e3_start + edges[2].dy * (x - setup.minX);
                
                if (e1 >= 0 && e2 >= 0 && e3 >= 0) {
                    rasterizeStandardPixel(x, y, setup.vertices, shader);
                }
            }
        }
    }
}

// 处理三角形块
void Renderer::traverseTriangleBlock(
    const TriangleSetupData &setup,
    int blockX, int blockY, 
    int maxBlockX, int maxBlockY,
    std::shared_ptr<IShader> shader) 
{
    const auto &edges = setup.edges;
    const auto &vertices = setup.vertices;
    
    // 计算块的起始点的边函数值
    float startX = blockX + 0.5f;
    float startY = blockY + 0.5f;
    
    float e1_start = edges[0].dx * startY + edges[0].dy * startX + edges[0].c;
    float e2_start = edges[1].dx * startY + edges[1].dy * startX + edges[1].c;
    float e3_start = edges[2].dx * startY + edges[2].dy * startX + edges[2].c;
    
    // 检查块的四个角是否都在三角形外部
    bool anyCornerInside = false;
    
    // 左上角
    if (e1_start >= 0 && e2_start >= 0 && e3_start >= 0)
        anyCornerInside = true;
    
    // 右上角
    float e1_right = e1_start + edges[0].dy * (maxBlockX - blockX);
    float e2_right = e2_start + edges[1].dy * (maxBlockX - blockX);
    float e3_right = e3_start + edges[2].dy * (maxBlockX - blockX);
    if (e1_right >= 0 && e2_right >= 0 && e3_right >= 0)
        anyCornerInside = true;
    
    // 左下角
    float e1_bottom = e1_start + edges[0].dx * (maxBlockY - blockY);
    float e2_bottom = e2_start + edges[1].dx * (maxBlockY - blockY);
    float e3_bottom = e3_start + edges[2].dx * (maxBlockY - blockY);
    if (e1_bottom >= 0 && e2_bottom >= 0 && e3_bottom >= 0)
        anyCornerInside = true;
    
    // 右下角
    float e1_bottom_right = e1_bottom + edges[0].dy * (maxBlockX - blockX);
    float e2_bottom_right = e2_bottom + edges[1].dy * (maxBlockX - blockX);
    float e3_bottom_right = e3_bottom + edges[2].dy * (maxBlockX - blockX);
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
        float e1_row = e1_start + edges[0].dx * (y - blockY);
        float e2_row = e2_start + edges[1].dx * (y - blockY);
        float e3_row = e3_start + edges[2].dx * (y - blockY);
        
        for (int x = blockX; x <= maxBlockX; ++x) {
            // 增量更新边函数值
            float e1 = e1_row + edges[0].dy * (x - blockX);
            float e2 = e2_row + edges[1].dy * (x - blockX);
            float e3 = e3_row + edges[2].dy * (x - blockX);
            
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

// 保留其他处理函数
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

