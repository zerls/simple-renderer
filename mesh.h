// mesh.h
// 定义Mesh类，用于表示3D模型

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "maths.h"
#include "renderer.h"



// 定义面的结构
struct Face {
    std::vector<int> vertexIndices;    // 顶点索引
    std::vector<int> texCoordIndices;  // 纹理坐标索引
    std::vector<int> normalIndices;    // 法线索引
    
    Face() = default;
};

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
    
    // 设置网格材质
    void setMaterial(const Material& mat);
    
    // 计算面法线（如果OBJ文件没有提供法线）
    void calculateFaceNormals();
    
    // 计算顶点法线（通过平均相邻面的法线）
    void calculateVertexNormals();
    
    // 计算边界框
    void calculateBoundingBox(Vec3f& min, Vec3f& max) const;
    
    // 获取材质
    const Material& getMaterial() const { return material; }
    
    // 绘制网格
    void draw(Renderer& renderer) const;
    
    // 获取顶点数量
    size_t getVertexCount() const { return vertices.size(); }
    
    // 获取面数量
    size_t getFaceCount() const { return faces.size(); }
    
    // 设置颜色
    void setColor(const Color& color);
    
    // 中心化网格（使其中心位于原点）
    void centerize();
    
    // 缩放网格使其适合单位立方体
    // void normalize();
    
    // 将这些数据成员修改为public，以便在实现中访问
    std::vector<Vec3f> vertices;         // 顶点
    std::vector<Vec2f> texCoords;        // 纹理坐标
    std::vector<Vec3f> normals;          // 法线
    std::vector<Face> faces;            // 面
    std::vector<Color> vertexColors;    // 顶点颜色
    Material material;                  // 材质
    
private:
    // 将面转换为三角形（如果面有多于3个顶点）
    std::vector<Triangle> triangulate(const Face& face) const;
};

// OBJ文件加载函数
std::shared_ptr<Mesh> loadOBJ(const std::string& filename);