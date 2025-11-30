#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return 1;
    }

    // 创建一个 800x600 的窗口（窗口模式）。如需全屏，可把最后一个参数改为 SDL_WINDOW_FULLSCREEN 并处理显示模式。
    SDL_Window* win = SDL_CreateWindow("123",
                                       800, 600,
                                       SDL_WINDOW_FULLSCREEN);
    if (!win) {
        SDL_Log("SDL_CreateWindow error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 简单事件循环，按窗口关闭或按 ESC 退出
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                
            }
        }
        SDL_Delay(16); // 限制循环频率 ~60Hz
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}