// Renderer.h
// 基本光栅化渲染器的头文件

#pragma once

#include <vector>
#include <memory>
#include <array>

#include "maths.h"


// 光照结构体
struct Light
{
    Vec3 position;      // 光源位置
    Vec3 color;         // 光源颜色
    float intensity;    // 光源强度
    float ambientIntensity; // 环境光强度

    Light() : position(0, 0, 0), color(1, 1, 1), intensity(1.0f), ambientIntensity(0.1f) {}
    Light(const Vec3& pos, const Vec3& col, float intens, float ambIntens)
        : position(pos), color(col), intensity(intens), ambientIntensity(ambIntens) {}
};

// 材质结构体
struct Material
{
    Vec3 ambient;       // 环境光反射系数
    Vec3 diffuse;       // 漫反射系数
    Vec3 specular;      // 镜面反射系数
    float shininess;    // 光泽度（用于镜面反射计算）

    Material() : ambient(0.1f, 0.1f, 0.1f), diffuse(0.7f, 0.7f, 0.7f), 
                 specular(0.2f, 0.2f, 0.2f), shininess(32.0f) {}
    Material(const Vec3& amb, const Vec3& diff, const Vec3& spec, float shin)
        : ambient(amb), diffuse(diff), specular(spec), shininess(shin) {}
};


// 顶点结构体
struct Vertex {
    Vec3 position;  // 位置
    Vec3 normal;    // 法线
    Vec2 texCoord;  // 纹理坐标
    Color color;    // 顶点颜色
    
    Vertex() = default;
    Vertex(const Vec3& pos, const Color& col) : position(pos), color(col) {}
    Vertex(const Vec3& pos, const Vec3& norm, const Vec2& tex, const Color& col) : 
        position(pos), normal(norm), texCoord(tex), color(col) {}
};

// 三角形结构体
struct Triangle {
    std::array<Vertex, 3> vertices;
    
    Triangle() = default;
    Triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
    }
};


// 立方体结构体
struct Cube {
        std::array<Vec3, 8> vertices; // 8个顶点
        std::array<std::array<int, 3>, 12> indices; // 12个三角形(6个面，每个面2个三角形)
        std::array<Color, 6> faceColors; // 6个面的颜色
        std::array<Vec3, 8> normals; // 8个顶点的法线
        std::array<Vec3, 6> faceNormals; // 6个面的法线
        
        // 创建单位立方体
        static Cube createUnitCube() {
            Cube cube;
            
            // 定义8个顶点(以原点为中心)
            cube.vertices[0] = Vec3(-0.5f, -0.5f, -0.5f); // 前左下
            cube.vertices[1] = Vec3( 0.5f, -0.5f, -0.5f); // 前右下
            cube.vertices[2] = Vec3( 0.5f,  0.5f, -0.5f); // 前右上
            cube.vertices[3] = Vec3(-0.5f,  0.5f, -0.5f); // 前左上
            cube.vertices[4] = Vec3(-0.5f, -0.5f,  0.5f); // 后左下
            cube.vertices[5] = Vec3( 0.5f, -0.5f,  0.5f); // 后右下
            cube.vertices[6] = Vec3( 0.5f,  0.5f,  0.5f); // 后右上
            cube.vertices[7] = Vec3(-0.5f,  0.5f,  0.5f); // 后左上
            
            // 定义12个三角形(6个面，每个面2个三角形)
            // 前面(z = -0.5)
            cube.indices[0] = {0, 1, 2};
            cube.indices[1] = {0, 2, 3};
            
            // 后面(z = 0.5)
            cube.indices[2] = {5, 4, 7};
            cube.indices[3] = {5, 7, 6};
            
            // 左面(x = -0.5)
            cube.indices[4] = {4, 0, 3};
            cube.indices[5] = {4, 3, 7};
            
            // 右面(x = 0.5)
            cube.indices[6] = {1, 5, 6};
            cube.indices[7] = {1, 6, 2};
            
            // 上面(y = 0.5)
            cube.indices[8] = {3, 2, 6};
            cube.indices[9] = {3, 6, 7};
            
            // 下面(y = -0.5)
            cube.indices[10] = {4, 5, 1};
            cube.indices[11] = {4, 1, 0};
            
             // 计算每个面的法线
            // 前面(z = -0.5) - 法线指向负z方向
            cube.faceNormals[0] = Vec3(0.0f, 0.0f, -1.0f);
            
            // 后面(z = 0.5) - 法线指向正z方向
            cube.faceNormals[1] = Vec3(0.0f, 0.0f, 1.0f);
            
            // 左面(x = -0.5) - 法线指向负x方向
            cube.faceNormals[2] = Vec3(-1.0f, 0.0f, 0.0f);
            
            // 右面(x = 0.5) - 法线指向正x方向
            cube.faceNormals[3] = Vec3(1.0f, 0.0f, 0.0f);
            
            // 上面(y = 0.5) - 法线指向正y方向
            cube.faceNormals[4] = Vec3(0.0f, 1.0f, 0.0f);
            
            // 下面(y = -0.5) - 法线指向负y方向
            cube.faceNormals[5] = Vec3(0.0f, -1.0f, 0.0f);

            // 设置面的颜色
            cube.faceColors[0] = Color(255, 0, 0);     // 前面 - 红色
            cube.faceColors[1] = Color(0, 255, 0);     // 后面 - 绿色
            cube.faceColors[2] = Color(0, 0, 255);     // 左面 - 蓝色
            cube.faceColors[3] = Color(255, 255, 0);   // 右面 - 黄色
            cube.faceColors[4] = Color(0, 255, 255);   // 上面 - 青色
            cube.faceColors[5] = Color(255, 0, 255);   // 下面 - 紫色

            
            return cube;
        }
    };
      

