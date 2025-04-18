// main.cpp
// 测试光栅化渲染器的主程序
#include "renderer.h"
#include <iostream>
#include <fstream>
#include <string>
#include "mesh.h"
#include "test.h"


int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;
    
    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);

    // 创建一个简单的OBJ文件用于测试
    std::string testObjFile = "box_sphere.obj";
    // CreateSimpleOBJ(testObjFile);
    
    // 加载并渲染OBJ模型
    DrawOBJModel(renderer, testObjFile);
    
    // 以下是原来的测试代码，可以根据需要注释或取消注释
    // DrawTriangle(renderer);
    //DrawTriangleWithLighting(renderer);
    //DrawCube(renderer);
    //DrawCubeWithLighting(renderer);
    
    std::cout << "渲染完成!" << std::endl;
    
    return 0;
}