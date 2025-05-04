// mesh.h修改
#pragma once

#include <vector>
#include <string>
#include <memory>
#include "maths.h"
#include "common.h"
#include "IResource.h"

// 定义Mesh类
class Mesh : public IResource {
public:
    Mesh();
    ~Mesh() = default;
    
    // 添加顶点、纹理坐标、法线和面
    void addVertex(const Vec3f& v);
    void addTexCoord(const Vec2f& t);
    void addNormal(const Vec3f& n);
    void addTangent(const Vec4f& t);
    void addFace(const Face& f);
    
    // 计算切线向量
    void calculateTangents();
    
    // 计算面法线
    void calculateFaceNormals();
    
    // 计算顶点法线
    void calculateVertexNormals();
    
    // 计算边界框
    void calculateBoundingBox(Vec3f& min, Vec3f& max) const;
    
    // 获取顶点数量
    size_t getVertexCount() const { return vertices.size(); }
    
    // 获取面数量
    size_t getFaceCount() const { return faces.size(); }
    
    // 获取三角形数量
    size_t getTriangleCount() const { return triangles.size(); }
    
    // 设置颜色
    void setColor(const Color& color);
    
    // 中心化网格
    void centerize();
    
    // 预先将所有面转换为三角形
    void triangulate();
    
    // 获取预计算的三角形
    const std::vector<Triangle>& getTriangles() const { return triangles; }
    
    // 获取顶点颜色
    const std::vector<float4>& getVertexColors() const { return vertexColors; }
    
    // 数据成员
    std::vector<Vec3f> vertices;         // 顶点
    std::vector<Vec2f> texCoords;        // 纹理坐标
    std::vector<Vec3f> normals;          // 法线
    std::vector<Vec4f> tangents;         // 切线
    std::vector<Face> faces;             // 面
    std::vector<Triangle> triangles;     // 预计算的三角形
    std::vector<float4> vertexColors;    // 顶点颜色(改为float4)
    
private:
    // 将单个面转换为三角形
    std::vector<Triangle> triangulateFace(const Face& face) const;
};

// OBJ文件加载函数
std::shared_ptr<Mesh> loadOBJ(const std::string& filename);