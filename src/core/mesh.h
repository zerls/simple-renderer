// mesh.h
// 定义Mesh类，用于表示3D模型

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "maths.h"
#include "common.h"

// 定义Mesh类
class Mesh {
public:
    Mesh();
    ~Mesh() = default;
    
    // 添加顶点、纹理坐标、法线和面
    void addVertex(const Vec3f& v);
    void addTexCoord(const Vec2f& t);
    void addNormal(const Vec3f& n);
    void addFace(const Face& f);
    
    // 计算面法线（如果OBJ文件没有提供法线）
    void calculateFaceNormals();
    
    // 计算顶点法线（通过平均相邻面的法线）
    void calculateVertexNormals();
    
    // 计算边界框
    void calculateBoundingBox(Vec3f& min, Vec3f& max) const;
    
    // 获取顶点数量
    size_t getVertexCount() const { return vertices.size(); }
    
    // 获取面数量
    size_t getFaceCount() const { return faces.size(); }
    
    // 设置颜色
    void setColor(const Color& color);
    
    // 中心化网格（使其中心位于原点）
    void centerize();
    
    // 将面转换为三角形（如果面有多于3个顶点）
    std::vector<Triangle> triangulate(const Face& face) const;
    
    // 获取顶点颜色
    const std::vector<Color>& getVertexColors() const { return vertexColors; }
    
    // 数据成员
    std::vector<Vec3f> vertices;         // 顶点
    std::vector<Vec2f> texCoords;        // 纹理坐标
    std::vector<Vec3f> normals;          // 法线
    std::vector<Face> faces;             // 面
    std::vector<Color> vertexColors;     // 顶点颜色

    
};

// OBJ文件加载函数
std::shared_ptr<Mesh> loadOBJ(const std::string& filename);