// 基本光栅化渲染器的实现文件
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h" // 替换 test.h
#include <omp.h>

// 定义 MSAA 采样点数量和 epsilon 常量
constexpr int MSAA_SAMPLES = 4; // 4x MSAA
constexpr float EPSILON = 1e-6f;

// MSAA 采样点偏移
const Vec2f MSAA_OFFSETS[MSAA_SAMPLES] = {
    {0.25f, 0.25f},
    {0.75f, 0.25f},
    {0.25f, 0.75f},
    {0.75f, 0.75f}
};

// FrameBuffer 实现
FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height)
{
    // 初始化帧缓冲区，每个像素4个字节 (RGBA)
    frameData.resize(width * height * 4, 0);
    // 初始化深度缓冲区，每个像素一个浮点数
    depthBuffer.resize(width * height, 1.0f); // 初始化为最远深度(1.0)
    

    // 初始化MSAA缓冲区
    msaaEnabled = false;
    msaaColorBuffer.resize(width * height * MSAA_SAMPLES * 4, 0);
    msaaDepthBuffer.resize(width * height * MSAA_SAMPLES, 1.0f);
    

}

// 启用或禁用 MSAA
void FrameBuffer::enableMSAA(bool enable)
{
    if (msaaEnabled != enable) {
        msaaEnabled = enable;
        
        if (msaaEnabled) {
            // 分配MSAA缓冲区
            msaaColorBuffer.resize(width * height * MSAA_SAMPLES * 4, 0);
            msaaDepthBuffer.resize(width * height * MSAA_SAMPLES, 1.0f);
        } else {
            // 释放MSAA缓冲区
            std::vector<uint8_t>().swap(msaaColorBuffer);
            std::vector<float>().swap(msaaDepthBuffer);
        }
    }
}

// 设置像素颜色（带深度值）
void FrameBuffer::setPixel(int x, int y, float depth, const Vec4f &color)
{
    if (!isValidCoord(x, y))
    {
        return;
    }

    int index = calcIndex(x, y);

    // 更新深度缓冲区
    depthBuffer[index] = depth;

    // 更新颜色缓冲区
    int colorIndex = index * 4;
    // 从Vec4f转为颜色值
    frameData[colorIndex] = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 1] = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 2] = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 3] = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);
}

// 设置 MSAA 采样点
void FrameBuffer::setMSAASample(int x, int y, int sampleIndex, float depth, const Vec4f &color)
{
    if (!isValidCoord(x, y) || sampleIndex >= MSAA_SAMPLES)
    {
        return;
    }

    int pixelIndex = calcIndex(x, y);
    int sampleOffset = pixelIndex * MSAA_SAMPLES + sampleIndex;
    
    // 更新 MSAA 深度缓冲区
    msaaDepthBuffer[sampleOffset] = depth;
    
    // 更新 MSAA 颜色缓冲区
    int colorIndex = sampleOffset * 4;
    msaaColorBuffer[colorIndex] = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    msaaColorBuffer[colorIndex + 1] = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    msaaColorBuffer[colorIndex + 2] = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    msaaColorBuffer[colorIndex + 3] = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);
}

