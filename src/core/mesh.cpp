// mesh.cpp
// Mesh类和OBJ文件加载的实现 (更新版)

#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

// Mesh构造函数
Mesh::Mesh() : IResource(ResourceType::MESH) {
    // Rest of your constructor implementation
}

// 添加顶点
void Mesh::addVertex(const Vec3f& v) {
    vertices.push_back(v);
    // 添加默认顶点颜色（白色）
    vertexColors.push_back(float4(1.0f));
}

// 添加纹理坐标
void Mesh::addTexCoord(const Vec2f& t) {
    texCoords.push_back(t);
}

// 添加法线
void Mesh::addNormal(const Vec3f& n) {
    normals.push_back(n);
}

// 添加切线
void Mesh::addTangent(const Vec4f& t) {
    tangents.push_back(t);
}

// 添加面
void Mesh::addFace(const Face& f) {
    faces.push_back(f);
}

// 计算切线向量
void Mesh::calculateTangents() {
    if (texCoords.empty() || normals.empty()) {
        std::cerr << "需要纹理坐标和法线来计算切线" << std::endl;
        return;
    }
    
    // 清空现有切线
    tangents.clear();
    tangents.resize(vertices.size(), Vec4f(0, 0, 0, 0));
    
    std::vector<Vec3f> tan1(vertices.size(), Vec3f(0, 0, 0));
    std::vector<Vec3f> tan2(vertices.size(), Vec3f(0, 0, 0));
    
    // 计算每个三角形的切线和副切线
    for (const Face& face : faces) {
        if (face.vertexIndices.size() < 3) continue;
        
        // 对于多边形面，转换为三角形
        for (size_t i = 1; i < face.vertexIndices.size() - 1; ++i) {
            int i0 = face.vertexIndices[0];
            int i1 = face.vertexIndices[i];
            int i2 = face.vertexIndices[i + 1];
            
            const Vec3f& v0 = vertices[i0];
            const Vec3f& v1 = vertices[i1];
            const Vec3f& v2 = vertices[i2];
            
            Vec2f uv0 = face.texCoordIndices.size() > 0 ? texCoords[face.texCoordIndices[0]] : Vec2f(0, 0);
            Vec2f uv1 = face.texCoordIndices.size() > i ? texCoords[face.texCoordIndices[i]] : Vec2f(0, 0);
            Vec2f uv2 = face.texCoordIndices.size() > i + 1 ? texCoords[face.texCoordIndices[i + 1]] : Vec2f(0, 0);
            
            Vec3f edge1 = v1 - v0;
            Vec3f edge2 = v2 - v0;
            Vec2f deltaUV1 = uv1 - uv0;
            Vec2f deltaUV2 = uv2 - uv0;
            
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            
            Vec3f tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            
            Vec3f bitangent;
            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
            
            tan1[i0] = tan1[i0] + tangent;
            tan1[i1] = tan1[i1] + tangent;
            tan1[i2] = tan1[i2] + tangent;
            
            tan2[i0] = tan2[i0] + bitangent;
            tan2[i1] = tan2[i1] + bitangent;
            tan2[i2] = tan2[i2] + bitangent;
        }
    }
    
    // 计算最终的切线和手性
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec3f& n = normals[i];
        const Vec3f& t = tan1[i];
        
        // Gram-Schmidt正交化
        Vec3f tangent = normalize(t - n * dot(n, t));
        
        // 计算手性
        float handedness = (dot(cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
        
        tangents[i] = Vec4f(tangent.x, tangent.y, tangent.z, handedness);
    }
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
    vertexColors.resize(vertices.size(), color.toFloat4());
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

void Mesh::triangulate()
{
    triangles.clear();
    
    for (const Face& face : faces) {
        // 获取当前面的三角形数组
        std::vector<Triangle> faceTriangles = triangulateFace(face);
        
        // 将获取到的三角形添加到网格的triangles集合中
        triangles.insert(triangles.end(), faceTriangles.begin(), faceTriangles.end());
    }
    
    std::cout << "已将 " << faces.size() << " 个面转换为 " << triangles.size() << " 个三角形。" << std::endl;
}

// 将triangulateFace方法更新为用float4存储顶点颜色
std::vector<Triangle> Mesh::triangulateFace(const Face& face) const
{
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
        
        // 默认切线
        Vec4f tangent0 = Vec4f(1, 0, 0, 1);
        Vec4f tangent1 = Vec4f(1, 0, 0, 1);
        Vec4f tangent2 = Vec4f(1, 0, 0, 1);
        
        // 如果有切线索引，则使用指定的切线
        if (!face.tangentIndices.empty() && tangents.size() > 0) {
            if (face.tangentIndices.size() > 0) tangent0 = tangents[face.tangentIndices[0]];
            if (face.tangentIndices.size() > i) tangent1 = tangents[face.tangentIndices[i]];
            if (face.tangentIndices.size() > i + 1) tangent2 = tangents[face.tangentIndices[i + 1]];
        }
        // 如果没有指定切线，但有计算顶点切线，则使用顶点切线
        else if (tangents.size() >= vertices.size()) {
            tangent0 = tangents[idx0];
            tangent1 = tangents[idx1];
            tangent2 = tangents[idx2];
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
        float4 color0 = vertexColors[idx0];
        float4 color1 = vertexColors[idx1];
        float4 color2 = vertexColors[idx2];
        
        // 创建顶点
        triangle.vertices[0] = Vertex(pos0, normal0, tangent0, tex0, color0);
        triangle.vertices[1] = Vertex(pos1, normal1, tangent1, tex1, color1);
        triangle.vertices[2] = Vertex(pos2, normal2, tangent2, tex2, color2);
        
        triangles.push_back(triangle);
    }
    
    return triangles;
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
    
    // 如果有纹理坐标和法线，但没有切线，则计算切线
    if (mesh->getVertexCount() > 0 && mesh->getFaceCount() > 0 && 
        !mesh->texCoords.empty() && !mesh->normals.empty() && mesh->tangents.empty()) {
        mesh->calculateTangents();
    }
    
    // 中心化网格
    mesh->centerize();
    
    // 预先将所有面转换为三角形
    mesh->triangulate();
    
    std::cout << "已加载 " << filename << "：" << mesh->getVertexCount() << " 个顶点，" 
              << mesh->getFaceCount() << " 个面，" << mesh->getTriangleCount() << " 个三角形。" << std::endl;
    
    return mesh;
}