#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 平台初始化和清理
bool platform_init(const char* title, int width, int height);
void platform_cleanup(void);

// 窗口管理
void platform_set_title(const char* title);
void platform_get_size(int* width, int* height);
bool platform_should_close(void);

// 渲染相关
void* platform_get_framebuffer(void);
void platform_update_framebuffer(void);

// 事件处理
void platform_process_events(void);

// 输入处理
bool platform_get_key(int key);
bool platform_get_mouse_button(int button);
void platform_get_mouse_position(int* x, int* y);
void platform_get_mouse_delta(int* dx, int* dy);
void platform_get_mouse_wheel(float* x, float* y);

// 截图相关
bool platform_should_take_screenshot(void);
void platform_set_screenshot_filename(const char* filename);
const char* platform_get_screenshot_filename(void);

// 时间相关
double platform_get_time(void);
void platform_sleep(double seconds);

// SDL 按键常量定义
#define PLATFORM_KEY_W 26
#define PLATFORM_KEY_A 4
#define PLATFORM_KEY_S 22
#define PLATFORM_KEY_D 7
#define PLATFORM_KEY_Q 20
#define PLATFORM_KEY_E 8
#define PLATFORM_KEY_UP 82
#define PLATFORM_KEY_DOWN 81
#define PLATFORM_KEY_LEFT 80
#define PLATFORM_KEY_RIGHT 79
#define PLATFORM_KEY_SPACE 44
#define PLATFORM_KEY_LSHIFT 225
#define PLATFORM_KEY_ESCAPE 41
#define PLATFORM_KEY_F1 58
#define PLATFORM_KEY_F2 59

// 鼠标按钮常量
#define PLATFORM_MOUSE_LEFT 0
#define PLATFORM_MOUSE_RIGHT 1
#define PLATFORM_MOUSE_MIDDLE 2

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H