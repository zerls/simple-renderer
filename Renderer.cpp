// Renderer.cpp
// 基本光栅化渲染器的实现文件

#include "renderer.h"
#include <algorithm>
#include <cmath>

// 矩阵-向量变换函数
Vec3 transform(const Matrix4x4& matrix, const Vec3& vector, float w = 1.0f) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    float wOut = vector.x * matrix.m30 + vector.y * matrix.m31 + vector.z * matrix.m32 + w * matrix.m33;
    
    // 透视除法
    if (std::abs(wOut) > 1e-6f) {
        float invW = 1.0f / wOut;
        return Vec3(x * invW, y * invW, z * invW);
    }
    
    return Vec3(x, y, z);
}

// 仅进行矩阵-向量乘法，不进行透视除法
Vec3 transformNoDiv(const Matrix4x4& matrix, const Vec3& vector, float w = 1.0f) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    
    return Vec3(x, y, z);
}

// 变换法线向量 (使用法线矩阵: model矩阵的逆转置)
Vec3 transformNormal(const Matrix4x4 &modelMatrix, const Vec3 &normal)
{
    // 简化实现: 假设模型矩阵只有旋转和均匀缩放，可以直接使用模型矩阵
    // 完整实现应该使用模型矩阵的逆转置矩阵
    Vec3 result;
    result.x = normal.x * modelMatrix.m00 + normal.y * modelMatrix.m01 + normal.z * modelMatrix.m02;
    result.y = normal.x * modelMatrix.m10 + normal.y * modelMatrix.m11 + normal.z * modelMatrix.m12;
    result.z = normal.x * modelMatrix.m20 + normal.y * modelMatrix.m21 + normal.z * modelMatrix.m22;
    
    return normalize(result);
}

// FrameBuffer 实现
FrameBuffer::FrameBuffer(int width, int height)
    : width(width), height(height) {
    // 初始化帧缓冲区，每个像素4个字节 (RGBA)
    frameData.resize(width * height * 4, 0);
    // 初始化深度缓冲区，每个像素一个浮点数
    depthBuffer.resize(width * height, 1.0f); // 初始化为最远深度(1.0)
}

