#ifndef INIT_H
#define INIT_H

#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include "vulkan_renderer.h"

static int init_window(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "MyGame - Vulkan Test",
        1280,
        720,
        SDL_WINDOW_VULKAN
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize Vulkan renderer
    VulkanRenderer *renderer = vulkan_renderer_create(window);
    if (!renderer) {
        fprintf(stderr, "Failed to initialize Vulkan renderer\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    fprintf(stderr, "Vulkan renderer initialized successfully\n");
    fprintf(stderr, "Display: Hello Motherf***er\n");

    bool running = true;
    int frame_count = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        // Render frame
        vulkan_renderer_draw(renderer);

        frame_count++;
        if (frame_count % 60 == 0) {
            fprintf(stderr, "Vulkan rendering... (frames: %d)\n", frame_count);
        }

        // Simple frame delay to avoid 100% CPU usage
        SDL_Delay(16);
    }

    vulkan_renderer_destroy(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#endif
