#include "renderer/vulkan_renderer.hpp"

#include "scene_vert.hpp"
#include "scene_frag.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr std::array<const char*, 1> deviceExtensions{{VK_KHR_SWAPCHAIN_EXTENSION_NAME}};
constexpr std::array<VkVertexInputAttributeDescription, 2> vertexAttributes{{
    {0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(VulkanVertex, x))},
    {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<std::uint32_t>(offsetof(VulkanVertex, r))}
}};

std::string vulkanError(const char* operation, VkResult result) {
    return std::string(operation) + " failed with VkResult " + std::to_string(result);
}

bool hasDeviceExtension(VkPhysicalDevice device, const char* extensionName) {
    std::uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) {
        return false;
    }
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) {
        return false;
    }
    return std::any_of(extensions.begin(), extensions.end(), [extensionName](const auto& extension) {
        return std::strcmp(extension.extensionName, extensionName) == 0;
    });
}

bool findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
                      std::uint32_t& graphicsFamily, std::uint32_t& presentFamily) {
    std::uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    bool hasGraphics = false;
    bool hasPresent = false;
    for (std::uint32_t i = 0; i < count; ++i) {
        const bool supportsGraphics = (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        if (supportsGraphics) {
            graphicsFamily = i;
            hasGraphics = true;
        }
        VkBool32 supportsPresent = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent) == VK_SUCCESS &&
            supportsPresent == VK_TRUE) {
            presentFamily = i;
            hasPresent = true;
        }
        if (supportsGraphics && supportsPresent == VK_TRUE) {
            graphicsFamily = i;
            presentFamily = i;
            return true;
        }
    }
    return hasGraphics && hasPresent;
}

bool hasSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    std::uint32_t formatCount = 0;
    std::uint32_t presentModeCount = 0;
    return vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) == VK_SUCCESS &&
           vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr) ==
               VK_SUCCESS &&
           formatCount > 0 && presentModeCount > 0;
}

} // namespace

VulkanRenderer::~VulkanRenderer() {
    shutdown();
}

bool VulkanRenderer::initialize(SDL_Window* window, std::string& error) {
    window_ = window;
    if (!createInstance(error)) {
        shutdown();
        return false;
    }
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
        error = std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError();
        shutdown();
        return false;
    }
    if (!selectPhysicalDevice(error) || !createLogicalDevice(error) ||
        !createSwapchain(error) || !createRenderPass(error) || !createPipeline(error) ||
        !createFramebuffers(error) || !createFrames(error)) {
        shutdown();
        return false;
    }
    return true;
}

bool VulkanRenderer::createInstance(std::string& error) {
    std::uint32_t extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensions) {
        error = std::string("SDL_Vulkan_GetInstanceExtensions failed: ") + SDL_GetError();
        return false;
    }

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "MyGame";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "MyGame";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateInstance", result);
        return false;
    }
    return true;
}

bool VulkanRenderer::selectPhysicalDevice(std::string& error) {
    std::uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (result != VK_SUCCESS || deviceCount == 0) {
        error = result == VK_SUCCESS ? "No Vulkan-capable GPU was found"
                                     : vulkanError("vkEnumeratePhysicalDevices", result);
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkEnumeratePhysicalDevices", result);
        return false;
    }

    std::int64_t bestScore = std::numeric_limits<std::int64_t>::min();
    std::uint32_t bestGraphicsFamily = 0;
    std::uint32_t bestPresentFamily = 0;
    for (const VkPhysicalDevice candidate : devices) {
        std::uint32_t graphicsFamily = 0;
        std::uint32_t presentFamily = 0;
        if (findQueueFamilies(candidate, surface_, graphicsFamily, presentFamily) &&
            hasDeviceExtension(candidate, deviceExtensions[0]) &&
            hasSwapchainSupport(candidate, surface_)) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            std::int64_t score = 0;
            switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score = 10000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score = 5000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score = 3000;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score = 1000;
                break;
            default:
                break;
            }
            score += static_cast<std::int64_t>(
                std::min(properties.limits.maxImageDimension2D, 8192u) / 1024u);
            if (graphicsFamily == presentFamily) {
                score += 100;
            }
            if (score > bestScore) {
                physicalDevice_ = candidate;
                bestGraphicsFamily = graphicsFamily;
                bestPresentFamily = presentFamily;
                bestScore = score;
            }
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        error = "No Vulkan GPU supports graphics, presentation, and swapchains";
        return false;
    }
    graphicsFamily_ = bestGraphicsFamily;
    presentFamily_ = bestPresentFamily;
    return true;
}

