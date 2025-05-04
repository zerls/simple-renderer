#include "camera.h" // 引入独立的相机头文件
#include "../platform/platform.h"
#include <algorithm>

// 处理相机移动
void processCamera(Camera &camera, float deltaTime)
{
    const float moveSpeed = 3.0f * deltaTime;
    const float rotateSpeed = 3.0f * deltaTime;
    const float zoomSpeed = 5.0f * deltaTime;

    Vec3f position = camera.getPosition();
    Vec3f target = camera.getTarget();
    Vec3f up = camera.getUp();

    // 计算前向、右向和上向量
    Vec3f forward = normalize(target - position);
    Vec3f right = normalize(cross(forward, up));
    Vec3f upDir = normalize(cross(right, forward));


    // 应用移动
    Vec3f movement(0, 0, 0);
    
    if (platform_get_key(PLATFORM_KEY_W)) movement = movement + forward * moveSpeed;
    if (platform_get_key(PLATFORM_KEY_S)) movement = movement + forward * (-moveSpeed);
    if (platform_get_key(PLATFORM_KEY_D)) movement = movement + right * moveSpeed;
    if (platform_get_key(PLATFORM_KEY_A)) movement = movement + right * (-moveSpeed);
    if (platform_get_key(PLATFORM_KEY_Q)) movement = movement + upDir * moveSpeed;
    if (platform_get_key(PLATFORM_KEY_E)) movement = movement + upDir * (-moveSpeed);
    
    // 同时移动位置和目标点，保持相机朝向
    position = position + movement;
    target = target + movement;

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
