#ifndef OPENGL_RENDERER_HPP
#define OPENGL_RENDERER_HPP

#include "renderer/renderer.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <vector>

class OpenGLRenderer final : public Renderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

    bool initialize(SDL_Window* window, std::string& error, bool verticalSync) override;
    bool render(const std::vector<VulkanVertex>& vertices,
                const std::vector<std::uint32_t>& indices,
                std::string& error) override;
    void shutdown() override;

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
};

#endif
