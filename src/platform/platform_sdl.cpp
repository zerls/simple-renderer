#include "platform.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SDL 相关变量
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* framebuffer_texture = NULL;
static uint32_t* framebuffer = NULL;
static int window_width = 0;
static int window_height = 0;
static bool should_close = false;
static uint64_t performance_frequency = 0;

// 相机控制相关变量
static bool keys[512] = {false};
static bool mouse_buttons[5] = {false};
static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_delta_x = 0;
static int mouse_delta_y = 0;
static bool mouse_moved = false;
static float mouse_wheel_x = 0.0f;
static float mouse_wheel_y = 0.0f;
static bool mouse_wheel_moved = false;

// 截图相关变量
static bool take_screenshot = false;
static char screenshot_filename[256] = "../output/screenshot.ppm";

bool platform_init(const char* title, int width, int height) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL 初始化失败: %s\n", SDL_GetError());
        return false;
    }

    // 创建窗口
    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        fprintf(stderr, "窗口创建失败: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // 创建渲染器
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "渲染器创建失败: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // 创建纹理作为帧缓冲
    framebuffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );
    
    if (!framebuffer_texture) {
        fprintf(stderr, "纹理创建失败: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // 分配帧缓冲内存
    framebuffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if (!framebuffer) {
        fprintf(stderr, "帧缓冲内存分配失败\n");
        SDL_DestroyTexture(framebuffer_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // 初始化帧缓冲为黑色
    memset(framebuffer, 0, width * height * sizeof(uint32_t));
    
    // 保存窗口尺寸
    window_width = width;
    window_height = height;
    
    // 获取性能计数器频率（用于时间函数）
    performance_frequency = SDL_GetPerformanceFrequency();
    
    // 启用相对鼠标模式（用于相机控制）
    SDL_SetRelativeMouseMode(SDL_FALSE);
    
    return true;
}

void platform_cleanup(void) {
    if (framebuffer) {
        free(framebuffer);
        framebuffer = NULL;
    }
    
    if (framebuffer_texture) {
        SDL_DestroyTexture(framebuffer_texture);
        framebuffer_texture = NULL;
    }
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    SDL_Quit();
}

void platform_set_title(const char* title) {
    if (window) {
        SDL_SetWindowTitle(window, title);
    }
}

void platform_get_size(int* width, int* height) {
    if (width) *width = window_width;
    if (height) *height = window_height;
}

bool platform_should_close(void) {
    return should_close;
}

void* platform_get_framebuffer(void) {
    return framebuffer;
}

void platform_update_framebuffer(void) {
    if (!renderer || !framebuffer_texture || !framebuffer) {
        return;
    }
    
    // 更新纹理
    SDL_UpdateTexture(
        framebuffer_texture,
        NULL,
        framebuffer,
        window_width * sizeof(uint32_t)
    );
    
    // 清除渲染器
    SDL_RenderClear(renderer);
    
    // 复制纹理到渲染器
    SDL_RenderCopy(renderer, framebuffer_texture, NULL, NULL);
    
    // 呈现
    SDL_RenderPresent(renderer);
}

void platform_process_events(void) {
    // 重置鼠标增量
    mouse_delta_x = 0;
    mouse_delta_y = 0;
    mouse_moved = false;
    mouse_wheel_x = 0.0f;
    mouse_wheel_y = 0.0f;
    mouse_wheel_moved = false;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_close = true;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int new_width = event.window.data1;
                    int new_height = event.window.data2;
                    
                    // 重新创建纹理
                    if (framebuffer_texture) {
                        SDL_DestroyTexture(framebuffer_texture);
                    }
                    
                    framebuffer_texture = SDL_CreateTexture(
                        renderer,
                        SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING,
                        new_width, new_height
                    );
                    
                    // 重新分配帧缓冲
                    if (framebuffer) {
                        free(framebuffer);
                    }
                    
                    framebuffer = (uint32_t*)malloc(new_width * new_height * sizeof(uint32_t));
                    if (framebuffer) {
                        memset(framebuffer, 0, new_width * new_height * sizeof(uint32_t));
                    }
                    
                    // 更新尺寸
                    window_width = new_width;
                    window_height = new_height;
                }
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode < 512) {
                    keys[event.key.keysym.scancode] = true;
                    
                    // 按下F2截图
                    if (event.key.keysym.scancode == SDL_SCANCODE_F2) {
                        take_screenshot = true;
                    }
                    
                    // 按下ESC退出
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        should_close = true;
                    }
                    
                    // 按下空格键切换鼠标模式
                    if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                        SDL_bool relative_mode = SDL_GetRelativeMouseMode();
                        SDL_SetRelativeMouseMode(relative_mode ? SDL_FALSE : SDL_TRUE);
                    }
                }
                break;
                
            case SDL_KEYUP:
                if (event.key.keysym.scancode < 512) {
                    keys[event.key.keysym.scancode] = false;
                }
                break;
                
            case SDL_MOUSEMOTION:
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                mouse_delta_x = event.motion.xrel;
                mouse_delta_y = event.motion.yrel;
                mouse_moved = true;
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button <= 5) {
                    mouse_buttons[event.button.button - 1] = true;
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button <= 5) {
                    mouse_buttons[event.button.button - 1] = false;
                }
                break;
                
            case SDL_MOUSEWHEEL:
                mouse_wheel_x = event.wheel.x;
                mouse_wheel_y = event.wheel.y;
                mouse_wheel_moved = true;
                break;
                
            default:
                break;
        }
    }
}

// 获取按键状态
bool platform_get_key(int key) {
    if (key >= 0 && key < 512) {
        return keys[key];
    }
    return false;
}

// 获取鼠标按钮状态
bool platform_get_mouse_button(int button) {
    if (button >= 0 && button < 5) {
        return mouse_buttons[button];
    }
    return false;
}

// 获取鼠标位置
void platform_get_mouse_position(int* x, int* y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
}

// 获取鼠标移动增量
void platform_get_mouse_delta(int* dx, int* dy) {
    if (dx) *dx = mouse_delta_x;
    if (dy) *dy = mouse_delta_y;
}

// 获取鼠标滚轮增量
void platform_get_mouse_wheel(float* x, float* y) {
    if (x) *x = mouse_wheel_x;
    if (y) *y = mouse_wheel_y;
}

// 检查是否需要截图
bool platform_should_take_screenshot(void) {
    bool result = take_screenshot;
    take_screenshot = false;
    return result;
}

// 设置截图文件名
void platform_set_screenshot_filename(const char* filename) {
    if (filename) {
        strncpy(screenshot_filename, filename, sizeof(screenshot_filename) - 1);
        screenshot_filename[sizeof(screenshot_filename) - 1] = '\0';
    }
}

// 获取截图文件名
const char* platform_get_screenshot_filename(void) {
    return screenshot_filename;
}

double platform_get_time(void) {
    return (double)SDL_GetPerformanceCounter() / (double)performance_frequency;
}

void platform_sleep(double seconds) {
    SDL_Delay((Uint32)(seconds * 1000.0));
}