bool VulkanRenderer::createLogicalDevice(std::string& error) {
    const std::array<std::uint32_t, 2> families{{graphicsFamily_, presentFamily_}};
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(families.size());
    const float priority = 1.0f;
    for (std::size_t i = 0; i < families.size(); ++i) {
        if (i > 0 && families[i] == families[0]) {
            continue;
        }
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = families[i];
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures = &features;

    const VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateDevice", result);
        return false;
    }
    vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentFamily_, 0, &presentQueue_);
    return true;
}

bool VulkanRenderer::createSwapchain(std::string& error) {
    const std::array<std::uint32_t, 2> families{{graphicsFamily_, presentFamily_}};
    VkSurfaceCapabilitiesKHR capabilities{};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_,
                                                                 &capabilities);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);
        return false;
    }

    std::uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    if (result != VK_SUCCESS || formatCount == 0) {
        error = result == VK_SUCCESS ? "The Vulkan surface has no supported formats"
                                     : vulkanError("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        return false;
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount,
                                                  formats.data());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        return false;
    }
    VkSurfaceFormatKHR surfaceFormat = formats.front();
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }

    std::uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount,
                                                       nullptr);
    if (result != VK_SUCCESS || presentModeCount == 0) {
        error = result == VK_SUCCESS ? "The Vulkan surface has no supported presentation modes"
                                     : vulkanError("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        return false;
    }
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount,
                                                       presentModes.data());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        return false;
    }
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) !=
        presentModes.end()) {
        presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    } else if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) !=
               presentModes.end()) {
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        swapchainExtent_ = capabilities.currentExtent;
    } else {
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(window_, &width, &height)) {
            error = std::string("SDL_GetWindowSizeInPixels failed: ") + SDL_GetError();
            return false;
        }
        swapchainExtent_.width = std::clamp(static_cast<std::uint32_t>(std::max(width, 0)),
                                             capabilities.minImageExtent.width,
                                             capabilities.maxImageExtent.width);
        swapchainExtent_.height = std::clamp(static_cast<std::uint32_t>(std::max(height, 0)),
                                              capabilities.minImageExtent.height,
                                              capabilities.maxImageExtent.height);
    }
    if (swapchainExtent_.width == 0 || swapchainExtent_.height == 0) {
        error = "Cannot create a Vulkan swapchain for a zero-sized window";
        return false;
    }

    std::uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphicsFamily_ != presentFamily_) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = families.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ((capabilities.supportedCompositeAlpha & createInfo.compositeAlpha) == 0) {
        for (const VkCompositeAlphaFlagBitsKHR alpha : {
                 VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                 VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                 VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
            if ((capabilities.supportedCompositeAlpha & alpha) != 0) {
                createInfo.compositeAlpha = alpha;
                break;
            }
        }
    }
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain_;

    const VkSwapchainKHR oldSwapchain = swapchain_;
    result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateSwapchainKHR", result);
        return false;
    }
    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, oldSwapchain, nullptr);
    }
    swapchainFormat_ = surfaceFormat.format;

    result = vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkGetSwapchainImagesKHR", result);
        return false;
    }
    swapchainImages_.resize(imageCount);
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkGetSwapchainImagesKHR", result);
        return false;
    }
    swapchainImages_.resize(imageCount);

    imageViews_.resize(swapchainImages_.size(), VK_NULL_HANDLE);
    for (std::size_t i = 0; i < swapchainImages_.size(); ++i) {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = swapchainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat_;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        result = vkCreateImageView(device_, &viewInfo, nullptr, &imageViews_[i]);
        if (result != VK_SUCCESS) {
            error = vulkanError("vkCreateImageView", result);
            return false;
        }
    }
    imagesInFlight_.assign(swapchainImages_.size(), VK_NULL_HANDLE);
    return true;
}

