// main.cpp
// 测试光栅化渲染器的主程序（更新版本）
#include "renderer.h"
#include <iostream>
#include <fstream>
#include <string>
#include "mesh.h"
#include "test.h"
#include "shader.h"


int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;
    
    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);

    
    // 创建一个简单的OBJ文件用于测试
    std::string testObjFile = "box_sphere.obj";
    
    // 加载并渲染OBJ模型（使用Phong着色器）
    std::cout << "渲染OBJ模型，使用Phong着色器..." << std::endl;
    DrawOBJModel(renderer, testObjFile);
    
    std::cout << "渲染完成!" << std::endl;
    
    return 0;
}