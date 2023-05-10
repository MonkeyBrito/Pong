#include <stdio.h>
#include <vulkan/vulkan.h>

int main(void) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Pong";
    app_info.pEngineName = "Ponggine";

    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, VK_NULL_HANDLE, &instance);
    if (result == VK_SUCCESS) {
        printf("Successfully create vulkan instance!");
    }

    return 0;
}