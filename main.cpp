#include "scene.h"
#include "texture_io.h"
#include "platform/platform.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <cstring>
#include "scene_manager.h"

// 全局变量
bool g_debugMode = false;

// 将帧缓冲区保存为 PPM 图像文件
void saveToPPM(const std::string &filename, const FrameBuffer &frameBuffer)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        if (g_debugMode)
        {
            std::cerr << "无法创建文件：" << filename << std::endl;
        }
        return;
    }

    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();

    // 写入 PPM 文件头
    file << "P6\n"
         << width << " " << height << "\n255\n";

    // 写入像素数据（注意：PPM 只支持 RGB，不支持 alpha 通道）
    const uint8_t *data = frameBuffer.getData();
    for (int i = 0; i < width * height; ++i)
    {
        int index = i * 4;                                           // RGBA 格式
        file.write(reinterpret_cast<const char *>(&data[index]), 3); // 只写入 RGB
    }

    if (g_debugMode)
    {
        std::cout << "图像已保存到 " << filename << std::endl;
    }
}

// 将渲染器的帧缓冲区复制到平台帧缓冲区
void copyFrameBufferToPlatform(const Renderer &renderer)
{
    // 获取平台帧缓冲区
    uint32_t *dst = static_cast<uint32_t *>(platform_get_framebuffer());

    // 直接使用帧缓冲区内置的复制方法
    renderer.getFrameBuffer().copyToPlatform(dst);
}

// 处理相机移动
void processCamera(Camera &camera, float deltaTime)
{
    const float moveSpeed = 2.0f * deltaTime;
    const float rotateSpeed = 1.0f * deltaTime;
    const float zoomSpeed = 5.0f * deltaTime;

    Vec3f position = camera.getPosition();
    Vec3f target = camera.getTarget();
    Vec3f up = camera.getUp();

    // 计算前向、右向和上向量
    Vec3f forward = normalize(target - position);
    Vec3f right = normalize(cross(forward, up));
    Vec3f upDir = normalize(cross(right, forward));

    // 处理键盘输入 - 移动
    if (platform_get_key(PLATFORM_KEY_W))
    {
        position = position + forward * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_S))
    {
        position = position - forward * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_A))
    {
        position = position - right * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_D))
    {
        position = position + right * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_Q))
    {
        position = position + upDir * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_E))
    {
        position = position - upDir * moveSpeed;
    }

    // 处理鼠标输入 - 旋转
    int dx, dy;
    platform_get_mouse_delta(&dx, &dy);

    if (platform_get_mouse_button(PLATFORM_MOUSE_LEFT) && (dx != 0 || dy != 0))
    {
        // 水平旋转
        float yaw = atan2f(forward.z, forward.x) + dx * rotateSpeed * 0.01f;
        float pitch = asinf(forward.y) - dy * rotateSpeed * 0.01f;

        // 限制俯仰角
        pitch = std::max(std::min(pitch, 1.5f), -1.5f);

        // 计算新的前向向量
        forward.x = cosf(yaw) * cosf(pitch);
        forward.y = sinf(pitch);
        forward.z = sinf(yaw) * cosf(pitch);

        forward = normalize(forward);
        target = position + forward;
    }

    // 处理鼠标滚轮 - 缩放
    float wheelX, wheelY;
    platform_get_mouse_wheel(&wheelX, &wheelY);

    if (wheelY != 0.0f)
    {
        // 计算当前距离
        float distance = (target - position).length();

        // 根据滚轮方向调整距离
        float newDistance = distance - wheelY * zoomSpeed;

        // 限制最小距离，防止穿过目标点
        newDistance = std::max(newDistance, 0.1f);

        // 更新位置
        position = target - forward * newDistance;
    }

    // 更新相机
    camera.setPosition(position);
    camera.setTarget(target);
}

// 打印帮助信息
void printHelp()
{
    std::cout << "软光栅渲染器使用说明：" << std::endl;
    std::cout << "  --help            显示此帮助信息" << std::endl;
    std::cout << "  --debug           启用调试模式，显示控制台输出" << std::endl;
    std::cout << "  --scene=<type>    选择场景类型 (default, spheres, cubes)" << std::endl;
    std::cout << "  --msaa=<0|1>      启用/禁用MSAA抗锯齿 (默认: 0)" << std::endl;
    std::cout << "  --shadow=<0|1>    启用/禁用阴影投射 (默认: 0)" << std::endl;
    std::cout << std::endl;
    std::cout << "控制方式：" << std::endl;
    std::cout << "  W/A/S/D          前后左右移动" << std::endl;
    std::cout << "  Q/E              上升/下降" << std::endl;
    std::cout << "  鼠标左键拖动      旋转视角" << std::endl;
    std::cout << "  鼠标滚轮         缩放视图" << std::endl;
}

