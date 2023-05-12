#pragma once

#include <vulkan/vulkan.h>

typedef struct Image {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
} Image;

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *data;
} Buffer;