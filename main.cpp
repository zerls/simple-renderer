#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <memory>
#include <string>
#include <array>
#include <sstream>
#include <unordered_map>

// 向量和数学结构定义
struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float f) const { return Vec3(x * f, y * f, z * f); }
    
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    
    Vec3 normalize() const {
        float len = length();
        if (len > 0)
            return Vec3(x / len, y / len, z / len);
        return *this;
    }
    
    // 用于unordered_map中的哈希比较
    bool operator==(const Vec3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Vec3的哈希函数，用于unordered_map
namespace std {
    template<> struct hash<Vec3*> {
        size_t operator()(Vec3* const& v) const {
            return hash<float>()(v->x) ^ 
                  (hash<float>()(v->y) << 1) ^ 
                  (hash<float>()(v->z) << 2);
        }
    };
}

struct Vec2 {
    float u, v;
    
    Vec2() : u(0), v(0) {}
    Vec2(float u, float v) : u(u), v(v) {}
};

struct Matrix4x4 {
    union {
        struct {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
        float m[16];
    };
    
    Matrix4x4() {
        m00 = 1.0f; m01 = 0.0f; m02 = 0.0f; m03 = 0.0f;
        m10 = 0.0f; m11 = 1.0f; m12 = 0.0f; m13 = 0.0f;
        m20 = 0.0f; m21 = 0.0f; m22 = 1.0f; m23 = 0.0f;
        m30 = 0.0f; m31 = 0.0f; m32 = 0.0f; m33 = 1.0f;
    }
    
    static Matrix4x4 identity() {
        return Matrix4x4();
    }
    
    static Matrix4x4 perspective(float fov, float aspect, float near, float far) {
        Matrix4x4 result;
        float tanHalfFov = std::tan(fov / 2.0f);
        
        result.m00 = 1.0f / (aspect * tanHalfFov);
        result.m11 = 1.0f / tanHalfFov;
        result.m22 = -(far + near) / (far - near);
        result.m23 = -1.0f;
        result.m32 = -(2.0f * far * near) / (far - near);
        result.m33 = 0.0f;
        
        return result;
    }
    
    // 正交投影矩阵，适用于阴影贴图生成
    static Matrix4x4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        Matrix4x4 result;
        
        result.m00 = 2.0f / (right - left);
        result.m11 = 2.0f / (top - bottom);
        result.m22 = -2.0f / (far - near);
        
        result.m03 = -(right + left) / (right - left);
        result.m13 = -(top + bottom) / (top - bottom);
        result.m23 = -(far + near) / (far - near);
        
        return result;
    }
    
    static Matrix4x4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Matrix4x4 result;
        
        Vec3 f = (center - eye).normalize();
        Vec3 s = f.cross(up).normalize();
        Vec3 u = s.cross(f);
        
        result.m00 = s.x;
        result.m01 = u.x;
        result.m02 = -f.x;
        result.m03 = 0.0f;
        
        result.m10 = s.y;
        result.m11 = u.y;
        result.m12 = -f.y;
        result.m13 = 0.0f;
        
        result.m20 = s.z;
        result.m21 = u.z;
        result.m22 = -f.z;
        result.m23 = 0.0f;
        
        result.m30 = -s.dot(eye);
        result.m31 = -u.dot(eye);
        result.m32 = f.dot(eye);
        result.m33 = 1.0f;
        
        return result;
    }
    
    Vec3 transformVector(const Vec3& v) const {
        return Vec3(
            m00 * v.x + m01 * v.y + m02 * v.z + m03,
            m10 * v.x + m11 * v.y + m12 * v.z + m13,
            m20 * v.x + m21 * v.y + m22 * v.z + m23
        );
    }
    
    // 执行透视除法的变换
    Vec3 transformVectorComplete(const Vec3& v) const {
        float x = m00 * v.x + m01 * v.y + m02 * v.z + m03;
        float y = m10 * v.x + m11 * v.y + m12 * v.z + m13;
        float z = m20 * v.x + m21 * v.y + m22 * v.z + m23;
        float w = m30 * v.x + m31 * v.y + m32 * v.z + m33;
        
        if (w != 0) {
            return Vec3(x/w, y/w, z/w);
        }
        return Vec3(x, y, z);
    }
    
    Vec3 transformDirection(const Vec3& v) const {
        return Vec3(
            m00 * v.x + m01 * v.y + m02 * v.z,
            m10 * v.x + m11 * v.y + m12 * v.z,
            m20 * v.x + m21 * v.y + m22 * v.z
        );
    }
    
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        
        result.m00 = m00 * other.m00 + m01 * other.m10 + m02 * other.m20 + m03 * other.m30;
        result.m01 = m00 * other.m01 + m01 * other.m11 + m02 * other.m21 + m03 * other.m31;
        result.m02 = m00 * other.m02 + m01 * other.m12 + m02 * other.m22 + m03 * other.m32;
        result.m03 = m00 * other.m03 + m01 * other.m13 + m02 * other.m23 + m03 * other.m33;
        
        result.m10 = m10 * other.m00 + m11 * other.m10 + m12 * other.m20 + m13 * other.m30;
        result.m11 = m10 * other.m01 + m11 * other.m11 + m12 * other.m21 + m13 * other.m31;
        result.m12 = m10 * other.m02 + m11 * other.m12 + m12 * other.m22 + m13 * other.m32;
        result.m13 = m10 * other.m03 + m11 * other.m13 + m12 * other.m23 + m13 * other.m33;
        
        result.m20 = m20 * other.m00 + m21 * other.m10 + m22 * other.m20 + m23 * other.m30;
        result.m21 = m20 * other.m01 + m21 * other.m11 + m22 * other.m21 + m23 * other.m31;
        result.m22 = m20 * other.m02 + m21 * other.m12 + m22 * other.m22 + m23 * other.m32;
        result.m23 = m20 * other.m03 + m21 * other.m13 + m22 * other.m23 + m23 * other.m33;
        
        result.m30 = m30 * other.m00 + m31 * other.m10 + m32 * other.m20 + m33 * other.m30;
        result.m31 = m30 * other.m01 + m31 * other.m11 + m32 * other.m21 + m33 * other.m31;
        result.m32 = m30 * other.m02 + m31 * other.m12 + m32 * other.m22 + m33 * other.m32;
        result.m33 = m30 * other.m03 + m31 * other.m13 + m32 * other.m23 + m33 * other.m33;
        
        return result;
    }
};

