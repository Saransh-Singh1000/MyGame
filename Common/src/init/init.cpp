#include "init.hpp"
#include <SDL3/SDL.h>
#include <iostream>

namespace Engine {

bool run() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Engine Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

} // namespace Engine