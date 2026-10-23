#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

struct VulkanVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool initialize(SDL_Window* window, std::string& error, bool verticalSync) = 0;
    virtual bool render(const std::vector<VulkanVertex>& vertices,
                        const std::vector<std::uint32_t>& indices,
                        std::string& error) = 0;
    virtual void shutdown() = 0;
};

#endif