// 颜色表示
struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    
    // 颜色混合
    Color blend(const Color& other, float factor) const {
        float invFactor = 1.0f - factor;
        return Color(
            static_cast<uint8_t>(r * invFactor + other.r * factor),
            static_cast<uint8_t>(g * invFactor + other.g * factor),
            static_cast<uint8_t>(b * invFactor + other.b * factor)
        );
    }
};

// 顶点结构
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    
    Vertex() {}
    
    Vertex(const Vec3& position, const Vec3& normal, const Vec2& texCoord)
        : position(position), normal(normal), texCoord(texCoord) {}
};

// 三角形结构
struct Triangle {
    std::array<Vertex, 3> vertices;
    
    Triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        vertices[0] = v1;
        vertices[1] = v2;
        vertices[2] = v3;
    }
};

// 光源类
class Light {
public:
    Vec3 position;
    Color color;
    float intensity;
    
    // 新增：光源视角的阴影贴图矩阵
    Matrix4x4 lightViewMatrix;
    Matrix4x4 lightProjectionMatrix;
    Matrix4x4 lightSpaceMatrix; // 从世界空间到光源空间的转换矩阵
    
    Light(const Vec3& position, const Color& color, float intensity)
        : position(position), color(color), intensity(intensity) {}
    
    // 更新光源的视图和投影矩阵
    void updateMatrices(const Vec3& targetPosition, const Vec3& up, 
                        float left, float right, float bottom, float top, 
                        float near, float far) {
        // 对于方向光，我们使用正交投影
        lightViewMatrix = Matrix4x4::lookAt(position, targetPosition, up);
        lightProjectionMatrix = Matrix4x4::orthographic(left, right, bottom, top, near, far);
        lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
    }
};

// 网格类
class Mesh {
public:
    std::vector<Triangle> triangles;
    
    void addTriangle(const Triangle& triangle) {
        triangles.push_back(triangle);
    }
    
    // 从OBJ文件加载模型
    bool loadFromOBJ(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open OBJ file: " << filename << std::endl;
            return false;
        }
        
        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec2> texCoords;
        
