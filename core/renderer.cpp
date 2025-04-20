// Renderer.cpp
// 基本光栅化渲染器的实现文件

#include "renderer.h"
#include "mesh.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// FrameBuffer 实现
FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height)
{
    // 初始化帧缓冲区，每个像素4个字节 (RGBA)
    frameData.resize(width * height * 4, 0);
    // 初始化深度缓冲区，每个像素一个浮点数
    depthBuffer.resize(width * height, 1.0f); // 初始化为最远深度(1.0)
}

void FrameBuffer::setPixel(int x, int y, const Color &color)
{
    // 不进行深度测试的版本
    if (!isValidCoord(x, y))
    {
        return;
    }

    // 计算像素在帧缓冲区中的位置
    int index = calcIndex(x, y) * 4;

    // 设置RGBA值
    frameData[index] = color.r;
    frameData[index + 1] = color.g;
    frameData[index + 2] = color.b;
    frameData[index + 3] = color.a;
}

void FrameBuffer::setPixel(int x, int y, float depth, const Color &color)
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
        frameData[colorIndex] = color.r;
        frameData[colorIndex + 1] = color.g;
        frameData[colorIndex + 2] = color.b;
        frameData[colorIndex + 3] = color.a;
    }
}

float FrameBuffer::getDepth(int x, int y) const
{
    if (!isValidCoord(x, y))
    {
        return 1.0f; // 返回最远深度
    }

    return depthBuffer[calcIndex(x, y)];
}

