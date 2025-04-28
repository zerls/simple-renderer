#include "scene.h"
#include "texture_io.h" // 替换 test.h

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


//TODO 代码的框架目前需要优化，Materials,Shaders 组织有些混乱，Renderer中需要将部分功能进行封装
int main()
{
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // 创建渲染器
    Renderer renderer(WIDTH, HEIGHT);
    renderer.enableMSAA(true); // 启用 MSAA

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
    // 渲染场景
    scene.setupShadowMapping(true);
    scene.render(renderer);

    // 保存结果
    saveToPPM("../output/scene_MSAA.ppm",renderer.getFrameBuffer());

    std::cout << "渲染完成!" << std::endl;

    return 0;
}