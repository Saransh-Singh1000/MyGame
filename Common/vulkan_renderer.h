#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    SDL_Window *window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    uint32_t swapchain_image_count;
    VkImageView *image_views;
    VkRenderPass render_pass;
    VkFramebuffer *framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    SDL_Surface *text_surface;
    VkImage text_image;
    VkDeviceMemory text_image_memory;
    VkImageView text_image_view;
} VulkanRenderer;

static const char *vk_result_to_string(VkResult result)
{
    switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        default: return "UNKNOWN_RESULT";
    }
}

static VulkanRenderer *vulkan_renderer_create(SDL_Window *window)
{
    VulkanRenderer *renderer = (VulkanRenderer *)malloc(sizeof(VulkanRenderer));
    if (!renderer) {
        fprintf(stderr, "Failed to allocate renderer\n");
        return NULL;
    }

    renderer->window = window;
    renderer->instance = VK_NULL_HANDLE;
    renderer->surface = VK_NULL_HANDLE;
    renderer->physical_device = VK_NULL_HANDLE;
    renderer->device = VK_NULL_HANDLE;
    renderer->queue = VK_NULL_HANDLE;
    renderer->swapchain = VK_NULL_HANDLE;
    renderer->swapchain_images = NULL;
    renderer->swapchain_image_count = 0;
    renderer->image_views = NULL;
    renderer->render_pass = VK_NULL_HANDLE;
    renderer->framebuffers = NULL;
    renderer->command_pool = VK_NULL_HANDLE;
    renderer->command_buffer = VK_NULL_HANDLE;

    // Create Vulkan instance
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "MyGame",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "MyEngine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    uint32_t extension_count = 0;
    const char **extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkResult result = vkCreateInstance(&instance_info, NULL, &renderer->instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance: %s\n", vk_result_to_string(result));
        free(renderer);
        return NULL;
    }

    // Create surface
    if (!SDL_Vulkan_CreateSurface(window, renderer->instance, NULL, &renderer->surface)) {
        fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Find physical device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "No Vulkan devices found\n");
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    VkPhysicalDevice *devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, devices);
    renderer->physical_device = devices[0];
    free(devices);

    // Find queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_count, queue_families);

    uint32_t graphics_queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family = i;
            break;
        }
    }
    free(queue_families);

    if (graphics_queue_family == UINT32_MAX) {
        fprintf(stderr, "No graphics queue family found\n");
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Create logical device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphics_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions,
    };

    result = vkCreateDevice(renderer->physical_device, &device_info, NULL, &renderer->device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device: %s\n", vk_result_to_string(result));
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    vkGetDeviceQueue(renderer->device, graphics_queue_family, 0, &renderer->queue);

    fprintf(stderr, "Vulkan initialized successfully\n");
    return renderer;
}

static void vulkan_renderer_destroy(VulkanRenderer *renderer)
{
    if (!renderer) return;

    if (renderer->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(renderer->device);
        
        if (renderer->command_pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        }

        if (renderer->render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        }

        if (renderer->framebuffers) {
            for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
                vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
            }
            free(renderer->framebuffers);
        }

        if (renderer->image_views) {
            for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
                vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
            }
            free(renderer->image_views);
        }

        if (renderer->swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        }

        if (renderer->swapchain_images) {
            free(renderer->swapchain_images);
        }

        vkDestroyDevice(renderer->device, NULL);
    }

    if (renderer->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
    }

    if (renderer->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(renderer->instance, NULL);
    }

    if (renderer->text_surface) {
        SDL_DestroySurface(renderer->text_surface);
    }

    free(renderer);
}

static void vulkan_renderer_draw(VulkanRenderer *renderer)
{
    if (!renderer || !renderer->device) return;
    vkDeviceWaitIdle(renderer->device);
}

#endif
