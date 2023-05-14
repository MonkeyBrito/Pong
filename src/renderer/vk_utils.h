#pragma once

#include "../defines.h"
#include "vk_types.h"

Image vk_allocate_image(
    VkDevice device,
    VkPhysicalDevice gpu,
    uint32_t width,
    uint32_t height,
    VkFormat format
);

Buffer vk_allocate_buffer(
    VkDevice device,
    VkPhysicalDevice gpu,
    uint32_t size,
    VkBufferUsageFlags buffer_usage,
    VkMemoryPropertyFlags mem_props
);

void vk_copy_to_buffer(Buffer *buffer, void *data, uint32_t size);