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
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      lightingEnabled(false)
{
    // 初始化默认光源
    this->light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
}

void Renderer::clear(const Color &color)
{
    frameBuffer->clear(color);
}
// 绘制经过MVP变换的三角形
void Renderer::drawTriangle(const Triangle &triangle, const Matrix4x4f &mvpMatrix)
{
    // 创建变换后的三角形
    Triangle transformedTriangle;

    // 变换每个顶点
    for (int i = 0; i < 3; ++i)
    {
        Vertex &v = transformedTriangle.vertices[i];
        v = triangle.vertices[i]; // 复制顶点其他属性

        // 变换位置
        v.position = transformVertex(triangle.vertices[i].position, mvpMatrix);
    }

    // 使用原始函数绘制变换后的三角形
    drawTriangle(transformedTriangle);
}



void Renderer::drawTriangle(const Triangle &triangle) {
    // 提取三角形的三个顶点
    const Vertex &v1 = triangle.vertices[0];
    const Vertex &v2 = triangle.vertices[1];
    const Vertex &v3 = triangle.vertices[2];
    
    // 找到三角形的包围盒
    int minX = static_cast<int>(std::min({v1.position.x, v2.position.x, v3.position.x}));
    int minY = static_cast<int>(std::min({v1.position.y, v2.position.y, v3.position.y}));
    int maxX = static_cast<int>(std::ceil(std::max({v1.position.x, v2.position.x, v3.position.x})));
    int maxY = static_cast<int>(std::ceil(std::max({v1.position.y, v2.position.y, v3.position.y})));
    
    // 裁剪到屏幕范围
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(frameBuffer->getWidth() - 1, maxX);
    maxY = std::min(frameBuffer->getHeight() - 1, maxY);
    
    // 获取顶点的屏幕坐标
    float x0 = v1.position.x;
    float y0 = v1.position.y;
    float x1 = v2.position.x;
    float y1 = v2.position.y;
    float x2 = v3.position.x;
    float y2 = v3.position.y;
    
    // 获取顶点的z坐标
    float z0 = v1.position.z;
    float z1 = v2.position.z;
    float z2 = v3.position.z;
    
    // 透视正确插值需要顶点的w值（通常是1/z）
    float w0 = 1.0f / v1.position.z; 
    float w1 = 1.0f / v2.position.z;
    float w2 = 1.0f / v3.position.z;
    
    // 预乘顶点颜色与w
    Color c0w(
        static_cast<uint8_t>(v1.color.r * w0),
        static_cast<uint8_t>(v1.color.g * w0),
        static_cast<uint8_t>(v1.color.b * w0),
        static_cast<uint8_t>(v1.color.a * w0)
    );
    
    Color c1w(
        static_cast<uint8_t>(v2.color.r * w1),
        static_cast<uint8_t>(v2.color.g * w1),
        static_cast<uint8_t>(v2.color.b * w1),
        static_cast<uint8_t>(v2.color.a * w1)
    );
    
    Color c2w(
        static_cast<uint8_t>(v3.color.r * w2),
        static_cast<uint8_t>(v3.color.g * w2),
        static_cast<uint8_t>(v3.color.b * w2),
        static_cast<uint8_t>(v3.color.a * w2)
    );
    
    // 计算面积的两倍（叉积）
    float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    
    // 如果面积为0，则三角形是一条线或一个点
    if (std::abs(area) < 1e-6) {
        return;
    }
    
    // 遍历包围盒中的每个像素
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            // 计算重心坐标
            float lambda0 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            float lambda1 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            float lambda2 = 1.0f - lambda0 - lambda1;
            
            // 检查点是否在三角形内
            if (lambda0 >= 0 && lambda1 >= 0 && lambda2 >= 0) {
                // 插值1/w
                float interpolated_w_inverse = lambda0 * w0 + lambda1 * w1 + lambda2 * w2;
                
                // 使用透视正确插值计算深度
                float depth = (lambda0 * z0 * w0 + lambda1 * z1 * w1 + lambda2 * z2 * w2) / interpolated_w_inverse;
                
                // 透视正确插值顶点颜色
                Color interpolated_color(
                    static_cast<uint8_t>((lambda0 * c0w.r + lambda1 * c1w.r + lambda2 * c2w.r) / interpolated_w_inverse),
                    static_cast<uint8_t>((lambda0 * c0w.g + lambda1 * c1w.g + lambda2 * c2w.g) / interpolated_w_inverse),
                    static_cast<uint8_t>((lambda0 * c0w.b + lambda1 * c1w.b + lambda2 * c2w.b) / interpolated_w_inverse),
                    static_cast<uint8_t>((lambda0 * c0w.a + lambda1 * c1w.a + lambda2 * c2w.a) / interpolated_w_inverse)
                );
                
                // 设置像素颜色（包含深度测试）
                frameBuffer->setPixel(x, y, depth, interpolated_color);
            }
        }
    }
}