// 解析 MSAA 缓冲区到普通缓冲区
void FrameBuffer::resolveMSAA()
{
    if (!msaaEnabled)
        return;
        
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int pixelIndex = calcIndex(x, y);
            int pixelColorIndex = pixelIndex * 4;
            
            // 对所有采样点进行平均
            Vec4f avgColor(0.0f, 0.0f, 0.0f, 0.0f);
            float minDepth = 1.0f;
            
            for (int s = 0; s < MSAA_SAMPLES; ++s)
            {
                int sampleOffset = pixelIndex * MSAA_SAMPLES + s;
                int sampleColorIndex = sampleOffset * 4;
                
                // 累加颜色
                avgColor.x += msaaColorBuffer[sampleColorIndex] / 255.0f;
                avgColor.y += msaaColorBuffer[sampleColorIndex + 1] / 255.0f;
                avgColor.z += msaaColorBuffer[sampleColorIndex + 2] / 255.0f;
                avgColor.w += msaaColorBuffer[sampleColorIndex + 3] / 255.0f;
                
                // 找到最小深度值
                minDepth = std::min(minDepth, msaaDepthBuffer[sampleOffset]);
            }
            
            // 计算平均颜色
            avgColor = avgColor * (1.0f / MSAA_SAMPLES);
            
            // 更新普通缓冲区
            frameData[pixelColorIndex] = static_cast<uint8_t>(std::min(std::max(avgColor.x, 0.0f), 1.0f) * 255);
            frameData[pixelColorIndex + 1] = static_cast<uint8_t>(std::min(std::max(avgColor.y, 0.0f), 1.0f) * 255);
            frameData[pixelColorIndex + 2] = static_cast<uint8_t>(std::min(std::max(avgColor.z, 0.0f), 1.0f) * 255);
            frameData[pixelColorIndex + 3] = static_cast<uint8_t>(std::min(std::max(avgColor.w, 0.0f), 1.0f) * 255);
            
            // 更新深度缓冲区
            depthBuffer[pixelIndex] = minDepth;
        }
    }
}

