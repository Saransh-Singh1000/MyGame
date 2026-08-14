#include "init.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// SDL_main automatically binds with org.libsdl.app.SDLActivity
int main(int argc, char* argv[]) {
    SDL_Log("Initializing Engine on Android...");

    // Call your engine's entry function
    if (!Engine::run()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Engine execution failed!");
        return -1;
    }

    SDL_Log("Engine shut down cleanly.");
    return 0;
}