void FrameBuffer::setPixel(int x, int y, const Color& color) {
    // 不进行深度测试的版本
    if (!isValidCoord(x, y)) {
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

void FrameBuffer::setPixel(int x, int y, float depth, const Color& color) {
    // 带深度测试的版本
    if (!isValidCoord(x, y)) {
        return;
    }
    
    int index = calcIndex(x, y);
    
    // 深度测试：只有当新深度值小于当前深度值时（更靠近摄像机）才更新
    if (depth < depthBuffer[index]) {
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

float FrameBuffer::getDepth(int x, int y) const {
    if (!isValidCoord(x, y)) {
        return 1.0f; // 返回最远深度
    }
    
    return depthBuffer[calcIndex(x, y)];
}

void FrameBuffer::clear(const Color& color, float depth) {
    // 使用指定颜色和深度值清除整个缓冲区
    
    // 清除颜色缓冲区
    if (color.a == 255) { // 如果颜色是完全不透明的，可以使用更快的填充方法
        std::fill(frameData.begin(), frameData.end(), 0);
        for (size_t i = 0; i < frameData.size(); i += 4) {
            frameData[i] = color.r;
            frameData[i + 1] = color.g;
            frameData[i + 2] = color.b;
            frameData[i + 3] = color.a;
        }
    } else {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
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
      modelMatrix(Matrix4x4::identity()),
      viewMatrix(Matrix4x4::identity()),
      projMatrix(Matrix4x4::identity()),
      lightingEnabled(false)
{
    // 初始化默认光源
    this->light = Light(Vec3(0.0f, 0.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
}

void Renderer::clear(const Color& color) {
    frameBuffer->clear(color);
}
// 绘制经过MVP变换的三角形
void Renderer::drawTriangle(const Triangle& triangle, const Matrix4x4& mvpMatrix) {
    // 创建变换后的三角形
    Triangle transformedTriangle;
    
    // 变换每个顶点
    for (int i = 0; i < 3; ++i) {
        Vertex& v = transformedTriangle.vertices[i];
        v = triangle.vertices[i]; // 复制顶点其他属性
        
        // 变换位置
        v.position = transformVertex(triangle.vertices[i].position, mvpMatrix);
    }
    
    // 使用原始函数绘制变换后的三角形
    drawTriangle(transformedTriangle);
}



// 绘制三角形的实现（带深度缓冲）
void Renderer::drawTriangle(const Triangle& triangle) {
    // 提取三角形的三个顶点
    const Vertex& v1 = triangle.vertices[0];
    const Vertex& v2 = triangle.vertices[1];
    const Vertex& v3 = triangle.vertices[2];
    
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
    
    // 获取顶点的 z 坐标
    float z0 = v1.position.z;
    float z1 = v2.position.z;
    float z2 = v3.position.z;
    
    // 获取顶点的屏幕坐标
    float x0 = v1.position.x;
    float y0 = v1.position.y;
    float x1 = v2.position.x;
    float y1 = v2.position.y;
    float x2 = v3.position.x;
    float y2 = v3.position.y;
    
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
            float w0 = ((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            float w1 = ((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            float w2 = 1.0f - w0 - w1;
            
            // 检查点是否在三角形内
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // 使用重心坐标插值深度
                float depth = w0 * z0 + w1 * z1 + w2 * z2;
                
                // 使用重心坐标插值顶点颜色
                Color color(
                    static_cast<uint8_t>(w0 * v1.color.r + w1 * v2.color.r + w2 * v3.color.r),
                    static_cast<uint8_t>(w0 * v1.color.g + w1 * v2.color.g + w2 * v3.color.g),
                    static_cast<uint8_t>(w0 * v1.color.b + w1 * v2.color.b + w2 * v3.color.b),
                    static_cast<uint8_t>(w0 * v1.color.a + w1 * v2.color.a + w2 * v3.color.a)
                );
                
                // 设置像素颜色（包含深度测试）
                frameBuffer->setPixel(x, y, depth, color);
            }
        }
    }


}
    // 顶点变换 - MVP变换和屏幕映射
    Vec3 Renderer::transformVertex(const Vec3& position, const Matrix4x4& mvpMatrix) {
        // 应用MVP矩阵变换
        Vec3 clipPos = transform(mvpMatrix, position);
        
        // 屏幕映射 - 变换到屏幕坐标系
        float screenX = (clipPos.x + 1.0f) * 0.5f * frameBuffer->getWidth();
        float screenY = (1.0f - clipPos.y) * 0.5f * frameBuffer->getHeight(); // 注意Y轴翻转
        
        // z值保持不变(用于深度缓冲)
        return Vec3(screenX, screenY, clipPos.z);
    }
// Renderer.cpp中实现:
void Renderer::drawCube(const Cube& cube) {
    // 获取当前的MVP矩阵
    Matrix4x4 mvpMatrix = getMVPMatrix();
    Vec3 eyePos = Vec3(0, 0, 0); // 在观察空间中，摄像机位于原点
    
    // 绘制立方体的12个三角形
    for (int face = 0; face < 6; ++face) {
        // 获取当前面的法线
        Vec3 faceNormal = cube.faceNormals[face];
        // 变换法线（使用模型矩阵）
        Vec3 transformedNormal = transformNormal(modelMatrix, faceNormal);
        
        // 每个面有2个三角形
        for (int i = 0; i < 2; ++i) {
            int triangleIndex = face * 2 + i;
            const auto& indices = cube.indices[triangleIndex];
            
            if (lightingEnabled) {
                // 创建带法线的三角形
                Triangle triangle(
                    Vertex(cube.vertices[indices[0]], transformedNormal, Vec2(0, 0), cube.faceColors[face]),
                    Vertex(cube.vertices[indices[1]], transformedNormal, Vec2(1, 0), cube.faceColors[face]),
                    Vertex(cube.vertices[indices[2]], transformedNormal, Vec2(0, 1), cube.faceColors[face])
                );
                
                // 应用光照计算
                Material material; // 使用默认材质
                Triangle litTriangle;
                
                for (int v = 0; v < 3; ++v) {
                    // 复制顶点数据
                    litTriangle.vertices[v] = triangle.vertices[v];
                    
                    // 变换顶点位置（MVP变换）
                    litTriangle.vertices[v].position = transformVertex(triangle.vertices[v].position, mvpMatrix);
                    
                    // 计算光照（使用变换后的法线和位置） //在顶点阶段计算，在片元插值
                    Vec3 worldPos = transformNoDiv(modelMatrix, triangle.vertices[v].position);
                    litTriangle.vertices[v].color = calculateLighting(
                        Vertex(worldPos, transformedNormal, triangle.vertices[v].texCoord, triangle.vertices[v].color),
                        material, 
                        transformNoDiv(viewMatrix, eyePos, 0.0f) // 将观察位置变换到世界空间
                    );
                }
                
                // 绘制带光照的三角形
                drawTriangle(litTriangle);
            } else {
                // 不使用光照，直接绘制
                Triangle triangle(
                    Vertex(cube.vertices[indices[0]], cube.faceColors[face]),
                    Vertex(cube.vertices[indices[1]], cube.faceColors[face]),
                    Vertex(cube.vertices[indices[2]], cube.faceColors[face])
                );
                drawTriangle(triangle, mvpMatrix);
            }
        }
    }
}

// 计算光照颜色
Color Renderer::calculateLighting(const Vertex& vertex, const Material& material, const Vec3& eyePos)
{
    if (!lightingEnabled) {
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
    float3 ambient =material.ambient * light.color * light.ambientIntensity;
    
    // 计算漫反射分量（考虑光照角度）
    float diff = std::max(dot(normal, lightDir), 0.0f);
    float3 diffuse =material.diffuse * light.color * (diff * light.intensity);
    
    // 计算镜面反射分量（考虑视角）
    float spec = std::pow(std::max(dot(normal, halfwayDir), 0.0f), material.shininess);
    float3 specular =material.specular * light.color * spec * light.intensity;;
    
    // 合并所有光照分量
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
        vertex.color.a
    );
    
    return finalColor;
}