        // OBJ索引从1开始，所以我们添加一个dummy元素在索引0
        positions.push_back(Vec3(0, 0, 0));
        normals.push_back(Vec3(0, 0, 0));
        texCoords.push_back(Vec2(0, 0));
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "v") {  // 顶点位置
                float x, y, z;
                iss >> x >> y >> z;
                positions.push_back(Vec3(x, y, z));
            }
            else if (token == "vn") {  // 顶点法线
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(Vec3(x, y, z));
            }
            else if (token == "vt") {  // 纹理坐标
                float u, v;
                iss >> u >> v;
                texCoords.push_back(Vec2(u, v));
            }
            else if (token == "f") {  // 面（三角形）
                std::vector<Vertex> faceVertices;
                
                std::string vertexData;
                while (iss >> vertexData) {
                    // 解析顶点数据格式 "位置索引/纹理索引/法线索引"
                    std::istringstream viss(vertexData);
                    std::string indexStr;
                    
                    int posIndex = 0, texIndex = 0, normIndex = 0;
                    
                    // 解析位置索引
                    std::getline(viss, indexStr, '/');
                    if (!indexStr.empty()) {
                        posIndex = std::stoi(indexStr);
                    }
                    
                    // 解析纹理索引
                    std::getline(viss, indexStr, '/');
                    if (!indexStr.empty()) {
                        texIndex = std::stoi(indexStr);
                    }
                    
                    // 解析法线索引
                    std::getline(viss, indexStr, '/');
                    if (!indexStr.empty()) {
                        normIndex = std::stoi(indexStr);
                    }
                    
                    // 创建顶点
                    Vertex vertex;
                    vertex.position = positions[posIndex];
                    
                    // 如果有纹理坐标
                    if (texIndex > 0 && texIndex < texCoords.size()) {
                        vertex.texCoord = texCoords[texIndex];
                    } else {
                        vertex.texCoord = Vec2(0, 0);
                    }
                    
                    // 如果有法线
                    if (normIndex > 0 && normIndex < normals.size()) {
                        vertex.normal = normals[normIndex];
                    } else {
                        vertex.normal = Vec3(0, 0, 1);  // 默认法线指向Z轴正方向
                    }
                    
                    faceVertices.push_back(vertex);
                }
                
                // 处理多边形（转换为三角形）
                if (faceVertices.size() >= 3) {
                    for (size_t i = 2; i < faceVertices.size(); ++i) {
                        Triangle triangle(faceVertices[0], faceVertices[i-1], faceVertices[i]);
                        addTriangle(triangle);
                    }
                }
            }
        }
        
        // 如果OBJ文件没有提供法线，我们可以计算它们
        if (normals.size() <= 1) {
            calculateNormals();
        }
        
        std::cout << "Loaded " << triangles.size() << " triangles from OBJ file." << std::endl;
        return true;
    }
    
    // 计算顶点法线
    void calculateNormals() {
        // 为每个顶点创建一个法线累加器
        std::unordered_map<Vec3*, Vec3> normalAccumulator;
        
        // 清除现有的法线
        for (auto& triangle : triangles) {
            for (int i = 0; i < 3; ++i) {
                triangle.vertices[i].normal = Vec3(0, 0, 0);
            }
        }
        
        // 计算每个三角形的法线并累加到相关顶点
        for (auto& triangle : triangles) {
            Vec3& v0 = triangle.vertices[0].position;
            Vec3& v1 = triangle.vertices[1].position;
            Vec3& v2 = triangle.vertices[2].position;
            
            // 计算三角形法线
            Vec3 edge1 = v1 - v0;
            Vec3 edge2 = v2 - v0;
            Vec3 normal = edge1.cross(edge2).normalize();
            
            // 添加到每个顶点的法线
            for (int i = 0; i < 3; ++i) {
                Vec3* posPtr = &triangle.vertices[i].position;
                if (normalAccumulator.find(posPtr) == normalAccumulator.end()) {
                    normalAccumulator[posPtr] = normal;
                } else {
                    normalAccumulator[posPtr] = normalAccumulator[posPtr] + normal;
                }
            }
        }
        
        // 将累加的法线分配回顶点并归一化
        for (auto& triangle : triangles) {
            for (int i = 0; i < 3; ++i) {
                Vec3* posPtr = &triangle.vertices[i].position;
                triangle.vertices[i].normal = normalAccumulator[posPtr].normalize();
            }
        }
        
        std::cout << "Calculated normals for " << normalAccumulator.size() << " unique vertices." << std::endl;
    }
};

// 相机类
class Camera {
public:
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float aspect;
    float nearPlane;
    float farPlane;
    