bool VulkanRenderer::createRenderPass(std::string& error) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    const VkResult result = vkCreateRenderPass(device_, &createInfo, nullptr, &renderPass_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateRenderPass", result);
        return false;
    }
    return true;
}

bool VulkanRenderer::createPipeline(std::string& error) {
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderInfo.codeSize = sizeof(mygame_shaders::scene_vert);
    shaderInfo.pCode = mygame_shaders::scene_vert;
    VkResult result = vkCreateShaderModule(device_, &shaderInfo, nullptr, &vertexShader);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateShaderModule (vertex)", result);
        return false;
    }
    shaderInfo.codeSize = sizeof(mygame_shaders::scene_frag);
    shaderInfo.pCode = mygame_shaders::scene_frag;
    result = vkCreateShaderModule(device_, &shaderInfo, nullptr, &fragmentShader);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateShaderModule (fragment)", result);
        vkDestroyShaderModule(device_, vertexShader, nullptr);
        return false;
    }

    const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main", nullptr}
    }};
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(VulkanVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkPipelineVertexInputStateCreateInfo vertexInput{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexAttributes.size());
    vertexInput.pVertexAttributeDescriptions = vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlending{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    constexpr std::array<VkDynamicState, 2> dynamicStates{{VK_DYNAMIC_STATE_VIEWPORT,
                                                          VK_DYNAMIC_STATE_SCISSOR}};
    VkPipelineDynamicStateCreateInfo dynamicState{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange viewportRange{};
    viewportRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    viewportRange.offset = 0;
    viewportRange.size = sizeof(float) * 2;
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &viewportRange;
    result = vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreatePipelineLayout", result);
        vkDestroyShaderModule(device_, fragmentShader, nullptr);
        vkDestroyShaderModule(device_, vertexShader, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = static_cast<std::uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;
    result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
    vkDestroyShaderModule(device_, fragmentShader, nullptr);
    vkDestroyShaderModule(device_, vertexShader, nullptr);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateGraphicsPipelines", result);
        return false;
    }
    return true;
}

bool VulkanRenderer::createFramebuffers(std::string& error) {
    framebuffers_.resize(imageViews_.size(), VK_NULL_HANDLE);
    for (std::size_t i = 0; i < imageViews_.size(); ++i) {
        VkFramebufferCreateInfo createInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        createInfo.renderPass = renderPass_;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &imageViews_[i];
        createInfo.width = swapchainExtent_.width;
        createInfo.height = swapchainExtent_.height;
        createInfo.layers = 1;
        const VkResult result = vkCreateFramebuffer(device_, &createInfo, nullptr, &framebuffers_[i]);
        if (result != VK_SUCCESS) {
            error = vulkanError("vkCreateFramebuffer", result);
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::createFrames(std::string& error) {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;
    VkResult result = vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateCommandPool", result);
        return false;
    }

    std::array<VkCommandBuffer, framesInFlight> commandBuffers{};
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool = commandPool_;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = framesInFlight;
    result = vkAllocateCommandBuffers(device_, &allocateInfo, commandBuffers.data());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkAllocateCommandBuffers", result);
        return false;
    }

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (std::uint32_t i = 0; i < framesInFlight; ++i) {
        frames_[i].commandBuffer = commandBuffers[i];
        result = vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &frames_[i].imageAvailable);
        if (result == VK_SUCCESS) {
            result = vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &frames_[i].renderFinished);
        }
        if (result == VK_SUCCESS) {
            result = vkCreateFence(device_, &fenceInfo, nullptr, &frames_[i].inFlight);
        }
        if (result != VK_SUCCESS) {
            error = vulkanError("Creating Vulkan frame synchronization", result);
            return false;
        }
    }
    return true;
}

std::uint32_t VulkanRenderer::findMemoryType(std::uint32_t typeFilter,
                                            VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) != 0 &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return std::numeric_limits<std::uint32_t>::max();
}

