#pragma once

#include "renderer.h"

// 将帧缓冲区保存为 PPM 图像文件
void saveToPPM(const std::string& filename, const FrameBuffer& frameBuffer);
   
void DrawTriangle(Renderer& renderer);

void DrawTriangleWithLighting(Renderer &renderer);

void DrawCube(Renderer& renderer);

void DrawCubeWithLighting(Renderer &renderer);

// 新增：加载并绘制OBJ模型
// 修复后的 DrawOBJModel 函数
void DrawOBJModel(Renderer& renderer, const std::string& filename);
// 新增：创建并保存一个简单的OBJ文件（用于测试）
void CreateSimpleOBJ(const std::string& filename);