    Camera(const Vec3& position, const Vec3& target, const Vec3& up, 
           float fov, float aspect, float nearPlane, float farPlane)
        : position(position), target(target), up(up), fov(fov), 
          aspect(aspect), nearPlane(nearPlane), farPlane(farPlane) {}
          
    Matrix4x4 getViewMatrix() const {
        return Matrix4x4::lookAt(position, target, up);
    }
    
    Matrix4x4 getProjectionMatrix() const {
        return Matrix4x4::perspective(fov, aspect, nearPlane, farPlane);
    }
};

// 帧缓冲类
class Framebuffer {
private:
    int width;
    int height;
    std::vector<Color> colorBuffer;
    std::vector<float> depthBuffer;
    
public:
    Framebuffer(int width, int height) 
        : width(width), height(height) {
        colorBuffer.resize(width * height, Color(0, 0, 0));
        depthBuffer.resize(width * height, 1.0f);
    }
    
    void clear(const Color& color = Color(0, 0, 0)) {
        std::fill(colorBuffer.begin(), colorBuffer.end(), color);
        std::fill(depthBuffer.begin(), depthBuffer.end(), 1.0f);
    }
    
    void setPixel(int x, int y, const Color& color, float depth) {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
            
        int index = y * width + x;
        
        if (depth < depthBuffer[index]) {
            colorBuffer[index] = color;
            depthBuffer[index] = depth;
        }
    }
    
    float getDepth(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return 1.0f;
            
        return depthBuffer[y * width + x];
    }
    
    bool saveToPPM(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file << "P6\n" << width << " " << height << "\n255\n";
        
        for (const auto& color : colorBuffer) {
            file.write(reinterpret_cast<const char*>(&color.r), 1);
            file.write(reinterpret_cast<const char*>(&color.g), 1);
            file.write(reinterpret_cast<const char*>(&color.b), 1);
        }
        
        return true;
    }
    
    // 新增：保存深度缓冲为灰度图
    bool saveDepthBufferToPPM(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file << "P6\n" << width << " " << height << "\n255\n";
        
        for (const auto& depth : depthBuffer) {
            // 将深度值映射到0-255范围
            uint8_t value = static_cast<uint8_t>((1.0f - depth) * 255.0f);
            file.write(reinterpret_cast<const char*>(&value), 1);
            file.write(reinterpret_cast<const char*>(&value), 1);
            file.write(reinterpret_cast<const char*>(&value), 1);
        }
        
        return true;
    }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::vector<float>& getDepthBuffer() const { return depthBuffer; }
};

// 阴影贴图类
class ShadowMap {
private:
    std::unique_ptr<Framebuffer> shadowBuffer;
    int width;
    int height;
    
public:
    ShadowMap(int width, int height) 
        : width(width), height(height) {
        shadowBuffer = std::make_unique<Framebuffer>(width, height);
    }
    
    void clear() {
        shadowBuffer->clear();
    }
    
    void setDepth(int x, int y, float depth) {
        // 只关心深度值，不关心颜色
        shadowBuffer->setPixel(x, y, Color(0, 0, 0), depth);
    }
    
    float getDepth(int x, int y) const {
        return shadowBuffer->getDepth(x, y);
    }
    
    const std::vector<float>& getDepthBuffer() const {
        return shadowBuffer->getDepthBuffer();
    }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    // 用于调试
    bool saveToPPM(const std::string& filename) const {
        return shadowBuffer->saveDepthBufferToPPM(filename);
    }
    
