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
Mesh::Mesh() {
    // 不再包含材质和着色器的初始化
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
    
    // 中心化网格
    mesh->centerize();
    
    std::cout << "已加载 " << filename << "：" << mesh->getVertexCount() << " 个顶点，" 
              << mesh->getFaceCount() << " 个面。" << std::endl;
    
    return mesh;
}