void FrameBuffer::clear(const Color &color, float depth)
{
    // 使用指定颜色和深度值清除整个缓冲区

    // 清除颜色缓冲区
    if (color.a == 255)
    { // 如果颜色是完全不透明的，可以使用更快的填充方法
        std::fill(frameData.begin(), frameData.end(), 0);
        for (size_t i = 0; i < frameData.size(); i += 4)
        {
            frameData[i] = color.r;
            frameData[i + 1] = color.g;
            frameData[i + 2] = color.b;
            frameData[i + 3] = color.a;
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

// Renderer 实现
// 更新构造函数以初始化变换矩阵
// 更新构造函数以初始化变换矩阵和光照
// 更新构造函数以初始化着色器
// Renderer 构造函数
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      shader(nullptr)  // 初始化着色器为空
{
    // 初始化默认光源
    this->light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
}

void Renderer::clear(const Color &color)
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
Vec3f Renderer::screenMapping(const Vec3f& clipPos) {
    // 屏幕映射 - 变换到屏幕坐标系
    float screenX = (clipPos.x + 1.0f) * 0.5f * frameBuffer->getWidth();
    float screenY = (1.0f - clipPos.y) * 0.5f * frameBuffer->getHeight(); // 注意Y轴翻转
    
    // z值保持不变(用于深度缓冲)
    return Vec3f(screenX, screenY, clipPos.z);
}

// 更新的三角形栅格化函数（使用外部提供的着色器）
void Renderer::rasterizeTriangle(const Triangle& triangle, std::shared_ptr<IShader> shader) {
    // 如果没有有效的着色器，则直接返回
    if (!shader) {
        std::cerr << "错误：未提供有效的着色器，无法渲染三角形。" << std::endl;
        return;
    }
    
    // 对每个顶点运行顶点着色器
    std::array<Vec3f, 3> clipPositions;
    std::array<Vec3f, 3> screenPositions;
    std::array<Varyings, 3> varyings;
    
    for (int i = 0; i < 3; ++i) {
        // 创建顶点属性
        VertexAttributes attributes;
        attributes.position = triangle.vertices[i].position;
        attributes.normal = triangle.vertices[i].normal;
        attributes.texCoord = triangle.vertices[i].texCoord;
        attributes.color = triangle.vertices[i].color;
        
        // 运行顶点着色器
        clipPositions[i] = shader->vertexShader(attributes, varyings[i]);
        
        // 屏幕映射
        screenPositions[i] = screenMapping(clipPositions[i]);
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
    
    // 获取顶点的屏幕坐标
    float x0 = screenPositions[0].x;
    float y0 = screenPositions[0].y;
    float x1 = screenPositions[1].x;
    float y1 = screenPositions[1].y;
    float x2 = screenPositions[2].x;
    float y2 = screenPositions[2].y;
    
    // 计算面积的两倍（叉积）
    float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    
    // 如果面积为0，则三角形是一条线或一个点
    if (std::abs(area) < 1e-6) {
        return;
    }
    
    // 透视校正插值需要的深度倒数
    float w0 = 1.0f / varyings[0].depth;
    float w1 = 1.0f / varyings[1].depth;
    float w2 = 1.0f / varyings[2].depth;
    
    // 遍历包围盒中的每个像素
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            // 计算重心坐标
            float lambda0 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            float lambda1 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            float lambda2 = 1.0f - lambda0 - lambda1;
            
            // 检查点是否在三角形内
            if (lambda0 >= 0 && lambda1 >= 0 && lambda2 >= 0) {
                // 透视校正插值
                float interpolated_w_inverse = lambda0 * w0 + lambda1 * w1 + lambda2 * w2;
                float w_correct = 1.0f / interpolated_w_inverse;
                
                // 创建插值后的 Varyings 结构
                Varyings interpolatedVaryings;
                
                // 透视校正插值位置
                interpolatedVaryings.position.x = (lambda0 * varyings[0].position.x * w0 + 
                                                lambda1 * varyings[1].position.x * w1 + 
                                                lambda2 * varyings[2].position.x * w2) * w_correct;
                interpolatedVaryings.position.y = (lambda0 * varyings[0].position.y * w0 + 
                                                lambda1 * varyings[1].position.y * w1 + 
                                                lambda2 * varyings[2].position.y * w2) * w_correct;
                interpolatedVaryings.position.z = (lambda0 * varyings[0].position.z * w0 + 
                                                lambda1 * varyings[1].position.z * w1 + 
                                                lambda2 * varyings[2].position.z * w2) * w_correct;
                
                // 透视校正插值法线
                interpolatedVaryings.normal.x = (lambda0 * varyings[0].normal.x * w0 + 
                                              lambda1 * varyings[1].normal.x * w1 + 
                                              lambda2 * varyings[2].normal.x * w2) * w_correct;
                interpolatedVaryings.normal.y = (lambda0 * varyings[0].normal.y * w0 + 
                                              lambda1 * varyings[1].normal.y * w1 + 
                                              lambda2 * varyings[2].normal.y * w2) * w_correct;
                interpolatedVaryings.normal.z = (lambda0 * varyings[0].normal.z * w0 + 
                                              lambda1 * varyings[1].normal.z * w1 + 
                                              lambda2 * varyings[2].normal.z * w2) * w_correct;
                
                // 归一化插值后的法线
                interpolatedVaryings.normal = normalize(interpolatedVaryings.normal);
                
                // 透视校正插值纹理坐标
                interpolatedVaryings.texCoord.x = (lambda0 * varyings[0].texCoord.x * w0 + 
                                                lambda1 * varyings[1].texCoord.x * w1 + 
                                                lambda2 * varyings[2].texCoord.x * w2) * w_correct;
                interpolatedVaryings.texCoord.y = (lambda0 * varyings[0].texCoord.y * w0 + 
                                                lambda1 * varyings[1].texCoord.y * w1 + 
                                                lambda2 * varyings[2].texCoord.y * w2) * w_correct;
                
                // 透视校正插值颜色
                interpolatedVaryings.color.r = static_cast<uint8_t>((lambda0 * varyings[0].color.r * w0 + 
                                                         lambda1 * varyings[1].color.r * w1 + 
                                                         lambda2 * varyings[2].color.r * w2) * w_correct);
                interpolatedVaryings.color.g = static_cast<uint8_t>((lambda0 * varyings[0].color.g * w0 + 
                                                         lambda1 * varyings[1].color.g * w1 + 
                                                         lambda2 * varyings[2].color.g * w2) * w_correct);
                interpolatedVaryings.color.b = static_cast<uint8_t>((lambda0 * varyings[0].color.b * w0 + 
                                                         lambda1 * varyings[1].color.b * w1 + 
                                                         lambda2 * varyings[2].color.b * w2) * w_correct);
                interpolatedVaryings.color.a = static_cast<uint8_t>((lambda0 * varyings[0].color.a * w0 + 
                                                         lambda1 * varyings[1].color.a * w1 + 
                                                         lambda2 * varyings[2].color.a * w2) * w_correct);
                
                // 插值深度
                interpolatedVaryings.depth = (lambda0 * varyings[0].depth * w0 + 
                                            lambda1 * varyings[1].depth * w1 + 
                                            lambda2 * varyings[2].depth * w2) * w_correct;
                
                // 运行片段着色器
                FragmentOutput fragmentOutput = shader->fragmentShader(interpolatedVaryings);
                
                // 设置像素颜色（包含深度测试）
                frameBuffer->setPixel(x, y, interpolatedVaryings.depth, fragmentOutput.color);
            }
        }
    }
}

// 绘制网格 - 更新版本（使用着色器）
// 绘制网格 - 完全重写版本（使用Mesh自身的着色器）
// 重新实现的 drawMesh 方法，不再依赖 Mesh::draw
void Renderer::drawMesh(const std::shared_ptr<Mesh> &mesh)
{
    if (!mesh) {
        return;
    }

    // 检查是否有着色器设置
    std::shared_ptr<IShader> activeShader = mesh->getShader() ? mesh->getShader() : shader;
    
    if (!activeShader) {
        std::cerr << "错误：没有设置着色器，无法渲染网格。" << std::endl;
        return;
    }
    
    // 设置着色器的统一变量
    ShaderUniforms uniforms;
    uniforms.modelMatrix = modelMatrix;
    uniforms.viewMatrix = viewMatrix;
    uniforms.projMatrix = projMatrix;
    uniforms.mvpMatrix = getMVPMatrix();
    uniforms.eyePosition = eyePosWS;
    uniforms.light = light;
    uniforms.material = mesh->getMaterial();
    
    activeShader->setUniforms(uniforms);
    
    // 处理每个面
    for (const Face& face : mesh->faces) {
        // 将面转换为一个或多个三角形
        std::vector<Triangle> triangles = mesh->triangulate(face);
        
        // 处理每个三角形
        for (const Triangle& tri : triangles) {
            // 直接使用栅格化函数渲染三角形
            rasterizeTriangle(tri, activeShader);
        }
    }
}


// 渲染一个场景，包含多个对象
void renderScene(Renderer &renderer, const std::vector<SceneObject> &objects)
{
    // 清除屏幕
    renderer.clear(Color(40, 40, 60)); // 深蓝灰色背景

    // 绘制每个对象
    for (const auto &object : objects)
    {
        // 设置模型矩阵
        renderer.setModelMatrix(object.modelMatrix);

        // 绘制网格
        renderer.drawMesh(object.mesh);
    }
}