void FrameBuffer::setPixel(int x, int y, const Vec4f &color)
{
    if (!isValidCoord(x, y))
    {
        return;
    }

    int index = calcIndex(x, y);

    // 没有深度测试，在 setPixel 外执行

    // 更新颜色缓冲区
    int colorIndex = index * 4;
    // 从Vec4f转为颜色值
    frameData[colorIndex] = static_cast<uint8_t>(std::min(std::max(color.x, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 1] = static_cast<uint8_t>(std::min(std::max(color.y, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 2] = static_cast<uint8_t>(std::min(std::max(color.z, 0.0f), 1.0f) * 255);
    frameData[colorIndex + 3] = static_cast<uint8_t>(std::min(std::max(color.w, 0.0f), 1.0f) * 255);
}

// 添加深度测试方法
bool FrameBuffer::depthTest(int x, int y, float depth) const
{
    if (!isValidCoord(x, y))
    {
        return false;
    }

    int index = calcIndex(x, y);
    return depth < depthBuffer[index];
}

// MSAA 深度测试
bool FrameBuffer::depthTestMSAA(int x, int y, int sampleIndex, float depth) const
{
    if (!isValidCoord(x, y) || sampleIndex >= MSAA_SAMPLES)
    {
        return false;
    }

    int pixelIndex = calcIndex(x, y);
    int sampleOffset = pixelIndex * MSAA_SAMPLES + sampleIndex;
    return depth < msaaDepthBuffer[sampleOffset];
}

float FrameBuffer::getDepth(int x, int y) const
{
    if (!isValidCoord(x, y))
    {
        return 1.0f; // 返回最远深度
    }

    return depthBuffer[calcIndex(x, y)];
}

void FrameBuffer::clear(const Vec4f &color, float depth)
{
    // 使用指定颜色和深度值清除整个缓冲区

    // 清除颜色缓冲区
    if (color.z >= 1.0f)
    { // 如果颜色是完全不透明的，可以使用更快的填充方法
        std::fill(frameData.begin(), frameData.end(), 0);
        for (size_t i = 0; i < frameData.size(); i += 4)
        {
            frameData[i] = color.x;
            frameData[i + 1] = color.y;
            frameData[i + 2] = color.z;
            frameData[i + 3] = color.w;
        }
    }
    else
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                setPixel(x, y, color);
            }
        }
    }

    // 清除深度缓冲区
    std::fill(depthBuffer.begin(), depthBuffer.end(), depth);
    
    // 清除 MSAA 缓冲区
    if (msaaEnabled)
    {
        std::fill(msaaDepthBuffer.begin(), msaaDepthBuffer.end(), depth);
        
        if (color.w >= 1.0f)
        {
            std::fill(msaaColorBuffer.begin(), msaaColorBuffer.end(), 0);
            for (size_t i = 0; i < msaaColorBuffer.size(); i += 4)
            {
                msaaColorBuffer[i] = static_cast<uint8_t>(color.x * 255);
                msaaColorBuffer[i + 1] = static_cast<uint8_t>(color.y * 255);
                msaaColorBuffer[i + 2] = static_cast<uint8_t>(color.z * 255);
                msaaColorBuffer[i + 3] = static_cast<uint8_t>(color.w * 255);
            }
        }
    }
}

// 创建阴影贴图
std::shared_ptr<Texture> Renderer::createShadowMap(int width, int height)
{
    // 创建阴影帧缓冲
    shadowFrameBuffer = std::make_unique<FrameBuffer>(width, height);
    shadowFrameBuffer->clear(Vec4f(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);

    // 创建阴影贴图纹理
    auto shadowMap = textures::createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);

    this->shadowMap = shadowMap;
    return shadowMap;
}

// 渲染阴影贴图
void Renderer::shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters)
{
    if (!shadowFrameBuffer || !shadowMap)
    {
        std::cerr << "无法渲染阴影贴图：阴影缓冲或纹理未初始化" << std::endl;
        return;
    }

    auto originalmsaaEnabled=msaaEnabled;
    auto originalFrameBuffer = std::move(frameBuffer); // 保存当前渲染状态 ｜ShadowBuffer 与 FrameBuffer 的尺寸不一致
    msaaEnabled =false; // 禁用 MSAA
    frameBuffer = std::move(shadowFrameBuffer); // 切换到阴影帧缓冲

    // 清除阴影帧缓冲
    clear(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

    // 创建阴影贴图着色器
    auto shadowShader = createShadowMapShader();

    // 设置着色器的统一变量
    ShaderUniforms uniforms;
    uniforms.viewMatrix = getViewMatrix();
    ;
    uniforms.projMatrix = getProjMatrix();
    ;
    uniforms.lightSpaceMatrix = uniforms.projMatrix * uniforms.viewMatrix;

    // 渲染所有网格到阴影贴图
    for (const auto &[mesh, _modelMatrix] : shadowCasters)
    {
        uniforms.modelMatrix = _modelMatrix;
        shadowShader->setUniforms(uniforms);
        for (const auto &triangle : mesh->getTriangles())
        {
            rasterizeTriangle(triangle, shadowShader);
        }
    }

    // 将阴影帧缓冲复制到阴影贴图纹理
    for (int y = 0; y < frameBuffer->getHeight(); ++y)
    {
        for (int x = 0; x < frameBuffer->getWidth(); ++x)
        {
            int index = y * frameBuffer->getWidth() + x;
            float depth = frameBuffer->getDepth(x, y);
            // 将深度值写入阴影贴图纹理
            shadowMap->write(x, y, Vec4f(depth));
        }
    }
    // TODO 增加让灰度对比更明显的函数
    shadowMap->saveDepthToFile("../output/shadow.tga", 0.0f, 1.0f);
    // saveDepthMap("../output/shadow.ppm", *frameBuffer,0.5,2.0);
    // 恢复原始渲染状态
    msaaEnabled =originalmsaaEnabled;
    shadowFrameBuffer = std::move(frameBuffer);
    frameBuffer = std::move(originalFrameBuffer);
}

// Renderer 构造函数
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      shader(nullptr), // 初始化着色器为空
      msaaEnabled(false) // 默认禁用 MSAA
{
    // 初始化默认光源
    this->light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);

    // 设置OpenMP线程数，只需要设置一次
    omp_set_num_threads(4);
}

// 启用或禁用 MSAA
void Renderer::enableMSAA(bool enable)
{
    msaaEnabled = enable;
    frameBuffer->enableMSAA(enable);
}

void Renderer::clear(const Vec4f &color)
{
    frameBuffer->clear(color);
}

// 计算重心坐标 - 优化版本
Vec3f computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v)
{
    float c1 = (x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*y + v[1].x*v[2].y - v[2].x*v[1].y) / 
               (v[0].x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*v[0].y + v[1].x*v[2].y - v[2].x*v[1].y);
    float c2 = (x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*y + v[2].x*v[0].y - v[0].x*v[2].y) / 
               (v[1].x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*v[1].y + v[2].x*v[0].y - v[0].x*v[2].y);
    return Vec3f(c1, c2, 1.0f - c1 - c2);
}

