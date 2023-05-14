#include "vk_renderer.h"

#include "../dds_structs.h"
#include "../logger.h"
#include "../platform.h"

#include "vk_types.h"
#include "vk_utils.h"

#include <windows.h>
#include <vulkan/vulkan_win32.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    messageSeverity = messageSeverity;
    messageTypes = messageTypes;
    pUserData = pUserData;
    CAKEZ_ERROR(pCallbackData->pMessage);
    CAKEZ_ASSERT(0, pCallbackData->pMessage);
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
    } else {
        return false;
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

    // Render Pass
    {
        VkAttachmentDescription color_attachment = {0};
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.format = vk_context->surface_format.format;

        VkAttachmentDescription attachments[] = {
            color_attachment
        };

        VkAttachmentReference color_attachment_ref = {0};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_desc = {0};
        subpass_desc.colorAttachmentCount = 1;
        subpass_desc.pColorAttachments = &color_attachment_ref;

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
        framebuffer_info.renderPass = vk_context->render_pass;
        framebuffer_info.width = vk_context->screen_size.width;
        framebuffer_info.height = vk_context->screen_size.height;
        framebuffer_info.layers = 1;
        framebuffer_info.attachmentCount = 1;

        for (uint32_t i = 0; i < vk_context->swapchain_image_count; i++) {
            framebuffer_info.pAttachments = &vk_context->swapchain_image_views[i];
            VK_CHECK(vkCreateFramebuffer(vk_context->device, &framebuffer_info, VK_NULL_HANDLE, &vk_context->framebuffers[i]));
        }
    }

    // Command Pool
    {
        VkCommandPoolCreateInfo pool_info = {0};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = vk_context->graphics_index;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(vk_context->device, &pool_info, VK_NULL_HANDLE, &vk_context->command_pool));
    }

    // Command Buffer
    {
        VkCommandBufferAllocateInfo alloc_info = {0};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandBufferCount = 1;
        alloc_info.commandPool = vk_context->command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        VK_CHECK(vkAllocateCommandBuffers(vk_context->device, &alloc_info, &vk_context->cmd));
    }

    // Sync Objects
    {
        VkSemaphoreCreateInfo semaphore_info = {0};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_CHECK(vkCreateSemaphore(vk_context->device, &semaphore_info, VK_NULL_HANDLE, &vk_context->acquire_semaphore));
        VK_CHECK(vkCreateSemaphore(vk_context->device, &semaphore_info, VK_NULL_HANDLE, &vk_context->submit_semaphore));

        VkFenceCreateInfo fence_info = {0};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(vk_context->device, &fence_info, VK_NULL_HANDLE, &vk_context->image_available_fence));
    }

    // Create Descriptor Set Layouts
    {
        VkDescriptorSetLayoutBinding binding0 = {0};
        binding0.binding = 0;
        binding0.descriptorCount = 1;
        binding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding binding1 = {0};
        binding1.binding = 1;
        binding1.descriptorCount = 1;
        binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding1.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding binding2 = {0};
        binding2.binding = 2;
        binding2.descriptorCount = 1;
        binding2.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[] = {
            binding0,
            binding1,
            binding2
        };

        VkDescriptorSetLayoutCreateInfo layout_info = {0};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = ARRAY_SIZE(bindings);
        layout_info.pBindings = bindings;

        VK_CHECK(vkCreateDescriptorSetLayout(vk_context->device, &layout_info, VK_NULL_HANDLE, &vk_context->descriptor_set_layout));
    }

    // Pipeline Layout
    {
        VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &vk_context->descriptor_set_layout;
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
        rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

        vkDestroyShaderModule(vk_context->device, vertex_shader, VK_NULL_HANDLE);
        vkDestroyShaderModule(vk_context->device, fragment_shader, VK_NULL_HANDLE);
    }

    // Staging Buffer
    {
        vk_context->staging_buffer = vk_allocate_buffer(
            vk_context->device,
            vk_context->gpu,
            MB(1),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
    }

    // Create Image
    {
        uint32_t file_size;
        DDSFile *file = (DDSFile *)platform_read_file("assets/textures/cakez.DDS", &file_size);
        uint32_t texture_size = file->header.width * file->header.height * 4;

        vk_copy_to_buffer(&vk_context->staging_buffer, &file->data_begin, texture_size);

        // TODO: Assertions
        vk_context->image = vk_allocate_image(
            vk_context->device,
            vk_context->gpu,
            file->header.width,
            file->header.height,
            VK_FORMAT_R8G8B8A8_UNORM
        );

        VkCommandBuffer cmd;
        VkCommandBufferAllocateInfo cmd_alloc_info = {0};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandBufferCount = 1;
        cmd_alloc_info.commandPool = vk_context->command_pool;
        vkAllocateCommandBuffers(vk_context->device, &cmd_alloc_info, &cmd);

        VkCommandBufferBeginInfo begin_info = {0};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

        // Transition Layout to transfer optimal
        VkImageSubresourceRange range = {0};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.layerCount = 1;
        range.levelCount = 1;

        VkImageMemoryBarrier image_memory_barrier = {0};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.image = vk_context->image.image;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.subresourceRange = range;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0,
            1, &image_memory_barrier);

        VkBufferImageCopy copy_region = {0};
        copy_region.imageExtent.width = file->header.width;
        copy_region.imageExtent.height = file->header.height;
        copy_region.imageExtent.depth = 1;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkCmdCopyBufferToImage(cmd, vk_context->staging_buffer.buffer, vk_context->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0,
            1, &image_memory_barrier);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkFence upload_fence;
        VkFenceCreateInfo fence_info = {0};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = 0;
        VK_CHECK(vkCreateFence(vk_context->device, &fence_info, VK_NULL_HANDLE, &upload_fence));

        VkSubmitInfo submit_info = {0};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;
        VK_CHECK(vkQueueSubmit(vk_context->graphics_queue, 1, &submit_info, upload_fence));

        VK_CHECK(vkWaitForFences(vk_context->device, 1, &upload_fence, VK_TRUE, UINT64_MAX));
    }

    // Create Image View
    {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = vk_context->image.image;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.layerCount = 1;
        view_info.subresourceRange.levelCount = 1;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        VK_CHECK(vkCreateImageView(vk_context->device, &view_info, VK_NULL_HANDLE, &vk_context->image.view));
    }

    // Create Sampler
    {
        VkSamplerCreateInfo sampler_info = {0};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.magFilter = VK_FILTER_NEAREST;

        VK_CHECK(vkCreateSampler(vk_context->device, &sampler_info, VK_NULL_HANDLE, &vk_context->sampler));
    }

    // Create transform buffer
    {
        vk_context->transform_storage_buffer = vk_allocate_buffer(
            vk_context->device,
            vk_context->gpu,
            sizeof(Transform) * MAX_ENTITIES,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
    }

    // Create Global Uniform Buffer Object
    {
        vk_context->global_UBO = vk_allocate_buffer(
            vk_context->device,
            vk_context->gpu,
            sizeof(GlobalData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
        GlobalData global_data = {
            (int)vk_context->screen_size.width,
            (int)vk_context->screen_size.height
        };
        vk_copy_to_buffer(&vk_context->global_UBO, &global_data, sizeof(global_data));
    }

    // Create index buffer
    {
        vk_context->index_buffer = vk_allocate_buffer(
            vk_context->device,
            vk_context->gpu,
            sizeof(uint32_t) * 6,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        );
        // Copy indices to the buffer
        {
            uint32_t indices[] = {0, 1, 2, 2, 3, 0};
            vk_copy_to_buffer(&vk_context->index_buffer, indices, sizeof(uint32_t) * 6);
        }
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
        };

        VkDescriptorPoolCreateInfo pool_info;
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = ARRAY_SIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(vk_context->device, &pool_info, VK_NULL_HANDLE, &vk_context->descriptor_pool));
    }

    // Create Descriptor Set
    {
        VkDescriptorSetAllocateInfo alloc_info = {0};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pSetLayouts = &vk_context->descriptor_set_layout;
        alloc_info.descriptorSetCount = 1;
        alloc_info.descriptorPool = vk_context->descriptor_pool;
        VK_CHECK(vkAllocateDescriptorSets(vk_context->device, &alloc_info, &vk_context->descriptor_set));
    }

    // Update descriptor set
    {
        DescriptorInfo descriptor_infos[] = {
            {{.buffer_info = {.buffer = vk_context->global_UBO.buffer, .offset = 0, .range = VK_WHOLE_SIZE}}},
            {{.buffer_info = {.buffer = vk_context->transform_storage_buffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE}}},
            {{.image_info = {.sampler = vk_context->sampler, .imageView = vk_context->image.view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}}}
        };

        VkWriteDescriptorSet writes[] = {
            write_set(vk_context->descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptor_infos[0], 0, 1),
            write_set(vk_context->descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptor_infos[1], 1, 1),
            write_set(vk_context->descriptor_set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &descriptor_infos[2], 2, 1)
        };

        vkUpdateDescriptorSets(vk_context->device, ARRAY_SIZE(writes), writes, 0, VK_NULL_HANDLE);
    }

    return true;
}

