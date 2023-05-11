#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct VkContext {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceFormatKHR surface_format;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkSwapchainKHR swapchain;
    VkCommandPool command_pool;

    VkSemaphore acquire_semaphore;
    VkSemaphore submit_semaphore;

    uint32_t swapchain_image_count;
    // TODO: Suballocation from Main Allocation
    VkImage swapchain_images[5];

    uint32_t graphics_index;
} VkContext;

bool vk_init(VkContext *vk_context, void *window);
bool vk_render(VkContext *vk_context);