/**
 * @file renderer_fragment.cpp
 * @brief 渲染器片段着色和插值实现
 */

 #include "maths.h"
 #include "renderer.h"
 #include <omp.h>
 
 // 全局常量
 constexpr float EPSILON = 1e-6f;
 
 // 计算重心坐标
 Vec3f Renderer::computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v)
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
 bool Renderer::isInsideTriangle(const Vec3f &barycentric)
 {
     // 简单判断所有重心坐标是否都大于-EPSILON
     return barycentric.x > -EPSILON && barycentric.y > -EPSILON && barycentric.z > -EPSILON;
 }
 
 // 计算透视校正权重
 Vec4f Renderer::calculatePerspectiveWeights(
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
 float Renderer::calculateFragmentDepth(
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
 void Renderer::interpolateVaryings(
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
 FragmentOutput Renderer::processFragment(
     const Varyings &interpolatedVaryings,
     std::shared_ptr<IShader> shader)
 {
     return shader->fragmentShader(interpolatedVaryings);
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
 