    // 渲染模型到阴影贴图
    void render(const Mesh& mesh, const Light& light) {
        for (const auto& triangle : mesh.triangles) {
            // 变换顶点到光源空间
            std::array<Vec3, 3> lightSpaceVertices;
            for (int i = 0; i < 3; ++i) {
                lightSpaceVertices[i] = light.lightSpaceMatrix.transformVector(triangle.vertices[i].position);
            }
            
            // 简单的背面剔除
            Vec3 a = lightSpaceVertices[1] - lightSpaceVertices[0];
            Vec3 b = lightSpaceVertices[2] - lightSpaceVertices[0];
            Vec3 normal = a.cross(b);
            if (normal.z < 0) {
                continue; // 跳过背面三角形
            }
            
            // 转换到NDC坐标
            std::array<Vec3, 3> ndcVertices;
            for (int i = 0; i < 3; ++i) {
                float w = light.lightSpaceMatrix.m30 * triangle.vertices[i].position.x + 
                          light.lightSpaceMatrix.m31 * triangle.vertices[i].position.y + 
                          light.lightSpaceMatrix.m32 * triangle.vertices[i].position.z + 
                          light.lightSpaceMatrix.m33;
                          
                if (w != 0) {
                    ndcVertices[i].x = lightSpaceVertices[i].x / w;
                    ndcVertices[i].y = lightSpaceVertices[i].y / w;
                    ndcVertices[i].z = lightSpaceVertices[i].z / w;
                } else {
                    ndcVertices[i] = lightSpaceVertices[i];
                }
            }
            
            // 转换到屏幕坐标
            std::array<int, 3> screenX, screenY;
            for (int i = 0; i < 3; ++i) {
                screenX[i] = static_cast<int>((ndcVertices[i].x + 1.0f) * 0.5f * width);
                screenY[i] = static_cast<int>((1.0f - ndcVertices[i].y) * 0.5f * height);
            }
            
            // 找到边界框
            int minX = std::max(0, std::min({screenX[0], screenX[1], screenX[2]}));
            int maxX = std::min(width - 1, std::max({screenX[0], screenX[1], screenX[2]}));
            int minY = std::max(0, std::min({screenY[0], screenY[1], screenY[2]}));
            int maxY = std::min(height - 1, std::max({screenY[0], screenY[1], screenY[2]}));
            
            // 光栅化三角形到阴影贴图
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    float w0, w1, w2;
                    if (isInsideTriangle(x, y, 
                                       Vec3(screenX[0], screenY[0], 0), 
                                       Vec3(screenX[1], screenY[1], 0), 
                                       Vec3(screenX[2], screenY[2], 0), 
                                       w0, w1, w2)) {
                        
                        // 插值深度
                        float z = interpolate(ndcVertices[0].z, ndcVertices[1].z, ndcVertices[2].z, w0, w1, w2);
                        
                        // 存储深度值
                        setDepth(x, y, z);
                    }
                }
            }
        }
    }
    
private:
    // 检查点是否在三角形内
    bool isInsideTriangle(float x, float y, const Vec3& v0, const Vec3& v1, const Vec3& v2, 
                         float& w0, float& w1, float& w2) const {
        Vec3 p(x, y, 0);
        
        Vec3 a = Vec3(v0.x - v2.x, v1.x - v2.x, v2.x - p.x);
        Vec3 b = Vec3(v0.y - v2.y, v1.y - v2.y, v2.y - p.y);
        
        Vec3 u = a.cross(b);
        
        if (std::abs(u.z) < 1e-6)
            return false;
            
        w0 = u.x / u.z;
        w1 = u.y / u.z;
        w2 = 1.0f - w0 - w1;
        
        return (w0 >= 0 && w1 >= 0 && w2 >= 0);
    }
    
    // 插值使用重心坐标
    float interpolate(float attr0, float attr1, float attr2, float w0, float w1, float w2) const {
        return attr0 * w0 + attr1 * w1 + attr2 * w2;
    }
};

// 渲染器类
class Renderer {
private:
    std::unique_ptr<Framebuffer> framebuffer;
    std::unique_ptr<ShadowMap> shadowMap;
    int width;
    int height;
    float shadowBias; // 阴影偏移量，避免z-fighting
    
    // 转换NDC坐标到屏幕坐标
    void ndcToScreen(float ndcX, float ndcY, int& screenX, int& screenY) const {
        screenX = static_cast<int>((ndcX + 1.0f) * 0.5f * width);
        screenY = static_cast<int>((1.0f - ndcY) * 0.5f * height);
    }
    
    // 检查点是否在三角形内（使用重心坐标）
    bool isInsideTriangle(float x, float y, const Vec3& v0, const Vec3& v1, const Vec3& v2, 
                         float& w0, float& w1, float& w2) const {
        Vec3 p(x, y, 0);
        
        Vec3 a = Vec3(v0.x - v2.x, v1.x - v2.x, v2.x - p.x);
        Vec3 b = Vec3(v0.y - v2.y, v1.y - v2.y, v2.y - p.y);
        
        Vec3 u = a.cross(b);
        
        if (std::abs(u.z) < 1e-6)
            return false;
            
        w0 = u.x / u.z;
        w1 = u.y / u.z;
        w2 = 1.0f - w0 - w1;
        
        return (w0 >= 0 && w1 >= 0 && w2 >= 0);
    }
    
