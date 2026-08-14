#include "init.hpp"
#include <SDL3/SDL.h>
#include <iostream>

namespace Engine {

bool run() {
    // 1. SDL_Init returns true on success in SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 2. SDL_CreateWindow in SDL3 takes (title, width, height, flags)
    // 3. SDL_WINDOW_SHOWN is no longer needed (windows are shown by default)
    SDL_Window* window = SDL_CreateWindow(
        "Engine Window",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    bool isRunning = true;
    SDL_Event event;

    // Simple event loop
    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            // 4. SDL_QUIT is now SDL_EVENT_QUIT in SDL3
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

} // namespace Engine