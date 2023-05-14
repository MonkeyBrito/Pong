#pragma once

#include "../logger.h"

#include <vulkan/vulkan.h>

#define VK_CHECK(result)                        \
    if (result != VK_SUCCESS) {                 \
        CAKEZ_ERROR("Vulkan Error: %d", result);\
        CAKEZ_ERROR("%s", #result);             \
        __debugbreak();                         \
    }


typedef struct Image {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
} Image;

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t size;
    void *data;
} Buffer;

typedef struct DescriptorInfo {
    union {
        VkDescriptorBufferInfo buffer_info;
        VkDescriptorImageInfo image_info;
    };
} DescriptorInfo;