    // 使用重心坐标插值顶点属性
    float interpolate(float attr0, float attr1, float attr2, float w0, float w1, float w2) const {
        return attr0 * w0 + attr1 * w1 + attr2 * w2;
    }
    
    // 检查点是否在阴影中
    bool isInShadow(const Vec3& worldPos, const Light& light) const {
        // 将世界坐标转换到光源空间
        Vec3 posFromLight = light.lightSpaceMatrix.transformVector(worldPos);
        
        // 执行透视除法
        float w = light.lightSpaceMatrix.m30 * worldPos.x + 
                 light.lightSpaceMatrix.m31 * worldPos.y + 
                 light.lightSpaceMatrix.m32 * worldPos.z + 
                 light.lightSpaceMatrix.m33;
                 
        if (w != 0) {
            posFromLight.x /= w;
            posFromLight.y /= w;
            posFromLight.z /= w;
        }
        
        // 转换到阴影贴图坐标 (0-1)
        float shadowMapX = (posFromLight.x + 1.0f) * 0.5f;
        float shadowMapY = (1.0f - posFromLight.y) * 0.5f;
        
        // 将坐标转换为阴影贴图像素坐标
        int shadowX = static_cast<int>(shadowMapX * shadowMap->getWidth());
        int shadowY = static_cast<int>(shadowMapY * shadowMap->getHeight());
        
        // 获取存储的深度值
        float closestDepth = shadowMap->getDepth(shadowX, shadowY);
        
        // 当前片段的深度
        float currentDepth = posFromLight.z;
        
        // 添加偏移量避免阴影痤疮
        return currentDepth - shadowBias > closestDepth;
    }
    
public:
    Renderer(int width, int height) 
        : width(width), height(height), shadowBias(0.005f) {
        framebuffer = std::make_unique<Framebuffer>(width, height);
        
        // 为阴影贴图创建更高分辨率的缓冲区
        int shadowMapSize = 1024; // 阴影贴图分辨率
        shadowMap = std::make_unique<ShadowMap>(shadowMapSize, shadowMapSize);
    }
    
    void clear(const Color& color = Color(0, 0, 0)) {
        framebuffer->clear(color);
        shadowMap->clear();
    }
    
    // 创建阴影贴图
    void createShadowMap(const Mesh& mesh, const Light& light) {
        shadowMap->clear();
        shadowMap->render(mesh, light);
        
        // 调试：保存阴影贴图
        shadowMap->saveToPPM("shadow_map.ppm");
    }
    