// 判断点是否在三角形内
bool isInsideTriangle(float alpha, float beta, float gamma)
{
    // 使用 epsilon 缓解精度问题
    return alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON;
}

// Calculate bounding box for triangle rasterization
constexpr auto calculateBoundingBox(
    const std::array<Vec3f, 3> &screenPositions,
    const int screenWidth,
    const int screenHeight)
{
    // Use SIMD-friendly min/max operations
    float minX = screenPositions[0].x;
    float minY = screenPositions[0].y;
    float maxX = screenPositions[0].x;
    float maxY = screenPositions[0].y;

    for (int i = 1; i < 3; ++i)
    {
        minX = std::min(minX, screenPositions[i].x);
        minY = std::min(minY, screenPositions[i].y);
        maxX = std::max(maxX, screenPositions[i].x);
        maxY = std::max(maxY, screenPositions[i].y);
    }

    // Convert to integer coordinates with proper rounding
    const int boundMinX = std::max(0, static_cast<int>(std::floor(minX)));
    const int boundMinY = std::max(0, static_cast<int>(std::floor(minY)));
    const int boundMaxX = std::min(screenWidth - 1, static_cast<int>(std::ceil(maxX)));
    const int boundMaxY = std::min(screenHeight - 1, static_cast<int>(std::ceil(maxY)));

    return std::make_tuple(boundMinX, boundMinY, boundMaxX, boundMaxY);
}

// 屏幕映射函数
Vec3f Renderer::screenMapping(const Vec3f &ndcPos)
{
    // 屏幕映射 - 变换到屏幕坐标系
    float screenX = (ndcPos.x + 1.0f) * 0.5f * frameBuffer->getWidth();
    float screenY = (1.0f - ndcPos.y) * 0.5f * frameBuffer->getHeight(); // 注意Y轴翻转

    // z值保持不变(用于深度缓冲)
    return Vec3f(screenX, screenY, ndcPos.z);
}

/**
 * @brief 透视校正插值函数
 *
 * @param interpolated 输出插值后的顶点属性
 * @param v 三角形三个顶点的属性数组 
 * @param b 重心坐标(b.x, b.y, b.z)
 * @param w 透视校正权重
 *    w.xyz(): 每个顶点的权重
 *    w.w: 透视校正系数 
 * @param fragmentDepth 片元深度值
 *
 * @details
 * 透视校正插值公式:
 * result = (v0*w0*b0 + v1*w1*b1 + v2*w2*b2) * wCorrection
 * - v0,v1,v2: 顶点属性
 * - w0,w1,w2: 透视权重  
 * - b0,b1,b2: 重心坐标
 * - wCorrection: 透视校正系数
 *
 * 特殊处理:
 * - 法线: 插值后需要重新归一化
 * - 切线: 保留第一个顶点的w分量
 * - 光照空间位置: 仅当w!=0时才进行插值
 */
void InitializeInterpolateVaryings(Varyings &interpolated,
                                   const std::array<Varyings, 3> &v,
                                   const Vec3f &b,
                                   const Vec4f &w,
                                   float fragmentDepth)
{
    // Extract perspective weight components for better readability
    const Vec3f weights = w.xyz();
    const float wCorrection = w.w;

    // Helper lambda for perspective correct interpolation
    auto interpolate = [&](const auto &v0, const auto &v1, const auto &v2)
    {
        return (v0 * weights.x * b.x + v1 * weights.y * b.y + v2 * weights.z * b.z) * wCorrection;
    };

    // Interpolate all attributes
    interpolated.position = interpolate(v[0].position, v[1].position, v[2].position);
    interpolated.texCoord = interpolate(v[0].texCoord, v[1].texCoord, v[2].texCoord);
    interpolated.color = interpolate(v[0].color, v[1].color, v[2].color);

    // Normal requires normalization after interpolation
    interpolated.normal = normalize(interpolate(v[0].normal, v[1].normal, v[2].normal));

    // Tangent interpolation with w component preservation
    interpolated.tangent = interpolate(v[0].tangent, v[1].tangent, v[2].tangent);
    interpolated.tangent.w = v[0].tangent.w; // Preserve w component from first vertex

    interpolated.depth = fragmentDepth;

    // Light space position interpolation (if available)
    if (v[0].positionLightSpace.w != 0)
    {
        interpolated.positionLightSpace = interpolate(
            v[0].positionLightSpace,
            v[1].positionLightSpace,
            v[2].positionLightSpace);
    }
}