bool VulkanRenderer::ensureBuffer(Buffer& buffer, VkDeviceSize requiredSize,
                                  VkBufferUsageFlags usage, std::string& error) {
    if (requiredSize == 0 || buffer.capacity >= requiredSize) {
        return true;
    }
    const VkDeviceSize previousCapacity = buffer.capacity;
    VkDeviceSize bufferSize = requiredSize;
    if (previousCapacity > 0 &&
        previousCapacity <= std::numeric_limits<VkDeviceSize>::max() / 2) {
        bufferSize = std::max(requiredSize, previousCapacity * 2);
    }
    destroyBuffer(buffer);
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer.handle);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkCreateBuffer", result);
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);
    const std::uint32_t memoryType = findMemoryType(
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == std::numeric_limits<std::uint32_t>::max()) {
        error = "No host-visible, coherent Vulkan memory type is available for geometry buffers";
        destroyBuffer(buffer);
        return false;
    }

    VkMemoryAllocateInfo allocationInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocationInfo.allocationSize = requirements.size;
    allocationInfo.memoryTypeIndex = memoryType;
    result = vkAllocateMemory(device_, &allocationInfo, nullptr, &buffer.memory);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkAllocateMemory", result);
        destroyBuffer(buffer);
        return false;
    }
    result = vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkBindBufferMemory", result);
        destroyBuffer(buffer);
        return false;
    }
    result = vkMapMemory(device_, buffer.memory, 0, requirements.size, 0, &buffer.mapped);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkMapMemory", result);
        destroyBuffer(buffer);
        return false;
    }
    buffer.capacity = bufferSize;
    return true;
}

void VulkanRenderer::destroyBuffer(Buffer& buffer) {
    if (device_ != VK_NULL_HANDLE) {
        if (buffer.mapped && buffer.memory != VK_NULL_HANDLE) {
            vkUnmapMemory(device_, buffer.memory);
        }
        if (buffer.handle != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer.handle, nullptr);
        }
        if (buffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, buffer.memory, nullptr);
        }
    }
    buffer = {};
}