// 帧缓冲类
class FrameBuffer {
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer() = default;
    
    // 设置像素（带深度测试）
    void setPixel(int x, int y, float depth, const Color& color);
    // 设置像素（不带深度测试）
    void setPixel(int x, int y, const Color& color);
    // 获取深度值
    float getDepth(int x, int y) const;
    // 清除缓冲区
    void clear(const Color& color = Color(0, 0, 0, 255), float depth = 1.0f);
    // 获取帧缓冲区数据
    const uint8_t* getData() const { return frameData.data(); }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width;
    int height;
    std::vector<uint8_t> frameData; // 按 RGBA 格式存储
    std::vector<float> depthBuffer; // 深度缓冲区
    
    // 检查坐标是否在有效范围内
    bool isValidCoord(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    
    // 计算索引
    int calcIndex(int x, int y) const {
        return y * width + x;
    }
};

// 光栅化渲染器类
class Renderer {
    public:
        Renderer(int width, int height);
        ~Renderer() = default;

        bool lightingEnabled;
        Light light;
        // 绘制变换后的三角形
        void drawTriangle(const Triangle& triangle, const Matrix4x4& mvpMatrix);
        
        // 绘制未变换的三角形(原始方法)
        void drawTriangle(const Triangle& triangle);
        
        // 清除屏幕
        void clear(const Color& color = Color(0, 0, 0, 255));
        
        // 设置变换矩阵
        void setModelMatrix(const Matrix4x4& matrix) { modelMatrix = matrix; }
        void setViewMatrix(const Matrix4x4& matrix) { viewMatrix = matrix; }
        void setProjMatrix(const Matrix4x4& matrix) { projMatrix = matrix; }
        
        // 获取当前变换矩阵
        Matrix4x4 getModelMatrix() const { return modelMatrix; }
        Matrix4x4 getViewMatrix() const { return viewMatrix; }
        Matrix4x4 getProjMatrix() const { return projMatrix; }
        Matrix4x4 getMVPMatrix() const { return projMatrix * viewMatrix * modelMatrix; }
        
        // 变换顶点位置
        Vec3 transformVertex(const Vec3& position, const Matrix4x4& mvpMatrix);
        
        // 获取帧缓冲
        const FrameBuffer& getFrameBuffer() const { return *frameBuffer; }

        
        void drawCube(const Cube& cube);
        Color calculateLighting(const Vertex& vertex, const Material& material, const Vec3& eyePos);
        void setLight(Light& light){this->light=light;};
    
        // 启用光照
        void enableLighting(bool b){this->lightingEnabled =b;};
        // 变换法线向量
        // Vec3 transformNormal(const Matrix4x4& modelMatrix, const Vec3& normal);
        
    private:
        std::unique_ptr<FrameBuffer> frameBuffer;
        Matrix4x4 modelMatrix; // 模型变换
        Matrix4x4 viewMatrix;  // 视图变换
        Matrix4x4 projMatrix;  // 投影变换
    };
