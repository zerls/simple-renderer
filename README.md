# Simple Renderer

一个基于CPU的软件光栅化渲染器实现。

## 项目介绍

Simple Renderer是一个使用C++开发的轻量级软件光栅化渲染引擎，主要用于学习和演示计算机图形学中的基本渲染原理。该项目不依赖于OpenGL或DirectX等现有图形API，而是从零实现渲染管线的各个阶段。

### 主要功能

- 基础光栅化算法
- 顶点和片段处理
- 深度测试 (Z-buffer)
- 基本光照模型 (Phong/Blinn-Phong)
- 纹理映射
- 简单相机系统
- 模型加载与渲染
- GUI

## 构建与运行

### 环境要求

- C++20或更高版本
- CMake 3.20+
- 支持的平台：Windows、macOS、Linux

### 构建步骤

```bash
mkdir build
cd build
cmake ..
make
```

### 运行示例

```bash
./SimpleRenderer
```

## 使用指南

### 基本用法

通过命令行参数启动渲染器，可以配置不同的渲染选项：

```bash
# 运行默认场景
./SimpleRenderer

# 选择特定场景
./SimpleRenderer --scene=spheres

# 启用MSAA抗锯齿和阴影
./SimpleRenderer --msaa=1 --shadow=1

# 启用调试模式
./SimpleRenderer --debug
```

### 命令行选项

渲染器支持以下命令行参数：

- `--help` - 显示帮助信息
- `--debug` - 启用调试模式，显示控制台输出
- `--scene=<type>` - 选择场景类型 (可选值: default, spheres, cubes)
- `--msaa=<0|1>` - 启用/禁用MSAA抗锯齿 (默认: 0)
- `--shadow=<0|1>` - 启用/禁用阴影投射 (默认: 0)

### 控制方式

在渲染窗口中，您可以使用以下控制方式操作相机：

- `W/A/S/D` - 前后左右移动
- `Q/E` - 上升/下降
- `鼠标左键拖动` - 旋转视角
- `鼠标滚轮` - 缩放视图

### 截图功能

渲染器支持`F2`截图功能，将当前渲染结果保存为PPM格式图像。
