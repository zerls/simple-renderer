// 基本光栅化渲染器的实现文件
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h" // 替换 test.h
#include <omp.h>

// FrameBuffer 实现
FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height)
{
    // 初始化帧缓冲区，每个像素4个字节 (RGBA)
    frameData.resize(width * height * 4, 0);
    // 初始化深度缓冲区，每个像素一个浮点数
    depthBuffer.resize(width * height, 1.0f); // 初始化为最远深度(1.0)
}

// 修改setPixel方法
void FrameBuffer::setPixel(int x, int y, float depth, const Vec4f &color)
{
    // 带深度测试的版本
    if (!isValidCoord(x, y))
    {
        return;
    }

    int index = calcIndex(x, y);

    // 深度测试：只有当新深度值小于当前深度值时（更靠近摄像机）才更新
    if (depth < depthBuffer[index])
    {
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

    auto originalFrameBuffer = std::move(frameBuffer); // 保存当前渲染状态 ｜ShadowBuffer 与 FrameBuffer 的尺寸不一致

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
    shadowFrameBuffer = std::move(frameBuffer);
    frameBuffer = std::move(originalFrameBuffer);
}

// Renderer 构造函数
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      shader(nullptr) // 初始化着色器为空
{
    // 初始化默认光源
    this->light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);

    // 设置OpenMP线程数，只需要设置一次
    omp_set_num_threads(4);
}

void Renderer::clear(const Vec4f &color)
{
    frameBuffer->clear(color);
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

// Helper function to interpolate varyings with perspective correction
/**
 * @brief Performs perspective-correct interpolation of vertex attributes for a fragment
 *
 * This function interpolates various vertex attributes (position, texCoords, normals, etc.)
 * using perspective-correct interpolation based on barycentric coordinates and perspective weights.
 *
 * @param interpolated Output parameter containing the interpolated vertex attributes
 * @param v Array of three Varyings containing the vertex attributes of the triangle
 * @param b Barycentric coordinates (b.x, b.y, b.z) for interpolation
 * @param w Perspective weights for correct interpolation
 *         w.xyz(): Individual perspective weights for each vertex
 *         w.w: Correction factor for perspective-correct interpolation
 * @param fragmentDepth The depth value of the current fragment
 *
 * @details
 * The interpolation is performed using the formula:
 * result = (v0 * w0 * b0 + v1 * w1 * b1 + v2 * w2 * b2) * wCorrection
 * where:
 * - v0, v1, v2: vertex attributes
 * - w0, w1, w2: perspective weights
 * - b0, b1, b2: barycentric coordinates
 * - wCorrection: perspective correction factor
 *
 * Special handling is applied for:
 * - Normals: normalized after interpolation
 * - Tangents: w component preserved from first vertex
 * - Light space positions: only interpolated if available (w != 0)
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

// 处理单个片元
inline void Renderer::processFragment(
    int x, int y,
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

    if (!frameBuffer->depthTest(x, y, depth))
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
        frameBuffer->setPixel(x, y, depth, output.color);
    }
}
static bool is_back_facing(const Vec3f &v0, const Vec3f &v1, const Vec3f &v2)
{
    // 计算三角形的面积
    float signed_area = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y); // AB AC
    // 如果面积小于0，则三角形是后向的
    return signed_area <= 0;
}
// 主三角形光栅化函数
/**
 * @brief Rasterizes a triangle and processes its fragments using the provided shader.
 *
 * This function performs the rasterization of a triangle defined by its vertices and
 * processes each fragment using the provided shader. It handles perspective correction,
 * depth testing, and writes the final color to the framebuffer.
 *
 * @param triangle The triangle to be rasterized.
 * @param shader The shader used for processing fragments.
 */
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
    const float area2 = cross(v1 - v0, v2 - v1);               // AB BC
    float viewport_sign = (projMatrix.m11 < 0) ? 1.0f : -1.0f; // 视口坐标
    
    if ((area2 * viewport_sign) <= 1e-8f) return;
        

    // 计算包围盒,限制包围盒在屏幕范围内
    auto [minX, minY, maxX, maxY] = calculateBoundingBox(
        {verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition},
        frameBuffer->getWidth(), frameBuffer->getHeight());

    // 使用 OpenMP 优化的光栅化主循环

       // 计算三角形的面积的倒数
       const float invArea2 = 1.0f / area2;

    // 注意：不要在这里设置线程数，应该在程序初始化时设置一次
    // #pragma omp parallel for schedule(dynamic, 16)
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const Vec2f p(x, y);
            const float alpha = cross(v1 - p, v2 - p) * invArea2;
            const float beta = cross(v2 - p, v0 - p) * invArea2;
            const float gamma = 1.0f - alpha - beta;

            // Early exit if any barycentric coordinate is negative (outside the triangle)
            if (alpha < 0 || beta < 0 || gamma < 0)
            {
                continue;
            }

            if (alpha >= 0 && beta >= 0 && gamma >= 0)
            {
                // 计算重心坐标
                const Vec3f barycentric(alpha, beta, gamma);
                const auto weights = calculatePerspectiveWeights(barycentric, verts);

                processFragment(x, y, barycentric, weights, verts, shader);
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
}