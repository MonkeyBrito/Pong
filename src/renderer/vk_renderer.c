#include "vk_renderer.h"

#include <stdio.h>

bool vk_init(VkContext *vk_context) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Pong";
    app_info.pEngineName = "Ponggine";

    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    VkResult result = vkCreateInstance(&instance_info, VK_NULL_HANDLE, &vk_context->instance);
    if (result == VK_SUCCESS) {
        printf("Successfully create vulkan instance!");
        return true;
    }

    return false;
}