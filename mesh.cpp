// mesh.cpp
// Mesh类和OBJ文件加载的实现 (修复后)

#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

// Mesh构造函数
Mesh::Mesh() {
    // 设置默认材质
    material = Material();
}

// 添加顶点
void Mesh::addVertex(const Vec3f& v) {
    vertices.push_back(v);
    // 添加默认顶点颜色（白色）
    vertexColors.push_back(Color(255, 255, 255));
}

// 添加纹理坐标
void Mesh::addTexCoord(const Vec2f& t) {
    texCoords.push_back(t);
}

// 添加法线
void Mesh::addNormal(const Vec3f& n) {
    normals.push_back(n);
}

// 添加面
void Mesh::addFace(const Face& f) {
    faces.push_back(f);
}

// 设置材质
void Mesh::setMaterial(const Material& mat) {
    material = mat;
}

// 计算面法线
void Mesh::calculateFaceNormals() {
    // 确保法线向量容器大小与面数相同
    normals.resize(faces.size());
    
    for (size_t i = 0; i < faces.size(); ++i) {
        const Face& face = faces[i];
        if (face.vertexIndices.size() < 3) {
            continue; // 跳过无效面
        }
        
        // 获取面的三个顶点
        const Vec3f& v0 = vertices[face.vertexIndices[0]];
        const Vec3f& v1 = vertices[face.vertexIndices[1]];
        const Vec3f& v2 = vertices[face.vertexIndices[2]];
        
        // 计算面法线（顶点按逆时针排列，右手法则）
        Vec3f edge1 = v1 - v0;
        Vec3f edge2 = v2 - v0;
        Vec3f normal = cross(edge1, edge2);
        
        // 归一化法线
        normals[i] = normalize(normal);
    }
}

// 计算顶点法线
void Mesh::calculateVertexNormals() {
    // 清空现有法线
    normals.clear();
    normals.resize(vertices.size(), Vec3f(0, 0, 0));
    
    // 遍历所有面，累加面法线到相关顶点
    for (const Face& face : faces) {
        if (face.vertexIndices.size() < 3) {
            continue; // 跳过无效面
        }
        
        // 获取面的三个顶点
        const Vec3f& v0 = vertices[face.vertexIndices[0]];
        const Vec3f& v1 = vertices[face.vertexIndices[1]];
        const Vec3f& v2 = vertices[face.vertexIndices[2]];
        
        // 计算面法线
        Vec3f edge1 = v1 - v0;
        Vec3f edge2 = v2 - v0;
        Vec3f normal = normalize(cross(edge1, edge2));
        
        // 将面法线加到所有相关顶点
        for (int index : face.vertexIndices) {
            normals[index] = normals[index] + normal;
        }
    }
    
    // 归一化所有顶点法线
    for (Vec3f& normal : normals) {
        normal = normalize(normal);
    }
}

