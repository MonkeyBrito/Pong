#include "vk_renderer.h"

#include "../platform.h"
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
    platform_get_window_size(&vk_context->screen_size.width, &vk_context->screen_size.height);

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

        // Create the image views
        {
            for (uint32_t i = 0; i < vk_context->swapchain_image_count; i++) {
                VkImageViewCreateInfo view_info = {0};
                view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                view_info.format = vk_context->surface_format.format;
                view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view_info.subresourceRange.layerCount = 1;
                view_info.subresourceRange.levelCount = 1;

                for (uint32_t i = 0; i < vk_context->swapchain_image_count; i++) {
                    view_info.image = vk_context->swapchain_images[i];
                    VK_CHECK(vkCreateImageView(vk_context->device, &view_info, VK_NULL_HANDLE, &vk_context->swapchain_image_views[i]));
                }
            }
        }
    }

    // Render Pass
    {
        VkAttachmentDescription attachment = {0};
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.format = vk_context->surface_format.format;

        VkAttachmentReference color_attachment_ref = {0};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_desc = {0};
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

        VkAttachmentDescription attachments[] = {
            attachment
        };

        VkRenderPassCreateInfo render_pass_info;
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.pAttachments = attachments;
        render_pass_info.attachmentCount = ARRAY_SIZE(attachments);
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass_desc;

        VK_CHECK(vkCreateRenderPass(vk_context->device, &render_pass_info, VK_NULL_HANDLE, &vk_context->render_pass));
    }

    // Framebuffers
    {
        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.width = vk_context->screen_size.width;
        framebuffer_info.height = vk_context->screen_size.height;
        framebuffer_info.renderPass = vk_context->render_pass;
        framebuffer_info.layers = 1;
        framebuffer_info.attachmentCount = 1;

        for (uint32_t i = 0; i < vk_context->swapchain_image_count; i++) {
            framebuffer_info.pAttachments = &vk_context->swapchain_image_views[i];
            VK_CHECK(vkCreateFramebuffer(vk_context->device, &framebuffer_info, VK_NULL_HANDLE, &vk_context->framebuffers[i]));
        }
    }

    // Pipeline Layout
    {
        VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        VK_CHECK(vkCreatePipelineLayout(vk_context->device, &pipeline_layout_info, VK_NULL_HANDLE, &vk_context->pipeline_layout));
    }

    // Pipeline
    {
        VkShaderModule vertex_shader, fragment_shader;

        uint32_t size_in_bytes;
        const char *code = platform_read_file("assets/shaders/shader.vert.spv", &size_in_bytes);

        VkShaderModuleCreateInfo shader_info = {0};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.pCode = (uint32_t *)code;
        shader_info.codeSize = size_in_bytes;
        VK_CHECK(vkCreateShaderModule(vk_context->device, &shader_info, VK_NULL_HANDLE, &vertex_shader));
        // TODO: Suballocation from main allocation
        free((void *)code);

        code = platform_read_file("assets/shaders/shader.frag.spv", &size_in_bytes);
        shader_info.pCode = (uint32_t *)code;
        shader_info.codeSize = size_in_bytes;
        VK_CHECK(vkCreateShaderModule(vk_context->device, &shader_info, VK_NULL_HANDLE, &fragment_shader));
        // TODO: Suballocation from main allocation
        free((void *)code);

        VkPipelineShaderStageCreateInfo vertex_stage = {0};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.pName = "main";
        vertex_stage.module = vertex_shader;

        VkPipelineShaderStageCreateInfo fragment_stage = {0};
        fragment_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_stage.pName = "main";
        fragment_stage.module = fragment_shader;

        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vertex_stage,
            fragment_stage
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state = {0};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // vertex_input_state.

        VkPipelineColorBlendAttachmentState color_attachment = {0};
        color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blend_state = {0};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pAttachments = &color_attachment;
        color_blend_state.attachmentCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization_state = {0};
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state = {0};
        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {0};
        input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkRect2D scissor = {0};
        VkViewport viewport = {0};

        VkPipelineViewportStateCreateInfo viewport_state = {0};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;

        VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamic_state = {0};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.pDynamicStates = dynamic_states;
        dynamic_state.dynamicStateCount = ARRAY_SIZE(dynamic_states);

        VkGraphicsPipelineCreateInfo pipeline_info = {0};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.layout = vk_context->pipeline_layout;
        pipeline_info.renderPass = vk_context->render_pass;
        pipeline_info.pVertexInputState = &vertex_input_state;
        pipeline_info.pColorBlendState = &color_blend_state;
        pipeline_info.pStages = shader_stages;
        pipeline_info.stageCount = ARRAY_SIZE(shader_stages);
        pipeline_info.pRasterizationState = &rasterization_state;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.pMultisampleState = &multisample_state;
        pipeline_info.pInputAssemblyState = &input_assembly_state;
        VK_CHECK(vkCreateGraphicsPipelines(vk_context->device, VK_NULL_HANDLE, 1, &pipeline_info, VK_NULL_HANDLE, &vk_context->pipeline));
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

    VkClearValue clear_value = {{{1.0f, 1.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_begin = {0};
    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderPass = vk_context->render_pass;
    render_pass_begin.renderArea.extent = vk_context->screen_size;
    render_pass_begin.framebuffer = vk_context->framebuffers[image_index];
    render_pass_begin.pClearValues = &clear_value;
    render_pass_begin.clearValueCount = 1;
    vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    // Rendering Commands
    {
        VkRect2D scissor = {0};
        scissor.extent = vk_context->screen_size;

        VkViewport viewport = {0};
        viewport.width = (float)vk_context->screen_size.width;
        viewport.height = (float)vk_context->screen_size.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->pipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);

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

    VK_CHECK(vkDeviceWaitIdle(vk_context->device));

    vkFreeCommandBuffers(vk_context->device, vk_context->command_pool, 1, &cmd);

    return true;
}