bool vk_render(VkContext *vk_context, GameState *game_state) {
    VK_CHECK(vkWaitForFences(vk_context->device, 1, &vk_context->image_available_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(vk_context->device, 1, &vk_context->image_available_fence));

    // Copy transforms to the buffer
    {
        vk_copy_to_buffer(&vk_context->transform_storage_buffer, &game_state->entities, sizeof(Transform) * game_state->entity_count);
    }

    uint32_t image_index = 0;
    VK_CHECK(vkAcquireNextImageKHR(vk_context->device, vk_context->swapchain, UINT64_MAX, vk_context->acquire_semaphore, VK_NULL_HANDLE, &image_index));

    VkCommandBuffer cmd = vk_context->cmd;
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    VkClearValue clear_value = {{{1.0f, 1.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_begin = {0};
    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderArea.extent = vk_context->screen_size;
    render_pass_begin.clearValueCount = 1;
    render_pass_begin.pClearValues = &clear_value;
    render_pass_begin.renderPass = vk_context->render_pass;
    render_pass_begin.framebuffer = vk_context->framebuffers[image_index];
    vkCmdBeginRenderPass(cmd, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    // Rendering Commands
    {
        VkViewport viewport = {0};
        viewport.width = (float)vk_context->screen_size.width;
        viewport.height = (float)vk_context->screen_size.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {0};
        scissor.extent = vk_context->screen_size;

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->pipeline_layout, 0, 1, &vk_context->descriptor_set, 0, VK_NULL_HANDLE);

        vkCmdBindIndexBuffer(cmd, vk_context->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->pipeline);
        // vkCmdDraw(cmd, 6, 1, 0, 0);
        vkCmdDrawIndexed(cmd, 6, game_state->entity_count, 0, 0, 0);
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
    submit_info.pWaitSemaphores = &vk_context->acquire_semaphore;
    submit_info.waitSemaphoreCount = 1;
    VK_CHECK(vkQueueSubmit(vk_context->graphics_queue, 1, &submit_info, vk_context->image_available_fence));

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &vk_context->swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &image_index;
    present_info.pWaitSemaphores = &vk_context->submit_semaphore;
    present_info.waitSemaphoreCount = 1;
    VK_CHECK(vkQueuePresentKHR(vk_context->graphics_queue, &present_info));

    return true;
}