    // 渲染场景（包含阴影）
    void render(const Mesh& mesh, const Camera& camera, const Light& light) {
        Matrix4x4 view = camera.getViewMatrix();
        Matrix4x4 projection = camera.getProjectionMatrix();
        Matrix4x4 viewProjection = projection * view;
        
        for (const auto& triangle : mesh.triangles) {
            // 变换顶点到裁剪空间
            std::array<Vec3, 3> clipVertices;
            for (int i = 0; i < 3; i++) {
                clipVertices[i] = viewProjection.transformVector(triangle.vertices[i].position);
            }
            
            // 简单的背面剔除
            Vec3 a = clipVertices[1] - clipVertices[0];
            Vec3 b = clipVertices[2] - clipVertices[0];
            Vec3 normal = a.cross(b);
            if (normal.z < 0) {
                continue; // 跳过背面三角形
            }
            
            // 透视除法得到NDC坐标
            std::array<Vec3, 3> ndcVertices;
            for (int i = 0; i < 3; i++) {
                float w = viewProjection.m30 * triangle.vertices[i].position.x + 
                         viewProjection.m31 * triangle.vertices[i].position.y + 
                         viewProjection.m32 * triangle.vertices[i].position.z + 
                         viewProjection.m33;
                         
                if (w != 0) {
                    ndcVertices[i].x = clipVertices[i].x / w;
                    ndcVertices[i].y = clipVertices[i].y / w;
                    ndcVertices[i].z = clipVertices[i].z / w;
                } else {
                    ndcVertices[i] = clipVertices[i];
                }
            }
            
            // 转换NDC到屏幕坐标
            std::array<int, 3> screenX, screenY;
            for (int i = 0; i < 3; i++) {
                ndcToScreen(ndcVertices[i].x, ndcVertices[i].y, screenX[i], screenY[i]);
            }
            
            // 找到边界框
            int minX = std::max(0, std::min({screenX[0], screenX[1], screenX[2]}));
            int maxX = std::min(width - 1, std::max({screenX[0], screenX[1], screenX[2]}));
            int minY = std::max(0, std::min({screenY[0], screenY[1], screenY[2]}));
            int maxY = std::min(height - 1, std::max({screenY[0], screenY[1], screenY[2]}));
            
            // 光栅化三角形
            for (int y = minY; y <= maxY; y++) {
                for (int x = minX; x <= maxX; x++) {
                    float w0, w1, w2;
                    if (isInsideTriangle(x, y, 
                                       Vec3(screenX[0], screenY[0], 0), 
                                       Vec3(screenX[1], screenY[1], 0), 
                                       Vec3(screenX[2], screenY[2], 0), 
                                       w0, w1, w2)) {
                        
                        // 插值深度(Z)
                        float z = interpolate(
                            1.0f / clipVertices[0].z, 
                            1.0f / clipVertices[1].z, 
                            1.0f / clipVertices[2].z, 
                            w0, w1, w2
                        );
                        z = 1.0f / z;
                        
                        // 插值世界空间位置，用于阴影测试
                        Vec3 worldPos = triangle.vertices[0].position * w0 + 
                                      triangle.vertices[1].position * w1 + 
                                      triangle.vertices[2].position * w2;
                        
                        // 插值法线
                        Vec3 normal = triangle.vertices[0].normal * w0 + 
                                    triangle.vertices[1].normal * w1 + 
                                    triangle.vertices[2].normal * w2;
                        normal = normal.normalize();
                        
                        // 光照方向
                        Vec3 lightDir = (light.position - worldPos).normalize();
                        
                        // 漫反射光照
                        float diffuse = std::max(0.0f, normal.dot(lightDir));
                        
                        // 环境光
                        float ambient = 0.2f;
                        
                        // 检查阴影
                        bool inShadow = isInShadow(worldPos, light);
                        
                        // 如果在阴影中，只使用环境光
                        float lightIntensity = inShadow ? ambient : ambient + diffuse * light.intensity;
                        
                        // 应用光照
                        uint8_t intensity = static_cast<uint8_t>(255.0f * lightIntensity);
                        Color pixelColor(intensity, intensity, intensity);
                        
                        // 如果在阴影中，给一个颜色提示（仅用于可视化）
                        if (inShadow) {
                            pixelColor = pixelColor.blend(Color(50, 50, 100), 0.3f);
                        }
                        
                        framebuffer->setPixel(x, y, pixelColor, z);
                    }
                }
            }
        }
    }
    
    bool saveOutput(const std::string& filename) const {
        return framebuffer->saveToPPM(filename);
    }
};

// 添加一个简单的地面平面到场景中
void addGroundPlane(Mesh& mesh, float size = 10.0f, float y = -2.0f) {
    // 地面顶点
    Vec3 v0(-size, y, -size);
    Vec3 v1(size, y, -size);
    Vec3 v2(size, y, size);
    Vec3 v3(-size, y, size);
    
    // 向上的法线
    Vec3 normal(0, 1, 0);
    
    // 纹理坐标
    Vec2 t0(0, 0);
    Vec2 t1(1, 0);
    Vec2 t2(1, 1);
    Vec2 t3(0, 1);
    
    // 添加两个三角形形成地面
    mesh.addTriangle(Triangle(
        Vertex(v0, normal, t0),
        Vertex(v1, normal, t1),
        Vertex(v2, normal, t2)
    ));
    
    mesh.addTriangle(Triangle(
        Vertex(v0, normal, t0),
        Vertex(v2, normal, t2),
        Vertex(v3, normal, t3)
    ));
}

