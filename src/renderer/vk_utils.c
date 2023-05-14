#include "vk_utils.h"

uint32_t vk_get_memory_type_index(
    VkPhysicalDevice gpu,
    VkMemoryRequirements mem_requirements,
    VkMemoryPropertyFlags mem_props
) {
    uint32_t type_index = INVALID_INDEX;

    VkPhysicalDeviceMemoryProperties gpu_mem_props;
    vkGetPhysicalDeviceMemoryProperties(gpu, &gpu_mem_props);

    for (uint32_t i = 0; i <gpu_mem_props.memoryTypeCount; i++) {
        if (mem_requirements.memoryTypeBits & (1 << i) && (gpu_mem_props.memoryTypes[i].propertyFlags & mem_props) == mem_props) {
            type_index = i;
            break;
        }
    }

    CAKEZ_ASSERT(type_index != INVALID_INDEX, "Failed to find proper type index for memory properties: %d", mem_props);
    return type_index;
}

Image vk_allocate_image(
    VkDevice device,
    VkPhysicalDevice gpu,
    uint32_t width,
    uint32_t height,
    VkFormat format
) {
    Image image = {0};

    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    // image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK(vkCreateImage(device, &image_info, VK_NULL_HANDLE, &image.image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, image.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = vk_get_memory_type_index(
        gpu,
        mem_requirements,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    VK_CHECK(vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, &image.memory));
    VK_CHECK(vkBindImageMemory(device, image.image, image.memory, 0));

    return image;
}

Buffer vk_allocate_buffer(
    VkDevice device,
    VkPhysicalDevice gpu,
    uint32_t size,
    VkBufferUsageFlags buffer_usage,
    VkMemoryPropertyFlags mem_props
) {
    Buffer buffer = {0};
    buffer.size = size;

    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.usage = buffer_usage;
    buffer_info.size = size;
    VK_CHECK(vkCreateBuffer(device, &buffer_info, VK_NULL_HANDLE, &buffer.buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = vk_get_memory_type_index(
        gpu,
        mem_requirements,
        mem_props
    );

    VK_CHECK(vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, &buffer.memory));
    VK_CHECK(vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));

    if (mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        VK_CHECK(vkMapMemory(device, buffer.memory, 0, mem_requirements.size, 0, &buffer.data));
    }

    return buffer;
}

// TODO: Implement
void vk_copy_to_buffer(Buffer *buffer, void *data, uint32_t size) {
    CAKEZ_ASSERT(buffer->size >= size, "Buffer too small: %d for data: %d", buffer->size, size);

    // If we have mapped data
    if (buffer->data) {
        memcpy(buffer->data, data, size);
    } else {
        // TODO: Implement, copy data using command buffer
    }
}

VkWriteDescriptorSet write_set(
    VkDescriptorSet descriptor_set,
    VkDescriptorType type,
    DescriptorInfo *descriptor_info,
    uint32_t binding_index,
    uint32_t count
) {
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = VK_NULL_HANDLE,
        .dstSet = descriptor_set,
        .dstBinding = binding_index,
        .dstArrayElement = 0,
        .descriptorCount = count,
        .descriptorType = type,
        .pImageInfo = &descriptor_info->image_info,
        .pBufferInfo = &descriptor_info->buffer_info,
        .pTexelBufferView = NULL
    };
    return write;
}