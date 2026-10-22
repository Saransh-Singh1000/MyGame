#ifndef VULKAN_RENDERER_HPP
#define VULKAN_RENDERER_HPP

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#include "renderer/renderer.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class VulkanRenderer final : public Renderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    bool initialize(SDL_Window* window, std::string& error) override;
    bool render(const std::vector<VulkanVertex>& vertices, const std::vector<std::uint32_t>& indices,
                std::string& error) override;
    void shutdown() override;

private:
    struct Buffer {
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize capacity = 0;
    };

    struct Frame {
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
        VkFence inFlight = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        Buffer vertexBuffer;
        Buffer indexBuffer;
    };

    static constexpr std::uint32_t framesInFlight = 2;

    bool createInstance(std::string& error);
    bool selectPhysicalDevice(std::string& error);
    bool createLogicalDevice(std::string& error);
    bool createSwapchain(std::string& error);
    bool createRenderPass(std::string& error);
    bool createPipeline(std::string& error);
    bool createFramebuffers(std::string& error);
    bool createFrames(std::string& error);
    bool recreateSwapchain(std::string& error);
    bool ensureBuffer(Buffer& buffer, VkDeviceSize requiredSize, VkBufferUsageFlags usage,
                      std::string& error);
    std::uint32_t findMemoryType(std::uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties) const;
    void destroyBuffer(Buffer& buffer);
    void destroySwapchainResources();

    SDL_Window* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    std::uint32_t graphicsFamily_ = 0;
    std::uint32_t presentFamily_ = 0;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_{};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> imageViews_;
    std::vector<VkFramebuffer> framebuffers_;
    std::vector<VkFence> imagesInFlight_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::array<Frame, framesInFlight> frames_{};
    std::uint32_t currentFrame_ = 0;
    bool swapchainDirty_ = false;
};

#endif
