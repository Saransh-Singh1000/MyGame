#include "renderer/opengl_renderer.hpp"

#if defined(MYGAME_ENABLE_OPENGL) && MYGAME_ENABLE_OPENGL
#include <SDL3/SDL_opengl.h>
#include <cstddef>
#include <limits>
#include <string>
#endif

OpenGLRenderer::~OpenGLRenderer() {
    shutdown();
}

bool OpenGLRenderer::initialize(SDL_Window* window, std::string& error) {
#if defined(MYGAME_ENABLE_OPENGL) && MYGAME_ENABLE_OPENGL
    window_ = window;
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
        error = std::string("SDL_GL_CreateContext failed: ") + SDL_GetError();
        shutdown();
        return false;
    }
    if (!SDL_GL_MakeCurrent(window_, context_)) {
        error = std::string("SDL_GL_MakeCurrent failed: ") + SDL_GetError();
        shutdown();
        return false;
    }
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                    "Could not enable OpenGL vertical synchronization: %s", SDL_GetError());
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearColor(0.055f, 0.071f, 0.125f, 1.0f);
    return true;
#else
    static_cast<void>(window);
    error = "OpenGL rendering is unavailable on this platform";
    return false;
#endif
}

bool OpenGLRenderer::render(const std::vector<VulkanVertex>& vertices,
                            const std::vector<std::uint32_t>& indices,
                            std::string& error) {
#if defined(MYGAME_ENABLE_OPENGL) && MYGAME_ENABLE_OPENGL
    if (indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        error = "OpenGL index count exceeds the supported draw size";
        return false;
    }

    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height)) {
        error = std::string("SDL_GetWindowSizeInPixels failed: ") + SDL_GetError();
        return false;
    }
    if (width <= 0 || height <= 0 || vertices.empty() || indices.empty()) {
        return true;
    }

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(VulkanVertex), &vertices.front().x);
    glColorPointer(4, GL_FLOAT, sizeof(VulkanVertex), &vertices.front().r);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT,
                   indices.data());
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    SDL_GL_SwapWindow(window_);
    return true;
#else
    static_cast<void>(vertices);
    static_cast<void>(indices);
    error = "OpenGL rendering is unavailable on this platform";
    return false;
#endif
}

void OpenGLRenderer::shutdown() {
#if defined(MYGAME_ENABLE_OPENGL) && MYGAME_ENABLE_OPENGL
    if (context_) {
        SDL_GL_DestroyContext(context_);
        context_ = nullptr;
    }
#endif
    window_ = nullptr;
}
