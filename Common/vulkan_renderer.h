#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#ifdef ANDROID
#include <vulkan/vulkan_android.h>
#endif
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declare for Android
#ifndef ANDROID
typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t count;
} VertexBuffer;

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
    VkCommandBuffer *command_buffers;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VertexBuffer vertex_buffer;
    uint32_t graphics_queue_family;
    VkSurfaceFormatKHR surface_format;
    VkExtent2D swapchain_extent;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    SDL_Surface *text_surface;
    VkImage text_image;
    VkDeviceMemory text_image_memory;
    VkImageView text_image_view;
} VulkanRenderer;
#else
// Minimal Android stub
typedef struct {
    int dummy;
} VulkanRenderer;
#endif

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
    const char **extensions = NULL;

    #ifdef SDL_PLATFORM_ANDROID
    // Android provides Vulkan extensions directly
    const char *android_extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
    };
    extensions = android_extensions;
    extension_count = 2;
    #else
    // Desktop platforms use SDL_Vulkan_GetInstanceExtensions
    if (!SDL_Vulkan_GetInstanceExtensions(&extension_count, NULL)) {
        fprintf(stderr, "Failed to get Vulkan extensions count\n");
        free(renderer);
        return NULL;
    }

    extensions = (const char **)malloc(sizeof(const char *) * extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(&extension_count, extensions)) {
        fprintf(stderr, "Failed to get Vulkan extensions\n");
        free(extensions);
        free(renderer);
        return NULL;
    }
    #endif

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = extension_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkResult result = vkCreateInstance(&instance_info, NULL, &renderer->instance);
    #ifndef SDL_PLATFORM_ANDROID
    free(extensions);
    #endif
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance: %s\n", vk_result_to_string(result));
        free(renderer);
        return NULL;
    #ifdef SDL_PLATFORM_ANDROID
    // For Android, use vkCreateAndroidSurfaceKHR
    VkAndroidSurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .window = NULL,  // Would be ANativeWindow* on actual Android
    };
    result = vkCreateAndroidSurfaceKHR(renderer->instance, &surface_info, NULL, &renderer->surface);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Android surface: %s\n", vk_result_to_string(result));
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }
    #else
    // Desktop: Use SDL_Vulkan_CreateSurface
    if (!SDL_Vulkan_CreateSurface(window, renderer->instance, NULL, &renderer->surface)) {
        fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }
    #endif

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
    renderer->graphics_queue_family = graphics_queue_family;

    // Get surface format
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &format_count, NULL);
    
    VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, renderer->surface, &format_count, formats);
    
    renderer->surface_format = formats[0];
    free(formats);

    // Get surface capabilities
    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, renderer->surface, &surface_caps);
    
    renderer->swapchain_extent = surface_caps.currentExtent;
    if (surface_caps.currentExtent.width == UINT32_MAX) {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        renderer->swapchain_extent.width = width;
        renderer->swapchain_extent.height = height;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = renderer->surface,
        .minImageCount = surface_caps.minImageCount,
        .imageFormat = renderer->surface_format.format,
        .imageColorSpace = renderer->surface_format.colorSpace,
        .imageExtent = renderer->swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphics_queue_family,
        .preTransform = surface_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };

    result = vkCreateSwapchainKHR(renderer->device, &swapchain_info, NULL, &renderer->swapchain);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain: %s\n", vk_result_to_string(result));
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchain_image_count, NULL);
    renderer->swapchain_images = (VkImage *)malloc(sizeof(VkImage) * renderer->swapchain_image_count);
    vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchain_image_count, renderer->swapchain_images);

    // Create image views
    renderer->image_views = (VkImageView *)malloc(sizeof(VkImageView) * renderer->swapchain_image_count);
    for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = renderer->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = renderer->surface_format.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        
        result = vkCreateImageView(renderer->device, &view_info, NULL, &renderer->image_views[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image view: %s\n", vk_result_to_string(result));
            vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
            vkDestroyDevice(renderer->device, NULL);
            vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
            vkDestroyInstance(renderer->instance, NULL);
            free(renderer);
            return NULL;
        }
    }

    // Create render pass
    VkAttachmentDescription color_attachment = {
        .format = renderer->surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    result = vkCreateRenderPass(renderer->device, &render_pass_info, NULL, &renderer->render_pass);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass: %s\n", vk_result_to_string(result));
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Create framebuffers
    renderer->framebuffers = (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * renderer->swapchain_image_count);
    for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
        VkImageView attachments[] = { renderer->image_views[i] };
        
        VkFramebufferCreateInfo framebuffer_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderer->render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = renderer->swapchain_extent.width,
            .height = renderer->swapchain_extent.height,
            .layers = 1,
        };
        
        result = vkCreateFramebuffer(renderer->device, &framebuffer_info, NULL, &renderer->framebuffers[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer: %s\n", vk_result_to_string(result));
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(renderer->device, renderer->framebuffers[j], NULL);
            }
            vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
            for (uint32_t j = 0; j < renderer->swapchain_image_count; j++) {
                vkDestroyImageView(renderer->device, renderer->image_views[j], NULL);
            }
            vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
            vkDestroyDevice(renderer->device, NULL);
            vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
            vkDestroyInstance(renderer->instance, NULL);
            free(renderer);
            return NULL;
        }
    }

    // Create command pool
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family,
    };

    result = vkCreateCommandPool(renderer->device, &pool_info, NULL, &renderer->command_pool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool: %s\n", vk_result_to_string(result));
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Allocate command buffers
    renderer->command_buffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * renderer->swapchain_image_count);
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderer->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = renderer->swapchain_image_count,
    };

    result = vkAllocateCommandBuffers(renderer->device, &alloc_info, renderer->command_buffers);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers: %s\n", vk_result_to_string(result));
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    result = vkCreatePipelineLayout(renderer->device, &pipeline_layout_info, NULL, &renderer->pipeline_layout);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout: %s\n", vk_result_to_string(result));
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Create graphics pipeline with simple shaders
    // Vertex shader (compiled SPIR-V - generates a square)
    uint32_t vert_shader_code[] = {
        0x07230203, 0x00010300, 0x0008000A, 0x0000001F, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0008000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000b, 0x0000000d, 0x00000011,
        0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050006,
        0x0000000b, 0x00000000, 0x6f506f43, 0x00000073, 0x00030005, 0x0000000b, 0x00000000, 0x00050005,
        0x0000000d, 0x4c475f67, 0x61507f5f, 0x00737365, 0x00050006, 0x0000000d, 0x00000000, 0x6f506f43,
        0x00000073, 0x00040005, 0x00000011, 0x78646e69, 0x00000000, 0x00050048, 0x0000000b, 0x00000000,
        0x0000000b, 0x00000000, 0x00030047, 0x0000000b, 0x00000002, 0x00050048, 0x0000000d, 0x00000000,
        0x0000000b, 0x00000000, 0x00030047, 0x0000000d, 0x00000002, 0x00040047, 0x00000011, 0x0000000b,
        0x0000001a, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
        0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00040020, 0x00000008, 0x00000003,
        0x00000007, 0x0004003b, 0x00000008, 0x0000000b, 0x00000003, 0x00040017, 0x00000009, 0x00000006,
        0x00000004, 0x00040020, 0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000d,
        0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x00040020, 0x0000000f, 0x00000001,
        0x0000000e, 0x0004003b, 0x0000000f, 0x00000011, 0x00000001, 0x0004002b, 0x0000000e, 0x00000013,
        0x00000000, 0x0004002b, 0x00000006, 0x00000016, 0xbf000000, 0x0004002b, 0x00000006, 0x00000018,
        0x3f000000, 0x00080036, 0x00000007, 0x00000014, 0x00000016, 0x00000016, 0x00000000, 0x00000000,
        0x00000000, 0x0004003d, 0x0000000e, 0x00000015, 0x00000011, 0x000300A6, 0x00000007, 0x00000014,
        0x0004006e, 0x0000000e, 0x00000015, 0x00000004, 0x000200FE, 0x00000009, 0x00010038,
    };
    uint32_t vert_shader_size = sizeof(vert_shader_code);

    // Fragment shader (compiled SPIR-V - outputs red color)
    uint32_t frag_shader_code[] = {
        0x07230203, 0x00010300, 0x0008000A, 0x0000000D, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x00030010,
        0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
        0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x00000000, 0x00040005, 0x0000000b, 0x50000000,
        0x00000000, 0x00050048, 0x00000009, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x00000009,
        0x00000002, 0x00040047, 0x0000000b, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021,
        0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
        0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009,
        0x00000003, 0x00040020, 0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b,
        0x00000001, 0x0004002b, 0x00000006, 0x0000000c, 0x3f800000, 0x0004002b, 0x00000006, 0x0000000d,
        0x00000000, 0x0008004b, 0x00000007, 0x0000000e, 0x0000000c, 0x0000000d, 0x0000000d, 0x0000000c,
        0x00000000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
        0x0003003e, 0x00000009, 0x0000000e, 0x000100fd, 0x00010038,
    };
    uint32_t frag_shader_size = sizeof(frag_shader_code);

    VkShaderModuleCreateInfo vert_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vert_shader_size,
        .pCode = vert_shader_code,
    };

    VkShaderModule vert_shader_module;
    result = vkCreateShaderModule(renderer->device, &vert_shader_info, NULL, &vert_shader_module);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader module: %s\n", vk_result_to_string(result));
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    VkShaderModuleCreateInfo frag_shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = frag_shader_size,
        .pCode = frag_shader_code,
    };

    VkShaderModule frag_shader_module;
    result = vkCreateShaderModule(renderer->device, &frag_shader_info, NULL, &frag_shader_module);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader module: %s\n", vk_result_to_string(result));
        vkDestroyShaderModule(renderer->device, vert_shader_module, NULL);
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    VkPipelineShaderStageCreateInfo vert_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)renderer->swapchain_extent.width,
        .height = (float)renderer->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = renderer->swapchain_extent,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .layout = renderer->pipeline_layout,
        .renderPass = renderer->render_pass,
        .subpass = 0,
    };

    result = vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &renderer->pipeline);
    vkDestroyShaderModule(renderer->device, vert_shader_module, NULL);
    vkDestroyShaderModule(renderer->device, frag_shader_module, NULL);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline: %s\n", vk_result_to_string(result));
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Create synchronization primitives
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    result = vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &renderer->image_available_semaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image available semaphore: %s\n", vk_result_to_string(result));
        vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    result = vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &renderer->render_finished_semaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render finished semaphore: %s\n", vk_result_to_string(result));
        vkDestroySemaphore(renderer->device, renderer->image_available_semaphore, NULL);
        vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    result = vkCreateFence(renderer->device, &fence_info, NULL, &renderer->in_flight_fence);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create in-flight fence: %s\n", vk_result_to_string(result));
        vkDestroySemaphore(renderer->device, renderer->render_finished_semaphore, NULL);
        vkDestroySemaphore(renderer->device, renderer->image_available_semaphore, NULL);
        vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
        vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyFramebuffer(renderer->device, renderer->framebuffers[i], NULL);
        }
        vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
        for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
            vkDestroyImageView(renderer->device, renderer->image_views[i], NULL);
        }
        vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
        vkDestroyDevice(renderer->device, NULL);
        vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
        vkDestroyInstance(renderer->instance, NULL);
        free(renderer);
        return NULL;
    }

    // Record command buffers for rendering
    for (uint32_t i = 0; i < renderer->swapchain_image_count; i++) {
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        };

        vkBeginCommandBuffer(renderer->command_buffers[i], &begin_info);

        VkRenderPassBeginInfo render_pass_info_cmd = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderer->render_pass,
            .framebuffer = renderer->framebuffers[i],
            .renderArea = {
                .offset = {0, 0},
                .extent = renderer->swapchain_extent,
            },
            .clearValueCount = 1,
            .pClearValues = &(VkClearValue){{0.0f, 0.0f, 0.0f, 1.0f}},
        };

        vkCmdBeginRenderPass(renderer->command_buffers[i], &render_pass_info_cmd, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(renderer->command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline);

        vkCmdDraw(renderer->command_buffers[i], 4, 1, 0, 0);
        vkCmdEndRenderPass(renderer->command_buffers[i]);

        result = vkEndCommandBuffer(renderer->command_buffers[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to record command buffer: %s\n", vk_result_to_string(result));
            vkDestroyFence(renderer->device, renderer->in_flight_fence, NULL);
            vkDestroySemaphore(renderer->device, renderer->render_finished_semaphore, NULL);
            vkDestroySemaphore(renderer->device, renderer->image_available_semaphore, NULL);
            vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
            vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
            vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
            for (uint32_t j = 0; j < renderer->swapchain_image_count; j++) {
                vkDestroyFramebuffer(renderer->device, renderer->framebuffers[j], NULL);
            }
            vkDestroyRenderPass(renderer->device, renderer->render_pass, NULL);
            for (uint32_t j = 0; j < renderer->swapchain_image_count; j++) {
                vkDestroyImageView(renderer->device, renderer->image_views[j], NULL);
            }
            vkDestroySwapchainKHR(renderer->device, renderer->swapchain, NULL);
            vkDestroyDevice(renderer->device, NULL);
            vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
            vkDestroyInstance(renderer->instance, NULL);
            free(renderer);
            return NULL;
        }
    }

    fprintf(stderr, "Vulkan initialized successfully\n");
    return renderer;
}

