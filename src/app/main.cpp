#include "scene.h"
#include "texture_io.h"
#include "platform.h"
#include "utils.hpp"
#include "camera_controller.h"
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
    std::cout << "  W/A/S/D         前后左右移动" << std::endl;
    std::cout << "  Q/E             上升/下降" << std::endl;
    std::cout << "  鼠标左键拖动     旋转视角" << std::endl;
    std::cout << "  鼠标滚轮        缩放视图" << std::endl;
    std::cout << "  ESC             退出程序" << std::endl;
    std::cout << "  F2              截图" << std::endl;
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
    const char *TITLE = "软光栅渲染器";

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

        // if (g_debugMode)
        // {
        //     std::cout << "渲染耗时: " << lastFrameTime << "ms" << std::endl;
        // }


        // 处理相机控制
        processCamera(scene.getCamera(), static_cast<float>(deltaTime));
        // 渲染场景
        // renderer.clear(Vec4f(0.1f, 0.1f, 0.1f, 1.0f));
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
            saveToPPM(platform_get_screenshot_filename(), renderer.getFrameBuffer(),g_debugMode);
        }

        // 限制帧率
        // if (deltaTime < 1.0 / 60.0)
        // {
        //     platform_sleep(1.0 / 60.0 - deltaTime);
        // }
    }

    // 清理资源
    platform_cleanup();

    if (g_debugMode)
    {
        std::cout << "渲染完成!" << std::endl;
    }

    return 0;
}