bool VulkanRenderer::render(const std::vector<VulkanVertex>& vertices,
                            const std::vector<std::uint32_t>& indices, std::string& error) {
    int windowWidth = 0;
    int windowHeight = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &windowWidth, &windowHeight)) {
        error = std::string("SDL_GetWindowSizeInPixels failed: ") + SDL_GetError();
        return false;
    }
    if (windowWidth == 0 || windowHeight == 0) {
        return true;
    }
    if (swapchainDirty_ && !recreateSwapchain(error)) {
        return false;
    }
    if (vertices.empty() || indices.empty()) {
        return true;
    }

    Frame& frame = frames_[currentFrame_];
    VkResult result = vkWaitForFences(device_, 1, &frame.inFlight, VK_TRUE,
                                      std::numeric_limits<std::uint64_t>::max());
    if (result != VK_SUCCESS) {
        error = vulkanError("vkWaitForFences", result);
        return false;
    }

    if (!ensureBuffer(frame.vertexBuffer, vertices.size() * sizeof(VulkanVertex),
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, error) ||
        !ensureBuffer(frame.indexBuffer, indices.size() * sizeof(std::uint32_t),
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT, error)) {
        return false;
    }
    std::memcpy(frame.vertexBuffer.mapped, vertices.data(), vertices.size() * sizeof(VulkanVertex));
    std::memcpy(frame.indexBuffer.mapped, indices.data(), indices.size() * sizeof(std::uint32_t));

    std::uint32_t imageIndex = 0;
    result = vkAcquireNextImageKHR(device_, swapchain_,
                                   std::numeric_limits<std::uint64_t>::max(),
                                   frame.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchainDirty_ = true;
        return recreateSwapchain(error);
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        error = vulkanError("vkAcquireNextImageKHR", result);
        return false;
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        swapchainDirty_ = true;
    }

    if (imagesInFlight_[imageIndex] != VK_NULL_HANDLE) {
        result = vkWaitForFences(device_, 1, &imagesInFlight_[imageIndex], VK_TRUE,
                                 std::numeric_limits<std::uint64_t>::max());
        if (result != VK_SUCCESS) {
            error = vulkanError("vkWaitForFences (swapchain image)", result);
            return false;
        }
    }
    imagesInFlight_[imageIndex] = frame.inFlight;

    result = vkResetCommandBuffer(frame.commandBuffer, 0);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkResetCommandBuffer", result);
        return false;
    }
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    result = vkBeginCommandBuffer(frame.commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkBeginCommandBuffer", result);
        return false;
    }

    VkClearValue clearValue{};
    clearValue.color = {{0.055f, 0.071f, 0.125f, 1.0f}};
    VkRenderPassBeginInfo renderInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderInfo.renderPass = renderPass_;
    renderInfo.framebuffer = framebuffers_[imageIndex];
    renderInfo.renderArea.extent = swapchainExtent_;
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(frame.commandBuffer, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchainExtent_.width);
    viewport.height = static_cast<float>(swapchainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapchainExtent_};
    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);
    const std::array<float, 2> viewportSize{{viewport.width, viewport.height}};
    vkCmdPushConstants(frame.commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(viewportSize), viewportSize.data());
    constexpr VkDeviceSize vertexOffset = 0;
    vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, &frame.vertexBuffer.handle, &vertexOffset);
    vkCmdBindIndexBuffer(frame.commandBuffer, frame.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(frame.commandBuffer, static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(frame.commandBuffer);
    result = vkEndCommandBuffer(frame.commandBuffer);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkEndCommandBuffer", result);
        return false;
    }

    result = vkResetFences(device_, 1, &frame.inFlight);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkResetFences", result);
        return false;
    }
    constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame.renderFinished;
    result = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, frame.inFlight);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkQueueSubmit", result);
        return false;
    }

    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &frame.renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        swapchainDirty_) {
        swapchainDirty_ = true;
        if (!recreateSwapchain(error)) {
            return false;
        }
    } else if (result != VK_SUCCESS) {
        error = vulkanError("vkQueuePresentKHR", result);
        return false;
    }

    currentFrame_ = (currentFrame_ + 1) % framesInFlight;
    return true;
}

bool VulkanRenderer::recreateSwapchain(std::string& error) {
    VkResult result = vkDeviceWaitIdle(device_);
    if (result != VK_SUCCESS) {
        error = vulkanError("vkDeviceWaitIdle", result);
        return false;
    }
    destroySwapchainResources();
    if (!createSwapchain(error) || !createRenderPass(error) ||
        !createPipeline(error) || !createFramebuffers(error)) {
        return false;
    }
    swapchainDirty_ = false;
    return true;
}

void VulkanRenderer::destroySwapchainResources() {
    if (device_ == VK_NULL_HANDLE) {
        return;
    }
    for (const VkFramebuffer framebuffer : framebuffers_) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
    }
    framebuffers_.clear();
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }
    for (const VkImageView view : imageViews_) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, view, nullptr);
        }
    }
    imageViews_.clear();
    swapchainImages_.clear();
    imagesInFlight_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::shutdown() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        for (Frame& frame : frames_) {
            destroyBuffer(frame.vertexBuffer);
            destroyBuffer(frame.indexBuffer);
            if (frame.imageAvailable != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, frame.imageAvailable, nullptr);
            }
            if (frame.renderFinished != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, frame.renderFinished, nullptr);
            }
            if (frame.inFlight != VK_NULL_HANDLE) {
                vkDestroyFence(device_, frame.inFlight, nullptr);
            }
            frame = {};
        }
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
        }
        destroySwapchainResources();
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    window_ = nullptr;
}
