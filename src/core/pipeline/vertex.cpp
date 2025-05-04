/**
 * @file renderer_vertex.cpp
 * @brief 渲染器几何和顶点处理实现
 */
#include "maths.h"
#include "renderer.h"
#include "mesh.h"
#include <omp.h>

// 全局常量
constexpr float EPSILON = 1e-6f;

// 屏幕映射（从NDC到屏幕坐标）
Vec3f Renderer::screenMapping(const Vec3f &ndcPos)
{
    return Vec3f(
        (ndcPos.x + 1.0f) * 0.5f * frameBuffer->getWidth(),
        (1.0f - ndcPos.y) * 0.5f * frameBuffer->getHeight(),
        ndcPos.z);
}

// 执行背面剔除
bool Renderer::faceCull(const std::array<ProcessedVertex, 3> &vertices, float reverseFactor)
{
    const float edge1x = vertices[1].screenPosition.x - vertices[0].screenPosition.x;
    const float edge1y = vertices[1].screenPosition.y - vertices[0].screenPosition.y;
    const float edge2x = vertices[2].screenPosition.x - vertices[0].screenPosition.x;
    const float edge2y = vertices[2].screenPosition.y - vertices[0].screenPosition.y;

    // 计算叉积确定朝向
    const float area = edge1x * edge2y - edge1y * edge2x;
    return (area * reverseFactor) <= EPSILON;
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

// 计算边界框
std::tuple<int, int, int, int> Renderer::calculateBoundingBox(
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

// 边缘函数计算
std::array<EdgeFunction, 3> Renderer::setupEdgeFunctions(
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

