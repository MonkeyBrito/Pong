#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct VkContext {
    VkExtent2D screen_size;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceFormatKHR surface_format;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkSwapchainKHR swapchain;
    VkRenderPass render_pass;
    VkCommandPool command_pool;

    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkSemaphore acquire_semaphore;
    VkSemaphore submit_semaphore;

    uint32_t swapchain_image_count;
    // TODO: Suballocation from Main Allocation
    VkImage swapchain_images[5];
    VkImageView swapchain_image_views[5];
    VkFramebuffer framebuffers[5];

    uint32_t graphics_index;
} VkContext;

bool vk_init(VkContext *vk_context, void *window);
bool vk_render(VkContext *vk_context);