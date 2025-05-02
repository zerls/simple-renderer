#pragma once

#include "scene.h"

// 场景类型枚举
enum class SceneType {
    DEFAULT,
    SPHERES,
    CUBES
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    // 初始化指定类型的场景
    void initializeScene(SceneType type, Scene& scene, int width, int height);

private:
    // 初始化默认场景
    void initDefaultScene(Scene& scene, int width, int height);
    
    // 初始化球体场景
    void initSpheresScene(Scene& scene, int width, int height);
    
    // 初始化立方体场景
    void initCubesScene(Scene& scene, int width, int height);
};