// 计算边界框
void Mesh::calculateBoundingBox(Vec3f& min, Vec3f& max) const {
    if (vertices.empty()) {
        min = Vec3f(0, 0, 0);
        max = Vec3f(0, 0, 0);
        return;
    }
    
    min = Vec3f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    max = Vec3f(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
    
    for (const Vec3f& v : vertices) {
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
    }
}

// 设置颜色
void Mesh::setColor(const Color& color) {
    vertexColors.clear();
    vertexColors.resize(vertices.size(), color);
}

// 中心化网格
void Mesh::centerize() {
    Vec3f min, max;
    calculateBoundingBox(min, max);
    
    Vec3f center = Vec3f(
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f,
        (min.z + max.z) * 0.5f
    );
    
    // 移动所有顶点，使中心点位于原点
    for (Vec3f& v : vertices) {
        v.x -= center.x;
        v.y -= center.y;
        v.z -= center.z;
    }
}

// 将网格缩放到单位立方体大小
// void Mesh::normaliz()e() {
//     Vec3f min, max;
//     calculateBoundingBox(min, max);
    
//     // 计算模型的尺寸
//     float siz()eX = max.x - min.x;
//     float siz()eY = max.y - min.y;
//     float siz()eZ() = max.z - min.z;
    
//     // 找出最大尺寸
//     float maxSiz()e = std::max(std::max(siz()eX, siz()eY), siz()eZ());
    
//     if (maxSiz()e < 0.0001f) {
//         return; // 防止除以零
//     }
    
//     // 缩放因子
//     float scale = 1.0f / maxSiz()e;
    
//     // 缩放所有顶点
//     for (Vec3f& v : vertices) {
//         v.x *= scale;
//         v.y *= scale;
//         v.z *= scale;
//     }
// }

// 将面转换为三角形
std::vector<Triangle> Mesh::triangulate(const Face& face) const {
    std::vector<Triangle> triangles;
    
    if (face.vertexIndices.size() < 3) {
        return triangles; // 面必须至少有3个顶点
    }
    
    // 简单的三角形扇形分解
    for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
        Triangle triangle;
        
        // 获取顶点索引
        int idx0 = face.vertexIndices[0];
        int idx1 = face.vertexIndices[i];
        int idx2 = face.vertexIndices[i + 1];
        
        // 获取顶点位置
        Vec3f pos0 = vertices[idx0];
        Vec3f pos1 = vertices[idx1];
        Vec3f pos2 = vertices[idx2];
        
        // 默认顶点法线
        Vec3f normal0 = Vec3f(0, 0, 1);
        Vec3f normal1 = Vec3f(0, 0, 1);
        Vec3f normal2 = Vec3f(0, 0, 1);
        
        // 如果有法线索引，则使用指定的法线
        if (!face.normalIndices.empty() && normals.size() > 0) {
            if (face.normalIndices.size() > 0) normal0 = normals[face.normalIndices[0]];
            if (face.normalIndices.size() > i) normal1 = normals[face.normalIndices[i]];
            if (face.normalIndices.size() > i + 1) normal2 = normals[face.normalIndices[i + 1]];
        } 
        // 如果没有指定法线，但有计算顶点法线，则使用顶点法线
        else if (normals.size() >= vertices.size()) {
            normal0 = normals[idx0];
            normal1 = normals[idx1];
            normal2 = normals[idx2];
        }
        
        // 默认纹理坐标
        Vec2f tex0 = Vec2f(0, 0);
        Vec2f tex1 = Vec2f(1, 0);
        Vec2f tex2 = Vec2f(0, 1);
        
        // 如果有纹理索引，则使用指定的纹理坐标
        if (!face.texCoordIndices.empty() && texCoords.size() > 0) {
            if (face.texCoordIndices.size() > 0) tex0 = texCoords[face.texCoordIndices[0]];
            if (face.texCoordIndices.size() > i) tex1 = texCoords[face.texCoordIndices[i]];
            if (face.texCoordIndices.size() > i + 1) tex2 = texCoords[face.texCoordIndices[i + 1]];
        }
        
        // 获取顶点颜色
        Color color0 = vertexColors[idx0];
        Color color1 = vertexColors[idx1];
        Color color2 = vertexColors[idx2];
        
        // 创建顶点
        triangle.vertices[0] = Vertex(pos0, normal0, tex0, color0);
        triangle.vertices[1] = Vertex(pos1, normal1, tex1, color1);
        triangle.vertices[2] = Vertex(pos2, normal2, tex2, color2);
        
        triangles.push_back(triangle);
    }
    
    return triangles;
}

// 绘制网格 - 修复版本
void Mesh::draw(Renderer& renderer) const {
    // 获取当前的MVP矩阵
    Matrix4x4f mvpMatrix = renderer.getMVPMatrix();
    
    // 在观察空间中，摄像机位于原点
    Vec3f eyePos = Vec3f(0, 0, 0);
    
    // 获取世界空间中的相机位置（用于光照计算）
    Vec3f worldEyePos = transformNoDiv(renderer.getViewMatrix(), eyePos, 0.0f);
    
    for (const Face& face : faces) {
        // 将面转换为一个或多个三角形
        std::vector<Triangle> triangles = triangulate(face);
        
        for (const Triangle& tri : triangles) {
            if (renderer.lightingEnabled) {
                // 创建带光照的三角形
                Triangle litTriangle;
                
                for (int v = 0; v < 3; ++v) {
                    // 复制顶点数据
                    litTriangle.vertices[v] = tri.vertices[v];
                    
                    // 变换顶点位置和法线
                    Vec3f worldPos = transformNoDiv(renderer.getModelMatrix(), tri.vertices[v].position);
                    Vec3f transformedNormal = transformNormal(renderer.getModelMatrix(), tri.vertices[v].normal);
                    
                    // 创建带变换后法线的顶点
                    Vertex worldVertex(worldPos, transformedNormal, tri.vertices[v].texCoord, tri.vertices[v].color);
                    
                    // 计算光照
                    litTriangle.vertices[v].color = renderer.calculateLighting(worldVertex, material, worldEyePos);
                    
                    // 变换顶点位置到屏幕空间
                    litTriangle.vertices[v].position = renderer.transformVertex(tri.vertices[v].position, mvpMatrix);
                }
                
                // 绘制带光照的三角形
                renderer.drawTriangle(litTriangle);
            } else {
                // 不使用光照，直接绘制
                renderer.drawTriangle(tri, mvpMatrix);
            }
        }
    }
}