void InitializeVertexAttributes(VertexAttributes &attributes, const Vertex &vertex)
{
    // 初始化顶点属性
    attributes.position = vertex.position;
    attributes.normal = vertex.normal;
    attributes.tangent = vertex.tangent;
    attributes.texCoord = vertex.texCoord;
    attributes.color = vertex.color;
}
// 三角形顶点处理结果

// 三角形顶点处理结果
void Renderer::processTriangleVertices(
    const Triangle &triangle,
    std::shared_ptr<IShader> shader,
    std::array<ProcessedVertex, 3> &vertices)
{
    // 预分配栈内存以提高性能
    VertexAttributes attributes;

#pragma omp parallel for if (false) // 三角形很小时禁用并行
    for (int i = 0; i < 3; ++i)
    {
        // 初始化顶点属性
        InitializeVertexAttributes(attributes, triangle.vertices[i]);

        // 顶点着色器处理
        const Vec4f &clipPos = shader->vertexShader(attributes, vertices[i].varying);
        vertices[i].clipPosition = clipPos;

        // 透视除法和屏幕映射 (优化除法操作)
        const float invW = 1.0f / clipPos.w;
        const Vec3f ndcPos(
            clipPos.x * invW,
            clipPos.y * invW,
            clipPos.z * invW);

        vertices[i].screenPosition = screenMapping(ndcPos);
    }
}

// 计算透视校正权重 (SIMD友好版本)
inline Vec4f calculatePerspectiveWeights(
    const Vec3f &barycentric,
    const std::array<ProcessedVertex, 3> &vertices)
{
    // 预计算倒数以避免重复除法
    const Vec3f invW(
        1.0f / vertices[0].clipPosition.w,
        1.0f / vertices[1].clipPosition.w,
        1.0f / vertices[2].clipPosition.w);

    // 单次点乘计算插值的w倒数
    const float interpolated_invW = dot(barycentric, invW);

    return Vec4f(invW.x, invW.y, invW.z, 1.0f / interpolated_invW);
}

// 处理单个片元 - 支持 MSAA
inline void Renderer::processFragment(
    int x, int y,
    float sampleX, float sampleY,
    int sampleIndex,
    const Vec3f &barycentric,
    const Vec4f &weights,
    const std::array<ProcessedVertex, 3> &vertices,
    std::shared_ptr<IShader> shader)
{
    // Early depth testing with vectorized calculation
    const float depth = dot(barycentric * Vec3f(
                                              vertices[0].varying.depth * weights.x,
                                              vertices[1].varying.depth * weights.y,
                                              vertices[2].varying.depth * weights.z),
                            Vec3f(1.0f)) *
                        weights.w;

    // 根据 MSAA 状态选择不同的深度测试
    bool depthTestPassed = msaaEnabled ? 
        frameBuffer->depthTestMSAA(x, y, sampleIndex, depth) : 
        frameBuffer->depthTest(x, y, depth);
        
    if (!depthTestPassed)
        return;

    // Stack-allocated varyings for better cache performance
    Varyings interpolatedVaryings;
    InitializeInterpolateVaryings(
        interpolatedVaryings,
        {vertices[0].varying, vertices[1].varying, vertices[2].varying},
        barycentric, weights, depth);

    // Process fragment and write output in one step
    const FragmentOutput output = shader->fragmentShader(interpolatedVaryings);
    if (!output.discard)
    {
        if (msaaEnabled) {
            frameBuffer->setMSAASample(x, y, sampleIndex, depth, output.color);
        } else {
            frameBuffer->setPixel(x, y, depth, output.color);
        }
    }
}

