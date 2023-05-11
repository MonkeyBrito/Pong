#include "vk_renderer.h"

#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <stdio.h>
#include <intrin.h> // __debugbreak

#define ARRAY_SIZE(arr) sizeof((arr))/sizeof((arr[0]))

#define VK_CHECK(result)                      \
    if (result != VK_SUCCESS) {               \
        printf("Vulkan Error: %d\n", result); \
        printf("%s\n", #result);              \
        __debugbreak();                       \
        return false;                         \
    }

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    messageSeverity = messageSeverity;
    messageTypes = messageTypes;
    pUserData = pUserData;
    printf("Validation Error: %s", pCallbackData->pMessage);
    return false;
}

bool vk_init(VkContext *vk_context, void *window) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Pong";
    app_info.pEngineName = "Ponggine";

    const char *extensions[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount = ARRAY_SIZE(extensions);
    instance_info.ppEnabledLayerNames = layers;
    instance_info.enabledLayerCount = ARRAY_SIZE(layers);
    VK_CHECK(vkCreateInstance(&instance_info, VK_NULL_HANDLE, &vk_context->instance));

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_context->instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT) {
        VkDebugUtilsMessengerCreateInfoEXT debug_info = {0};
        debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debug_info.pfnUserCallback = vk_debug_callback;

        VK_CHECK(vkCreateDebugUtilsMessengerEXT(vk_context->instance, &debug_info, VK_NULL_HANDLE, &vk_context->debug_messenger));
    }

    // Create Surface
    {
        // TODO: If WIN32
        VkWin32SurfaceCreateInfoKHR surface_info = {0};
        surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_info.hwnd = (HWND)window;
        surface_info.hinstance = GetModuleHandleA(0);
        VK_CHECK(vkCreateWin32SurfaceKHR(vk_context->instance, &surface_info, VK_NULL_HANDLE, &vk_context->surface));
    }

    // Choose GPU
    {
        vk_context->graphics_index = UINT32_MAX;
        uint32_t gpu_count = 0;
        // TODO: Suballocation from Main Allocation
        VkPhysicalDevice gpus[10];

        VK_CHECK(vkEnumeratePhysicalDevices(vk_context->instance, &gpu_count, 0));
        VK_CHECK(vkEnumeratePhysicalDevices(vk_context->instance, &gpu_count, gpus));

        for (uint32_t i = 0; i < gpu_count; i++) {
            VkPhysicalDevice gpu = gpus[i];

            uint32_t queue_family_count = 0;
            // TODO: Suballocation from Main Allocation
            VkQueueFamilyProperties queue_props[10];
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, 0);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_props);

            for (uint32_t j = 0; j < queue_family_count; j++) {
                if (queue_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    VkBool32 surface_support = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, j, vk_context->surface, &surface_support));
                    if (surface_support) {
                        vk_context->graphics_index = j;
                        vk_context->gpu = gpu;
                        break;
                    }
                }
            }
        }
    }
    if (vk_context->graphics_index == UINT32_MAX) {
        return false;
    }

    // Logical Device
    {
        float queue_priority = 1.0f;

        VkDeviceQueueCreateInfo queue_info = {0};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = vk_context->graphics_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        const char *extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkDeviceCreateInfo device_info = {0};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.queueCreateInfoCount = 1;
        device_info.ppEnabledExtensionNames = extensions;
        device_info.enabledExtensionCount = ARRAY_SIZE(extensions);

        VK_CHECK(vkCreateDevice(vk_context->gpu, &device_info, VK_NULL_HANDLE, &vk_context->device));

        vkGetDeviceQueue(vk_context->device, vk_context->graphics_index, 0, &vk_context->graphics_queue);
    }

    // Swapchain
    {
        uint32_t format_count = 0;
        // TODO: Suballocation from Main Allocation
        VkSurfaceFormatKHR surface_formats[10];
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_context->gpu, vk_context->surface, &format_count, VK_NULL_HANDLE));
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_context->gpu, vk_context->surface, &format_count, surface_formats));

        for (uint32_t i = 0; i < format_count; i++) {
            VkSurfaceFormatKHR format = surface_formats[i];

            if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
                vk_context->surface_format = format;
                break;
            }
        }

        VkSurfaceCapabilitiesKHR surface_caps = {0};
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_context->gpu, vk_context->surface, &surface_caps));
        uint32_t image_count = surface_caps.minImageCount + 1;
        if (image_count > surface_caps.maxImageCount) image_count = surface_caps.maxImageCount;

        VkSwapchainCreateInfoKHR swapchain_info = {0};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.surface = vk_context->surface;
        swapchain_info.imageFormat = vk_context->surface_format.format;
        swapchain_info.preTransform = surface_caps.currentTransform;
        swapchain_info.imageExtent = surface_caps.currentExtent;
        swapchain_info.minImageCount = image_count;
        swapchain_info.imageArrayLayers = 1;

        VK_CHECK(vkCreateSwapchainKHR(vk_context->device, &swapchain_info, VK_NULL_HANDLE, &vk_context->swapchain));

        VK_CHECK(vkGetSwapchainImagesKHR(vk_context->device, vk_context->swapchain, &vk_context->swapchain_image_count, 0));
        VK_CHECK(vkGetSwapchainImagesKHR(vk_context->device, vk_context->swapchain, &vk_context->swapchain_image_count, vk_context->swapchain_images));
    }

    // Command Pool
    {
        VkCommandPoolCreateInfo pool_info = {0};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = vk_context->graphics_index;
        VK_CHECK(vkCreateCommandPool(vk_context->device, &pool_info, VK_NULL_HANDLE, &vk_context->command_pool));
    }

    // Sync Objects
    {
        VkSemaphoreCreateInfo semaphore_info = {0};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(vk_context->device, &semaphore_info, VK_NULL_HANDLE, &vk_context->acquire_semaphore));
        VK_CHECK(vkCreateSemaphore(vk_context->device, &semaphore_info, VK_NULL_HANDLE, &vk_context->submit_semaphore));
    }

    return true;
}

bool vk_render(VkContext *vk_context) {
    uint32_t image_index = 0;
    VK_CHECK(vkAcquireNextImageKHR(vk_context->device, vk_context->swapchain, 0, vk_context->acquire_semaphore, 0, &image_index));

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandBufferCount = 1;
    alloc_info.commandPool = vk_context->command_pool;
    vkAllocateCommandBuffers(vk_context->device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    // Rendering Commands
    {
        VkClearColorValue color = {{1.0f, 1.0f, 0.0f, 1.0f}};
        VkImageSubresourceRange range = {0};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.layerCount = 1;
        range.levelCount = 1;
        vkCmdClearColorImage(cmd, vk_context->swapchain_images[image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &color, 1, &range);
    }

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.pSignalSemaphores = &vk_context->submit_semaphore;
    submit_info.signalSemaphoreCount = 1;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vk_context->acquire_semaphore;
    VK_CHECK(vkQueueSubmit(vk_context->graphics_queue, 1, &submit_info, 0));

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &vk_context->swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &image_index;
    present_info.pWaitSemaphores = &vk_context->submit_semaphore;
    present_info.waitSemaphoreCount = 1;
    VK_CHECK(vkQueuePresentKHR(vk_context->graphics_queue, &present_info));

    vkFreeCommandBuffers(vk_context->device, vk_context->command_pool, 1, &cmd);

    return true;
}