// 加载OBJ文件
std::shared_ptr<Mesh> loadOBJ(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "无法打开文件：" << filename << std::endl;
        return nullptr;
    }
    
    auto mesh = std::make_shared<Mesh>();
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "v") {
            // 读取顶点
            float x, y, z;
            iss >> x >> y >> z;
            mesh->addVertex(Vec3f(x, y, z));
        }
        else if (token == "vt") {
            // 读取纹理坐标
            float u, v;
            iss >> u >> v;
            mesh->addTexCoord(Vec2f(u, v));
        }
        else if (token == "vn") {
            // 读取法线
            float x, y, z;
            iss >> x >> y >> z;
            mesh->addNormal(normalize(Vec3f(x, y, z)));
        }
        else if (token == "f") {
            // 读取面
            Face face;
            std::string vertex;
            
            // 读取面的所有顶点
            while (iss >> vertex) {
                std::istringstream vertexStream(vertex);
                std::string indexStr;
                
                // 顶点索引
                std::getline(vertexStream, indexStr, '/');
                if (!indexStr.empty()) {
                    face.vertexIndices.push_back(std::stoi(indexStr) - 1); // OBJ索引从1开始
                }
                
                // 纹理坐标索引
                std::getline(vertexStream, indexStr, '/');
                if (!indexStr.empty()) {
                    face.texCoordIndices.push_back(std::stoi(indexStr) - 1);
                }
                
                // 法线索引
                std::getline(vertexStream, indexStr, '/');
                if (!indexStr.empty()) {
                    face.normalIndices.push_back(std::stoi(indexStr) - 1);
                }
            }
            
            mesh->addFace(face);
        }
        // 忽略其他行（如mtl材质定义）
    }
    
    // 如果没有法线，则计算顶点法线
    if (mesh->getVertexCount() > 0 && mesh->getFaceCount() > 0 && 
        (mesh->normals.empty() || mesh->normals.size() < mesh->vertices.size())) {
        mesh->calculateVertexNormals();
    }
    
    // 中心化和标准化网格
    mesh->centerize();
    // mesh->normaliz()e();
    
    std::cout << "已加载 " << filename << "：" << mesh->getVertexCount() << " 个顶点，" 
              << mesh->getFaceCount() << " 个面。" << std::endl;
    
    return mesh;
}

// 矩阵-向量变换函数
Vec3f transform(const Matrix4x4f& matrix, const Vec3f& vector, float w ) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    float wOut = vector.x * matrix.m30 + vector.y * matrix.m31 + vector.z * matrix.m32 + w * matrix.m33;
    
    // 透视除法
    if (std::abs(wOut) > 1e-6f) {
        float invW = 1.0f / wOut;
        return Vec3f(x * invW, y * invW, z * invW);
    }
    
    return Vec3f(x, y, z);
}


// 添加辅助函数：transformNoDiv 和 transformNormal 的实现
Vec3f transformNoDiv(const Matrix4x4f& matrix, const Vec3f& vector, float w) {
    float x = vector.x * matrix.m00 + vector.y * matrix.m01 + vector.z * matrix.m02 + w * matrix.m03;
    float y = vector.x * matrix.m10 + vector.y * matrix.m11 + vector.z * matrix.m12 + w * matrix.m13;
    float z = vector.x * matrix.m20 + vector.y * matrix.m21 + vector.z * matrix.m22 + w * matrix.m23;
    
    return Vec3f(x, y, z);
}

Vec3f transformNormal(const Matrix4x4f& modelMatrix, const Vec3f& normal) {
    // 简化实现: 假设模型矩阵只有旋转和均匀缩放，可以直接使用模型矩阵
    Vec3f result;
    result.x = normal.x * modelMatrix.m00 + normal.y * modelMatrix.m01 + normal.z * modelMatrix.m02;
    result.y = normal.x * modelMatrix.m10 + normal.y * modelMatrix.m11 + normal.z * modelMatrix.m12;
    result.z = normal.x * modelMatrix.m20 + normal.y * modelMatrix.m21 + normal.z * modelMatrix.m22;
    
    return normalize(result);
}