// 顶点变换 - MVP变换和屏幕映射
Vec3f Renderer::transformVertex(const Vec3f &position, const Matrix4x4f &mvpMatrix)
{
    // 应用MVP矩阵变换
    Vec3f clipPos = transform(mvpMatrix, position);

    // 屏幕映射 - 变换到屏幕坐标系
    float screenX = (clipPos.x + 1.0f) * 0.5f * frameBuffer->getWidth();
    float screenY = (1.0f - clipPos.y) * 0.5f * frameBuffer->getHeight(); // 注意Y轴翻转

    // z值保持不变(用于深度缓冲)
    return Vec3f(screenX, screenY, clipPos.z);
}


// 计算光照颜色
Color Renderer::calculateLighting(const Vertex &vertex, const Material &material, const Vec3f &eyePos)
{
    if (!lightingEnabled)
    {
        return vertex.color; // 如果未启用光照，直接返回顶点颜色
    }

    // 计算法向量（确保已归一化）
    float3 normal = normalize(vertex.normal);

    // 计算从顶点到光源的方向向量（确保已归一化）
    float3 lightDir = normalize(light.position - vertex.position);

    // 计算从顶点到观察者的方向向量（确保已归一化）
    float3 viewDir = normalize(eyePos - vertex.position);

    // 计算半程向量
    float3 halfwayDir = normalize(lightDir + viewDir);

    // 计算环境光分量
    float3 ambient = material.ambient * light.color * light.ambientIntensity;

    // 计算漫反射分量（考虑光照角度）
    float diff = std::max(dot(normal, lightDir), 0.0f);
    float3 diffuse = material.diffuse * light.color * (diff * light.intensity);

    // 计算镜面反射分量（考虑视角）
    float spec = std::pow(std::max(dot(normal, halfwayDir), 0.0f), material.shininess);
    float3 specular = material.specular * light.color * spec * light.intensity;
    

    // 合并所有光照分量
    // float3 result =ambient +diffuse+specular;
    float resultR = ambient.x + diffuse.x + specular.x;
    float resultG = ambient.y + diffuse.y + specular.y;
    float resultB = ambient.z + diffuse.z + specular.z;

    // 确保结果在 [0,1] 范围内
    resultR = std::min(resultR, 1.0f);
    resultG = std::min(resultG, 1.0f);
    resultB = std::min(resultB, 1.0f);

    // 结合原始顶点颜色
    float vertexColorFactor = 0.5f; // 可调整原始顶点颜色的影响因子
    float r = resultR * (vertex.color.r / 255.0f) * vertexColorFactor + resultR * (1.0f - vertexColorFactor);
    float g = resultG * (vertex.color.g / 255.0f) * vertexColorFactor + resultG * (1.0f - vertexColorFactor);
    float b = resultB * (vertex.color.b / 255.0f) * vertexColorFactor + resultB * (1.0f - vertexColorFactor);

    // 将 [0,1] 范围的浮点值转换为 [0,255] 范围的整数
    Color finalColor(
        static_cast<uint8_t>(r * 255.0f),
        static_cast<uint8_t>(g * 255.0f),
        static_cast<uint8_t>(b * 255.0f),
        vertex.color.a);

    return finalColor;
}

// 绘制Mesh的实现
void Renderer::drawMesh(const std::shared_ptr<Mesh> &mesh)
{
    if (!mesh)
    {
        return;
    }
    // 调用Mesh的绘制方法
    mesh->draw(*this);
}

// 保存深度图到PPM文件
void Renderer::saveDepthMap(const std::string &filename, float nearPlane, float farPlane)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "无法创建文件：" << filename << std::endl;
        return;
    }

    int width = frameBuffer->getWidth();
    int height = frameBuffer->getHeight();

    // 写入PPM文件头
    file << "P6\n"
         << width << " " << height << "\n255\n";

    // 准备临时缓冲区
    std::vector<uint8_t> depthPixels(width * height * 3);

    // 为了可视化，我们将深度值映射到灰度值
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = frameBuffer->getDepth(x, y);

            // 将深度值从 [nearPlane, farPlane] 映射到 [0, 1]
            float normalizedDepth = (depth - nearPlane) / (farPlane - nearPlane);
            normalizedDepth = std::max(0.0f, std::min(1.0f, normalizedDepth));

            // 将归一化的深度值转换为灰度值
            uint8_t grayValue = static_cast<uint8_t>((1.0f - normalizedDepth) * 255.0f);

            // 设置RGB值（相同的灰度值）
            int index = (y * width + x) * 3;
            depthPixels[index] = grayValue;     // R
            depthPixels[index + 1] = grayValue; // G
            depthPixels[index + 2] = grayValue; // B
        }
    }

    // 写入像素数据
    file.write(reinterpret_cast<const char *>(depthPixels.data()), depthPixels.size());

    std::cout << "深度图已保存到 " << filename << std::endl;
}