// 判断三角形是否背面朝向
static bool is_back_facing(const Vec3f &v0, const Vec3f &v1, const Vec3f &v2)
{
    // 计算三角形的有向面积
    float signed_area = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
    // 如果面积小于等于0，则三角形是背面朝向的
    return signed_area <= 0;
}

// 主三角形光栅化函数 - 支持 MSAA
void Renderer::rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader)
{
    if (!shader)
    {
        std::cerr << "Error: No valid shader provided for triangle rendering.\n";
        return;
    }

    // 处理三角形顶点
    std::array<ProcessedVertex, 3> verts;
    processTriangleVertices(triangle, shader, verts);
    
    // 计算三角形区域
    const Vec2f v0(verts[0].screenPosition.xy());
    const Vec2f v1(verts[1].screenPosition.xy());
    const Vec2f v2(verts[2].screenPosition.xy());

    // 计算三角形的面积2倍 如果面积小于0，则三角形是后向的,背面剔除
    const float area2 = cross(v1 - v0, v2 - v0);
    float viewport_sign = (projMatrix.m11 < 0) ? 1.0f : -1.0f; // 视口坐标
    
    if ((area2 * viewport_sign) <= EPSILON) return;
    
    // 计算包围盒,限制包围盒在屏幕范围内
    auto [minX, minY, maxX, maxY] = calculateBoundingBox(
        {verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition},
        frameBuffer->getWidth(), frameBuffer->getHeight());

    // 计算三角形的面积的倒数
    const float invArea2 = 1.0f / area2;

    // 使用 OpenMP 优化的光栅化主循环
    // #pragma omp parallel for collapse(2) if(maxX - minX > 32 || maxY - minY > 32)
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            if (msaaEnabled) {
                // MSAA 模式 - 对每个像素的多个采样点进行处理
                for (int s = 0; s < MSAA_SAMPLES; ++s) {
                    float sampleX = x + MSAA_OFFSETS[s].x;
                    float sampleY = y + MSAA_OFFSETS[s].y;
                    
                    // 计算采样点的重心坐标
                    const Vec3f barycentric = computeBarycentric2D(sampleX, sampleY, 
                        {verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition});
                    
                    // 检查采样点是否在三角形内
                    if (isInsideTriangle(barycentric.x, barycentric.y, barycentric.z)) {
                        const auto weights = calculatePerspectiveWeights(barycentric, verts);
                        processFragment(x, y, sampleX, sampleY, s, barycentric, weights, verts, shader);
                    }
                }
            } else {
                // 非 MSAA 模式 - 对像素中心进行采样
                const float pixelCenterX = x + 0.5f;
                const float pixelCenterY = y + 0.5f;
                
                // 计算像素中心的重心坐标
                const Vec3f barycentric = computeBarycentric2D(pixelCenterX, pixelCenterY, 
                    {verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition});
                
                // 检查像素中心是否在三角形内
                if (isInsideTriangle(barycentric.x, barycentric.y, barycentric.z)) {
                    const auto weights = calculatePerspectiveWeights(barycentric, verts);
                    processFragment(x, y, pixelCenterX, pixelCenterY, 0, barycentric, weights, verts, shader);
                }
            }
        }
    }
}

// 绘制网格 - 更新版本（使用着色器）
void Renderer::drawMesh(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader)
{
    if (!mesh)
        return;
    if (!activeShader)
    {
        std::cerr << "错误：材质没有设置着色器，无法渲染网格。" << std::endl;
        return;
    }

    // 使用预计算的三角形进行渲染
    for (const Triangle &tri : mesh->getTriangles())
    {
        // 直接使用栅格化函数渲染三角形
        rasterizeTriangle(tri, activeShader);
    }
    
    // 如果启用了 MSAA，在所有三角形渲染完成后解析 MSAA 缓冲区
    if (msaaEnabled) {
        frameBuffer->resolveMSAA();
    }
}