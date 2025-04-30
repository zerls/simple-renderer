#include "scene.h"
#include "texture_io.h"
#include "platform/platform.h"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

// 将帧缓冲区保存为 PPM 图像文件
void saveToPPM(const std::string& filename, const FrameBuffer& frameBuffer) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法创建文件：" << filename << std::endl;
        return;
    }

    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();
    
    // 写入 PPM 文件头
    file << "P6\n" << width << " " << height << "\n255\n";
    
    // 写入像素数据（注意：PPM 只支持 RGB，不支持 alpha 通道）
    const uint8_t* data = frameBuffer.getData();
    for (int i = 0; i < width * height; ++i) {
        int index = i * 4; // RGBA 格式
        file.write(reinterpret_cast<const char*>(&data[index]), 3); // 只写入 RGB
    }
    
    std::cout << "图像已保存到 " << filename << std::endl;
}

// 将渲染器的帧缓冲区复制到平台帧缓冲区
void copyFrameBufferToPlatform(const FrameBuffer& frameBuffer) {
    int width = frameBuffer.getWidth();
    int height = frameBuffer.getHeight();
    const uint8_t* src = frameBuffer.getData();
    uint32_t* dst = static_cast<uint32_t*>(platform_get_framebuffer());
    
    // 使用多线程并行处理
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(numThreads);
    
    // 按块处理以提高缓存命中率
    const int BLOCK_SIZE = 32; // 可以根据实际缓存大小调整
    
    auto processChunk = [&](int startRow, int endRow) {
        for (int blockY = startRow; blockY < endRow; blockY += BLOCK_SIZE) {
            for (int blockX = 0; blockX < width; blockX += BLOCK_SIZE) {
                // 处理当前块
                for (int y = blockY; y < std::min(blockY + BLOCK_SIZE, endRow); ++y) {
                    for (int x = blockX; x < std::min(blockX + BLOCK_SIZE, width); ++x) {
                        int i = y * width + x;
                        int srcIndex = i * 4; // RGBA 格式
                        
                        // 转换为 ARGB 格式 (SDL 使用)
                        dst[i] = 
                            (static_cast<uint32_t>(src[srcIndex + 3]) << 24) | // A
                            (static_cast<uint32_t>(src[srcIndex + 0]) << 16) | // R
                            (static_cast<uint32_t>(src[srcIndex + 1]) << 8) |  // G
                            (static_cast<uint32_t>(src[srcIndex + 2]));        // B
                    }
                }
            }
        }
    };
    
    // 划分工作区域
    int rowsPerThread = height / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        threads[i] = std::thread(processChunk, startRow, endRow);
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
}