// 解析命令行参数
void parseCommandLine(int argc, char *argv[], SceneType &sceneType, bool &enableMSAA, bool &enableShadow)
{
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--help")
        {
            printHelp();
            exit(0);
        }
        else if (arg == "--debug")
        {
            g_debugMode = true;
        }
        else if (arg.find("--scene=") == 0)
        {
            std::string sceneArg = arg.substr(8);
            if (sceneArg == "spheres")
            {
                sceneType = SceneType::SPHERES;
            }
            else if (sceneArg == "cubes")
            {
                sceneType = SceneType::CUBES;
            }
            else
            {
                sceneType = SceneType::DEFAULT;
            }
        }
        else if (arg.find("--msaa=") == 0)
        {
            std::string msaaArg = arg.substr(7);
            enableMSAA = (msaaArg == "1");
        }
        else if (arg.find("--shadow=") == 0)
        {
            std::string shadowArg = arg.substr(9);
            enableShadow = (shadowArg == "1");
        }
    }
}

int main(int argc, char *argv[])
{
    const int WIDTH = 800;
    const int HEIGHT = 450;
    const char *TITLE = "软光栅渲染器 - Sequoia15.3.1";

    // 默认参数
    SceneType sceneType = SceneType::DEFAULT;
    bool enableMSAA = false;
    bool enableShadow = false;

    // 解析命令行参数
    parseCommandLine(argc, argv, sceneType, enableMSAA, enableShadow);

    // 初始化平台
    if (!platform_init(TITLE, WIDTH, HEIGHT))
    {
        std::cerr << "平台初始化失败!" << std::endl;
        return -1;
    }

    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);
    renderer.enableMSAA(enableMSAA);

    // 创建场景
    Scene scene;

    // 使用场景管理器初始化场景
    SceneManager sceneManager;
    if (g_debugMode)
    {
        std::cout << "初始化场景..." << std::endl;
    }
    sceneManager.initializeScene(sceneType, scene, WIDTH, HEIGHT);

    // 设置阴影映射
    scene.setupShadowMapping(enableShadow);

    if (g_debugMode)
    {
        std::cout << "渲染设置：" << std::endl;
        std::cout << "  MSAA: " << (enableMSAA ? "启用" : "禁用") << std::endl;
        std::cout << "  阴影: " << (enableShadow ? "启用" : "禁用") << std::endl;
    }

    // 主循环部分保持不变
    double lastTime = platform_get_time();
    int frameCount = 0;
    double fpsTime = lastTime;
    long long lastFrameTime = 0;

    while (!platform_should_close())
    {

        // 处理事件
        platform_process_events();

        // 计算帧率
        double currentTime = platform_get_time();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        frameCount++;
        if (currentTime - fpsTime >= 1.0)
        {
            char title[128];
            snprintf(title, sizeof(title), "%s - FPS: %d | 渲染时间: %lld ms",
                     TITLE, frameCount, lastFrameTime);
            platform_set_title(title);
            frameCount = 0;
            fpsTime = currentTime;
        }

        if (g_debugMode)
        {
            std::cout << "渲染耗时: " << lastFrameTime << "ms" << std::endl;
        }

        // 处理相机控制
        processCamera(scene.getCamera(), static_cast<float>(deltaTime));

        // 渲染场景
        renderer.clear(Vec4f(0.1f, 0.1f, 0.1f, 1.0f));
        // 添加简单的性能计时器
        auto startTime = std::chrono::high_resolution_clock::now();
        scene.render(renderer);
        auto endTime = std::chrono::high_resolution_clock::now();
        lastFrameTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // 复制帧缓冲区到平台
        copyFrameBufferToPlatform(renderer);

        // 更新屏幕
        platform_update_framebuffer();

        // 处理截图
        if (platform_should_take_screenshot())
        {
            saveToPPM(platform_get_screenshot_filename(), renderer.getFrameBuffer());
        }

        // 限制帧率
        if (deltaTime < 1.0 / 60.0)
        {
            platform_sleep(1.0 / 60.0 - deltaTime);
        }
    }

    // 清理资源
    platform_cleanup();

    if (g_debugMode)
    {
        std::cout << "渲染完成!" << std::endl;
    }

    return 0;
}