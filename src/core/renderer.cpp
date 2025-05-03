/**
 * @file renderer_application.cpp
 * @brief 渲染器应用阶段实现，处理初始化、设置和高级渲染控制
 */

#include "maths.h"
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h"
#include <omp.h>

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

// 定义MSAA采样点偏移
const Vec2f Renderer::msaaSampleOffsets[4] = {
    {0.25f, 0.25f}, {0.75f, 0.25f},
    {0.25f, 0.75f}, {0.75f, 0.75f}
};
