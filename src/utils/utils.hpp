
#pragma once
#include "renderer.h"

// 将渲染器的帧缓冲区复制到平台帧缓冲区
void copyFrameBufferToPlatform(const Renderer &renderer);

void saveToPPM(const std::string &filename, const FrameBuffer &frameBuffer,bool debugMode);