// 示例使用
int main(int argc, char* argv[]) {
    const int width = 800;
    const int height = 600;
    
    // 创建渲染器
    Renderer renderer(width, height);
    
    // 创建网格
    Mesh mesh;
    
    // 尝试加载OBJ文件
    bool loaded = false;
    if (argc > 1) {
        loaded = mesh.loadFromOBJ(argv[1]);
    }
    
    // 如果没有加载成功，使用默认的立方体
    if (!loaded) {
        std::cout << "Using default cube mesh..." << std::endl;
        
        // 定义立方体顶点
        std::vector<Vec3> positions = {
            Vec3(-1, -1, -1), Vec3(1, -1, -1), Vec3(1, 1, -1), Vec3(-1, 1, -1),
            Vec3(-1, -1, 1), Vec3(1, -1, 1), Vec3(1, 1, 1), Vec3(-1, 1, 1)
        };
        
        std::vector<Vec3> normals = {
            Vec3(0, 0, -1), Vec3(0, 0, 1), Vec3(0, -1, 0),
            Vec3(0, 1, 0), Vec3(-1, 0, 0), Vec3(1, 0, 0)
        };
        
        // 添加立方体面
        // 前面
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[0], Vec2(0, 0)),
            Vertex(positions[1], normals[0], Vec2(1, 0)),
            Vertex(positions[2], normals[0], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[0], Vec2(0, 0)),
            Vertex(positions[2], normals[0], Vec2(1, 1)),
            Vertex(positions[3], normals[0], Vec2(0, 1))
        ));
        
        // 后面
        mesh.addTriangle(Triangle(
            Vertex(positions[5], normals[1], Vec2(0, 0)),
            Vertex(positions[4], normals[1], Vec2(1, 0)),
            Vertex(positions[7], normals[1], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[5], normals[1], Vec2(0, 0)),
            Vertex(positions[7], normals[1], Vec2(1, 1)),
            Vertex(positions[6], normals[1], Vec2(0, 1))
        ));
        
        // 底面
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[2], Vec2(0, 0)),
            Vertex(positions[4], normals[2], Vec2(1, 0)),
            Vertex(positions[5], normals[2], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[2], Vec2(0, 0)),
            Vertex(positions[5], normals[2], Vec2(1, 1)),
            Vertex(positions[1], normals[2], Vec2(0, 1))
        ));
        
        // 顶面
        mesh.addTriangle(Triangle(
            Vertex(positions[3], normals[3], Vec2(0, 0)),
            Vertex(positions[2], normals[3], Vec2(1, 0)),
            Vertex(positions[6], normals[3], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[3], normals[3], Vec2(0, 0)),
            Vertex(positions[6], normals[3], Vec2(1, 1)),
            Vertex(positions[7], normals[3], Vec2(0, 1))
        ));
        
        // 左面
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[4], Vec2(0, 0)),
            Vertex(positions[3], normals[4], Vec2(1, 0)),
            Vertex(positions[7], normals[4], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[0], normals[4], Vec2(0, 0)),
            Vertex(positions[7], normals[4], Vec2(1, 1)),
            Vertex(positions[4], normals[4], Vec2(0, 1))
        ));
        
        // 右面
        mesh.addTriangle(Triangle(
            Vertex(positions[1], normals[5], Vec2(0, 0)),
            Vertex(positions[5], normals[5], Vec2(1, 0)),
            Vertex(positions[6], normals[5], Vec2(1, 1))
        ));
        
        mesh.addTriangle(Triangle(
            Vertex(positions[1], normals[5], Vec2(0, 0)),
            Vertex(positions[6], normals[5], Vec2(1, 1)),
            Vertex(positions[2], normals[5], Vec2(0, 1))
        ));
    }
    
    // 添加地面
    addGroundPlane(mesh);
    
    // 设置相机
    Camera camera(
        Vec3(3, 3, 5),    // 位置
        Vec3(0, 0, 0),    // 目标
        Vec3(0, 1, 0),    // 上向量
        45.0f * 3.14159f / 180.0f,  // FOV（弧度）
        static_cast<float>(width) / height,  // 宽高比
        0.1f,    // 近平面
        100.0f   // 远平面
    );
    
    // 设置光源
    Light light(
        Vec3(5, 8, 5),    // 位置
        Color(255, 255, 255),  // 颜色
        0.8f               // 强度
    );
    
    // 更新光源矩阵以覆盖整个场景
    light.updateMatrices(
        Vec3(0, 0, 0),    // 光源看向的点
        Vec3(0, 1, 0),    // 上向量
        -10.0f, 10.0f,    // 左右边界
        -10.0f, 10.0f,    // 下上边界
        1.0f, 20.0f       // 近远平面
    );
    
    // 清除帧缓冲
    renderer.clear(Color(50, 50, 50));
    
    // 首先创建阴影贴图
    renderer.createShadowMap(mesh, light);
    
    // 然后渲染场景（使用阴影贴图）
    renderer.render(mesh, camera, light);
    
    // 保存输出
    if (renderer.saveOutput("output_with_shadows.ppm")) {
        std::cout << "Rendered image with shadows saved to output_with_shadows.ppm" << std::endl;
    } else {
        std::cerr << "Failed to save rendered image" << std::endl;
    }
    
    return 0;
}