// 处理相机移动
void processCamera(Camera& camera, float deltaTime) {
    const float moveSpeed = 2.0f * deltaTime;
    const float rotateSpeed = 1.0f * deltaTime;
    
    Vec3f position = camera.getPosition();
    Vec3f target = camera.getTarget();
    Vec3f up = camera.getUp();
    
    // 计算前向、右向和上向量
    Vec3f forward = normalize(target - position);
    Vec3f right = normalize(cross(forward, up));
    Vec3f upDir = normalize(cross(right, forward));
    
    // 处理键盘输入 - 移动
    if (platform_get_key(PLATFORM_KEY_W)) {
        position = position + forward * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_S)) {
        position = position - forward * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_A)) {
        position = position - right * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_D)) {
        position = position + right * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_Q)) {
        position = position + upDir * moveSpeed;
    }
    if (platform_get_key(PLATFORM_KEY_E)) {
        position = position - upDir * moveSpeed;
    }
    
    // 处理鼠标输入 - 旋转
    int dx, dy;
    platform_get_mouse_delta(&dx, &dy);
    
    if (platform_get_mouse_button(PLATFORM_MOUSE_LEFT) && (dx != 0 || dy != 0)) {
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
    
    // 更新相机
    camera.setPosition(position);
    camera.setTarget(target);
}

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 450;
    const char* TITLE = "软光栅渲染器 - Sequoia15.3.1";
    
    // 初始化平台
    if (!platform_init(TITLE, WIDTH, HEIGHT)) {
        std::cerr << "平台初始化失败!" << std::endl;
        return -1;
    }
    
    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);
    renderer.enableMSAA(false); // 启用 MSAA
  
    // 创建场景
    Scene scene;
    
    // 设置相机
    Camera &camera = scene.getCamera();
    camera.setPosition(Vec3f(0.0f, 3.0f, 4.0f));
    camera.setTarget(Vec3f(0.0f, 0.0f, 0.0f));
    camera.setUp(Vec3f(0.0f, 1.0f, 0.0f));
    camera.setAspect(static_cast<float>(WIDTH) / HEIGHT);
    camera.setFOV(3.14159f / 4.0f); // 45度视角
    
    // 设置光源
    Light light(Vec3f(1.0f, 2.0f, 4.0f), Vec3f(1.0f, 1.0f, 1.0f), 1.0f, 0.2f);
    scene.setLight(light);
    
    // 创建材质 - 红色材质带漫反射贴图
    auto material1 = scene.createMaterialWithTextures(
        "RedMaterial",
        "../assets/test.tga", // 漫反射贴图路径
        "../assets/normal_map.tga",  // 法线贴图路径
        float3(0.8f, 0.2f, 0.2f),    // 基础颜色
        32.0f                        // 光泽度
    );
    
    // 创建材质 - 蓝色材质带漫反射贴图
    auto material2 = scene.createMaterialWithTextures(
        "BlueMaterial",
        "", // 漫反射贴图路径
        "../assets/normal_map.tga",                           // 无法线贴图
        float3(0.3f, 0.2f, 0.8f),     // 基础颜色
        10.0f                         // 光泽度
    );
    
    auto material3 = scene.createMaterialWithTextures(
        "Material3",
        "../assets/blue_diffuse.tga", // 漫反射贴图路径
        "",                           // 无法线贴图
        float3(0.2f, 0.2f, 0.8f),     // 基础颜色
        4.0f                          // 光泽度
    );
    
    // 加载模型并添加到场景
    Matrix4x4f modelMatrix1 = Matrix4x4f::translation(-1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(0 ) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
    
    Matrix4x4f modelMatrix2 = Matrix4x4f::translation(1.0f, 0.0f, 0.0f) *
                              Matrix4x4f::rotationY(-30.0f) *
                              Matrix4x4f::scaling(0.7f, 0.7f, 0.7f);
    
    Matrix4x4f modelMatrix3 = Matrix4x4f::translation(0.0f, -1.0f, -1.0f) *
                              Matrix4x4f::rotationX(0) *
                              Matrix4x4f::scaling(2.0f, 2.0f, 2.0f);
    // 加载模型
    auto mesh = scene.loadMesh("../assets/sphere.obj", "RedSphere");
    auto mesh2 =  scene.loadMesh("../assets/box_sphere.obj", "BlueBox");
    auto mesh3 = scene.loadMesh("../assets/plane.obj", "plane");
    
    auto obj1 =SceneObject("RedSphere",mesh,material2,modelMatrix1);
    auto obj2 = SceneObject("BlueBox",mesh2,material3,modelMatrix2);
    auto obj3 = SceneObject("plane",mesh3,material1,modelMatrix3);
    
    scene.addObject(obj2);
    scene.addObject(obj1);
    scene.addObject(obj3);
    
    // 设置阴影映射
    scene.setupShadowMapping(false);
    
    // 主循环
    double lastTime = platform_get_time();
    int frameCount = 0;
    double fpsTime = lastTime;
    
    while (!platform_should_close()) {
        // 处理事件
        platform_process_events();
        
        // 计算帧率
        double currentTime = platform_get_time();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        frameCount++;
        if (currentTime - fpsTime >= 1.0) {
            char title[128];
            snprintf(title, sizeof(title), "%s - FPS: %d", TITLE, frameCount);
            platform_set_title(title);
            frameCount = 0;
            fpsTime = currentTime;
        }
        
        // 处理相机控制
        processCamera(camera, static_cast<float>(deltaTime));
        
        // 渲染场景
        renderer.clear(Vec4f(0.1f, 0.1f, 0.1f, 1.0f));
        // 添加简单的性能计时器
        auto startTime = std::chrono::high_resolution_clock::now();
        scene.render(renderer);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "渲染耗时: " << duration << "ms" << std::endl;
        
        // 复制帧缓冲区到平台
        copyFrameBufferToPlatform(renderer.getFrameBuffer());
        
        // 更新屏幕
        platform_update_framebuffer();
        
        // 处理截图
        if (platform_should_take_screenshot()) {
            saveToPPM(platform_get_screenshot_filename(), renderer.getFrameBuffer());
        }
        
        // 限制帧率
        if (deltaTime < 1.0 / 60.0) {
            platform_sleep(1.0 / 60.0 - deltaTime);
        }
    }
    
    // 清理资源
    platform_cleanup();
    
    std::cout << "渲染完成!" << std::endl;
    
    return 0;
}