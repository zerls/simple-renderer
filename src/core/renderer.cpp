// 基本光栅化渲染器的实现文件
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h" // 替换 test.h

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
    // 带深度测试的版本
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
// TODO 阴影渲染 Bias没有实现，ShadowMap使用uint8 存储，精度问题很大
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
            // TODO 优化存储精度问题 float
            shadowMap->write(x, y, Vec4f(depth));
            // shadowMap->data[index] = static_cast<uint8_t>(depth * 255.0f);
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
}

void Renderer::clear(const Vec4f &color)
{
    frameBuffer->clear(color);
}

// 顶点变换 - MVP变换和屏幕映射
Vec3f Renderer::transformVertex(const Vec3f &position, const Matrix4x4f &mvpMatrix)
{
    // 应用MVP矩阵变换
    return transform(mvpMatrix, position);
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

// 更新的三角形栅格化函数（使用外部提供的着色器）
void Renderer::rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader)
{
    // 如果没有有效的着色器，则直接返回
    if (!shader)
    {
        std::cerr << "错误：未提供有效的着色器，无法渲染三角形。" << std::endl;
        return;
    }

    // 对每个顶点运行顶点着色器
    std::array<Vec3f, 3> clipPositions;
    std::array<Vec3f, 3> screenPositions;
    std::array<Varyings, 3> varyings;

    for (int i = 0; i < 3; ++i)
    {
        // 创建顶点属性
        VertexAttributes attributes;
        attributes.position = triangle.vertices[i].position;
        attributes.normal = triangle.vertices[i].normal;
        attributes.tangent = triangle.vertices[i].tangent;
        attributes.texCoord = triangle.vertices[i].texCoord;
        attributes.color = triangle.vertices[i].color;

        // 运行顶点着色器
        clipPositions[i] = shader->vertexShader(attributes, varyings[i]);

        //TODO  透视除法
        // Vec3f ndcPos =  Vec3f(clipPositions[i].x/ clipPositions[i].w,
        //                       clipPositions[i].y/ clipPositions[i].w,
        //                       clipPositions[i].z/ clipPositions[i].w);
                              
                              Vec3f ndcPos =  Vec3f(clipPositions[i].x,
                                clipPositions[i].y,
                                clipPositions[i].z);       
                            

        // 屏幕映射
        screenPositions[i] = screenMapping(ndcPos);
    }

    // 找到三角形的包围盒
    int minX = static_cast<int>(std::min({screenPositions[0].x, screenPositions[1].x, screenPositions[2].x}));
    int minY = static_cast<int>(std::min({screenPositions[0].y, screenPositions[1].y, screenPositions[2].y}));
    int maxX = static_cast<int>(std::ceil(std::max({screenPositions[0].x, screenPositions[1].x, screenPositions[2].x})));
    int maxY = static_cast<int>(std::ceil(std::max({screenPositions[0].y, screenPositions[1].y, screenPositions[2].y})));

    // 裁剪到屏幕范围
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(frameBuffer->getWidth() - 1, maxX);
    maxY = std::min(frameBuffer->getHeight() - 1, maxY);


    
    Vec2f v0(screenPositions[0].x, screenPositions[0].y);
    Vec2f v1(screenPositions[1].x, screenPositions[1].y);
    Vec2f v2(screenPositions[2].x, screenPositions[2].y);
// 叉积面积
    float area = cross(v1 - v0, v2 - v0);
        // 如果面积为0，则三角形是一条线或一个点
    if (std::abs(area) < 1e-8)
    {
        return;
    }

    // 透视校正插值需要的深度倒数
    // float w0 = 1.0f / clipPositions[0].w;
    // float w1 = 1.0f / clipPositions[1].w;
    // float w2 = 1.0f / clipPositions[2].w;
    float w0 = 1.0f / clipPositions[0].z;
    float w1 = 1.0f / clipPositions[1].z;
    float w2 = 1.0f / clipPositions[2].z;
    Vec3f w = Vec3f(w0, w1, w2);

    // 遍历包围盒中的每个像素
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            Vec2f p(x, y); // 当前像素位置
            
            // 计算重心坐标
            // lambda0对应v0的权重
            float lambda0 = cross(v1 - p, v2 - p) / area;
            // lambda1对应v1的权重
            float lambda1 = cross(v2 - p, v0 - p) / area;
            // lambda2对应v2的权重
            float lambda2 = 1.0f - lambda0 - lambda1;

            // 计算重心坐标
            // float lambda0 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            // float lambda1 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            // float lambda2 = 1.0f - lambda0 - lambda1;
            Vec3f lambda = Vec3f(lambda0, lambda1, lambda2);

            // 检查点是否在三角形内
            if (lambda0 >= 0 && lambda1 >= 0 && lambda2 >= 0)
            {
                // 透视校正插值
                float interpolated_w_inverse = lambda0 * w0 + lambda1 * w1 + lambda2 * w2;
                float w_correct = 1.0f / interpolated_w_inverse;

                // 插值深度
                float depth = (lambda0 * varyings[0].depth * w0 +
                               lambda1 * varyings[1].depth * w1 +
                               lambda2 * varyings[2].depth * w2) *
                              w_correct;

                // 提前深度测试 - 如果深度测试失败，跳过片元处理
                if (!frameBuffer->depthTest(x, y, depth))
                {
                    continue;
                }

                // 进行顶点属性的插值
                Varyings interpolatedVaryings;

                interpolatedVaryings.position = interpolatePerspectiveCorrect(
                    varyings[0].position, varyings[1].position, varyings[2].position,
                    lambda, w, w_correct);

                interpolatedVaryings.normal = interpolatePerspectiveCorrect(
                    varyings[0].normal, varyings[1].normal, varyings[2].normal,
                    lambda, w, w_correct);

                interpolatedVaryings.normal = normalize(interpolatedVaryings.normal);

                interpolatedVaryings.tangent = interpolatePerspectiveCorrect(
                    varyings[0].tangent, varyings[1].tangent, varyings[2].tangent,
                    lambda, w, w_correct);

                interpolatedVaryings.tangent.w = varyings[0].tangent.w;

                interpolatedVaryings.texCoord = interpolatePerspectiveCorrect(
                    varyings[0].texCoord, varyings[1].texCoord, varyings[2].texCoord,
                    lambda, w, w_correct);
                // interpolatedVaryings.texCoord =varyings[0].texCoord;
                // 透视校正插值颜色
                interpolatedVaryings.color = interpolatePerspectiveCorrect(
                    varyings[0].color, varyings[1].color, varyings[2].color,
                    lambda, w, w_correct);

                interpolatedVaryings.depth = depth;

                // 插值光源空间位置（用于阴影映射）
                if (varyings[0].positionLightSpace.w != 0)
                {
                    interpolatedVaryings.positionLightSpace = interpolatePerspectiveCorrect(
                        varyings[0].positionLightSpace, varyings[1].positionLightSpace, varyings[2].positionLightSpace,
                        lambda, w, w_correct);
                }

                // 运行片段着色器
                FragmentOutput fragmentOutput = shader->fragmentShader(interpolatedVaryings);

                // 如果片元被丢弃，跳过后续处理
                if (fragmentOutput.discard)
                {
                    continue;
                }

                // 设置像素颜色
                frameBuffer->setPixel(x, y, depth, fragmentOutput.color);
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