static void vulkan_renderer_destroy(VulkanRenderer *renderer)
{
    if (!renderer) return;

    #ifdef ANDROID
    free(renderer);
    return;
    #else

    if (renderer->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(renderer->device);
        
        if (renderer->in_flight_fence != VK_NULL_HANDLE) {
            vkDestroyFence(renderer->device, renderer->in_flight_fence, NULL);
        }

        if (renderer->render_finished_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(renderer->device, renderer->render_finished_semaphore, NULL);
        }

        if (renderer->image_available_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(renderer->device, renderer->image_available_semaphore, NULL);
        }

        if (renderer->pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(renderer->device, renderer->pipeline, NULL);
        }

        if (renderer->pipeline_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(renderer->device, renderer->pipeline_layout, NULL);
        }

        if (renderer->command_pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(renderer->device, renderer->command_pool, NULL);
        }

        if (renderer->command_buffers) {
            free(renderer->command_buffers);
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
    #endif
}

static void vulkan_renderer_draw(VulkanRenderer *renderer)
{
    if (!renderer || !renderer->device) return;

    vkWaitForFences(renderer->device, 1, &renderer->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(renderer->device, 1, &renderer->in_flight_fence);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(renderer->device, renderer->swapchain, UINT64_MAX, 
                                           renderer->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire next image: %s\n", vk_result_to_string(result));
        return;
    }

    VkSemaphore wait_semaphores[] = { renderer->image_available_semaphore };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[] = { renderer->render_finished_semaphore };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &renderer->command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores,
    };

    result = vkQueueSubmit(renderer->queue, 1, &submit_info, renderer->in_flight_fence);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit command buffer: %s\n", vk_result_to_string(result));
        return;
    }

    VkSwapchainKHR swapchains[] = { renderer->swapchain };
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
    };

    result = vkQueuePresentKHR(renderer->queue, &present_info);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to present: %s\n", vk_result_to_string(result));
    }
}

#endif
