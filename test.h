#pragma once

#include "renderer.h"

std::shared_ptr<Mesh> createTriangleMesh(const Color &color);

// 将帧缓冲区保存为 PPM 图像文件
void saveToPPM(const std::string& filename, const FrameBuffer& frameBuffer);
// 将深度缓冲区保存为 PPM 图像文件
void saveDepthMap(const std::string &filename, const FrameBuffer& frameBuffer,float nearPlane, float farPlane);