#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef struct VkContext {
    VkInstance instance;
} VkContext;

bool vk_init(VkContext *vk_context);