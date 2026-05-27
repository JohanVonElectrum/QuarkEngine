#include "backend.h"

#include "device.h"
#include "swapchain.h"
#include "vk.h"

#include <quark/core/assert.h>
#include <quark/core/log.h>

#include <cstdlib/mem.h>

#include <GLFW/glfw3.h>

#include <stdio.h>

static VulkanContext context;
static GLFWwindow* s_current_window = nullptr;

static constexpr char vertex_shader_path[] = QUARK_SHADER_DIR "/obb.vert.spv";
static constexpr char fragment_shader_path[] = QUARK_SHADER_DIR "/obb.frag.spv";

#ifdef QUARK_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
);
#endif // QUARK_DEBUG

static b8_t vk_recreate_swapchain();
static b8_t read_binary_file(const char* path, u8_t** out_data, usize_t* out_size);
static b8_t create_shader_module_from_file(const char* path, VkShaderModule* out_module);
static b8_t create_vulkan_graphics_pipeline();
static void destroy_vulkan_graphics_pipeline();

b8_t init_renderer_backend(
    const char* app_name, const u16_t app_major, const u16_t app_minor, const u16_t app_patch
) {
    // TODO: use a custom allocator
    context.allocator = nullptr;

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_4,
        .pApplicationName = app_name,
        .applicationVersion = VK_MAKE_VERSION(app_major, app_minor, app_patch),
        .pEngineName = "Quark",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .pNext = nullptr,
    };

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .flags = 0,
        .pNext = nullptr,
    };

    instance_create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(
        &instance_create_info.enabledExtensionCount);

    QUARK_ASSERT_RETURN(
        false,
        instance_create_info.enabledExtensionCount == 0 || instance_create_info.ppEnabledExtensionNames != NULL,
        "Failed to get required Vulkan instance extensions from GLFW"
    );

#ifdef QUARK_DEBUG
    const char* extensions[instance_create_info.enabledExtensionCount + 1];
    mem_copy(extensions, instance_create_info.ppEnabledExtensionNames,
                   instance_create_info.enabledExtensionCount * sizeof(const char*));
    extensions[instance_create_info.enabledExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    instance_create_info.ppEnabledExtensionNames = extensions;

    QUARK_LOG_DEBUG("Enabled Vulkan instance extensions:");
    for (u32_t i = 0; i < instance_create_info.enabledExtensionCount; ++i) {
        QUARK_LOG_DEBUG("  %s", instance_create_info.ppEnabledExtensionNames[i]);
    }

    const char* required_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    constexpr usize_t required_layer_count = sizeof(required_layers) / sizeof(required_layers[0]);

    u32_t available_layer_count = 0;
    VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr), false);
    VkLayerProperties available_layers[available_layer_count];
    VK_CHECK_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers), false);
    for (u32_t i = 0; i < required_layer_count; ++i) {
        for (u32_t j = 0; j < available_layer_count; ++j) {
            if (strcmp(required_layers[i], available_layers[j].layerName) == 0) {
                goto found;
            }
        }
        QUARK_LOG_ERROR("Required Vulkan layer not found: %s", required_layers[i]);
        return false;
    found:
        QUARK_LOG_DEBUG("Found required Vulkan layer: %s", required_layers[i]);
    }
    instance_create_info.enabledLayerCount = required_layer_count;
    instance_create_info.ppEnabledLayerNames = required_layers;
#endif // QUARK_DEBUG

    VK_CHECK_RETURN(vkCreateInstance(&instance_create_info, context.allocator, &context.instance), false);

    QUARK_LOG_DEBUG("Created Vulkan instance");

#ifdef QUARK_DEBUG
    constexpr u32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            ;
    
    const VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = log_severity,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_debug_callback,
        .pUserData = nullptr,
        .pNext = nullptr,
    };

    const auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        context.instance,
        "vkCreateDebugUtilsMessengerEXT"
    );
    QUARK_ASSERT_RETURN(
        false,
        vkCreateDebugUtilsMessengerEXT != nullptr,
        "Failed to get vkCreateDebugUtilsMessengerEXT function pointer"
    );

    VK_CHECK_RETURN(
        vkCreateDebugUtilsMessengerEXT(
            context.instance, &debug_create_info, context.allocator, &context.debug_messenger
        ),
        false
    );

    QUARK_LOG_DEBUG("Created Vulkan debug messenger");
#endif // QUARK_DEBUG

    QUARK_LOG_INFO("Initialized renderer backend");

    return true;
}

b8_t shutdown_renderer_backend() {
#ifdef QUARK_DEBUG
    const auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        context.instance,
        "vkDestroyDebugUtilsMessengerEXT"
    );
    QUARK_ASSERT_RETURN(
        false,
        vkDestroyDebugUtilsMessengerEXT != nullptr,
        "Failed to get vkDestroyDebugUtilsMessengerEXT function pointer"
    );
    vkDestroyDebugUtilsMessengerEXT(context.instance, context.debug_messenger, context.allocator);
    QUARK_LOG_DEBUG("Destroyed Vulkan debug messenger");
#endif // QUARK_DEBUG

    vkDestroyInstance(context.instance, context.allocator);
    QUARK_LOG_DEBUG("Destroyed Vulkan instance");

    QUARK_LOG_INFO("Shutdown renderer backend");

    return true;
}

b8_t init_renderer_window(GLFWwindow* window) {
    s_current_window = window;

    VK_CHECK_RETURN(
        glfwCreateWindowSurface(context.instance, window, context.allocator, &context.surface),
        false
    );

    if (!create_vulkan_device(&context)) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
        return false;
    }

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    if (!create_vulkan_swapchain(
            &context,
            framebuffer_width > 0 ? (u32_t) framebuffer_width : 1,
            framebuffer_height > 0 ? (u32_t) framebuffer_height : 1
        )) {
        destroy_vulkan_device(&context);
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
        return false;
    }

    if (!create_vulkan_graphics_pipeline()) {
        destroy_vulkan_swapchain(&context);
        destroy_vulkan_device(&context);
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

b8_t shutdown_renderer_window() {
    b8_t success = true;
    destroy_vulkan_graphics_pipeline();
    success = success && destroy_vulkan_swapchain(&context);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    context.surface = VK_NULL_HANDLE;
    success = success && destroy_vulkan_device(&context);
    s_current_window = nullptr;
    return success;
}

b8_t render_renderer_frame(const Camera* camera) {
    QUARK_ASSERT_RETURN(false, camera != nullptr, "Invalid camera pointer");

    if (context.swapchain.extent.width == 0 || context.swapchain.extent.height == 0) {
        return true;
    }

    const f32_t aspect_ratio = (f32_t) context.swapchain.extent.width / (f32_t) context.swapchain.extent.height;
    mat4 view_projection;
    if (!camera_get_view_projection_matrix(camera, aspect_ratio, view_projection)) {
        QUARK_LOG_ERROR("Failed to compute camera view-projection matrix");
        return false;
    }

    const u32_t frame_index = context.swapchain.current_frame;
    VkFence frame_fence = context.swapchain.in_flight_fences[frame_index];

    VK_CHECK_RETURN(vkWaitForFences(context.device.logical_device, 1, &frame_fence, VK_TRUE, UINT64_MAX), false);

    u32_t image_index = 0;
    const VkResult acquire_result = vkAcquireNextImageKHR(
        context.device.logical_device,
        context.swapchain.swapchain,
        UINT64_MAX,
        context.swapchain.image_available_semaphores[frame_index],
        VK_NULL_HANDLE,
        &image_index
    );
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_recreate_swapchain();
        return true;
    }
    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
        QUARK_LOG_ERROR("Failed to acquire swapchain image: %u", acquire_result);
        return false;
    }
    VK_CHECK_RETURN(vkResetFences(context.device.logical_device, 1, &frame_fence), false);

    VkCommandBuffer command_buffer = context.swapchain.command_buffers[frame_index];
    VK_CHECK_RETURN(vkResetCommandBuffer(command_buffer, 0), false);

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK_RETURN(vkBeginCommandBuffer(command_buffer, &begin_info), false);

    const VkClearValue clear_values[] = {
        {
            .color = {
                .float32 = {0.08f, 0.09f, 0.12f, 1.0f},
            },
        },
        {
            .depthStencil = {
                .depth = 1.0f,
                .stencil = 0,
            },
        },
    };
    const VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = context.swapchain.render_pass,
        .framebuffer = context.swapchain.framebuffers[image_index],
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = context.swapchain.extent,
        },
        .clearValueCount = (u32_t) (sizeof(clear_values) / sizeof(clear_values[0])),
        .pClearValues = clear_values,
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context.graphics_pipeline);
    vkCmdPushConstants(
        command_buffer,
        context.pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        (u32_t) sizeof(mat4),
        view_projection
    );
    vkCmdDraw(command_buffer, 36, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    VK_CHECK_RETURN(vkEndCommandBuffer(command_buffer), false);

    const VkSemaphore wait_semaphores[] = {
        context.swapchain.image_available_semaphores[frame_index],
    };
    const VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    const VkSemaphore signal_semaphores[] = {
        context.swapchain.render_finished_semaphores[image_index],
    };
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (u32_t) (sizeof(wait_semaphores) / sizeof(wait_semaphores[0])),
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = (u32_t) (sizeof(signal_semaphores) / sizeof(signal_semaphores[0])),
        .pSignalSemaphores = signal_semaphores,
    };
    VK_CHECK_RETURN(vkQueueSubmit(context.device.graphics_queue, 1, &submit_info, frame_fence), false);

    const VkSwapchainKHR swapchains[] = {
        context.swapchain.swapchain,
    };
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = (u32_t) (sizeof(signal_semaphores) / sizeof(signal_semaphores[0])),
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = (u32_t) (sizeof(swapchains) / sizeof(swapchains[0])),
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };

    const VkResult present_result = vkQueuePresentKHR(context.device.present_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || context.swapchain.framebuffer_resized) {
        if (!vk_recreate_swapchain()) {
            return false;
        }
    } else if (present_result != VK_SUCCESS) {
        QUARK_LOG_ERROR("Failed to present swapchain image: %u", present_result);
        return false;
    }

    context.swapchain.current_frame = (context.swapchain.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

void on_framebuffer_resized() {
    context.swapchain.framebuffer_resized = true;
}

#ifdef QUARK_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    const VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            QUARK_LOG_ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            QUARK_LOG_WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            QUARK_LOG_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            QUARK_LOG_TRACE(callback_data->pMessage);
            break;
        default:
            QUARK_DEBUGBREAK();
            QUARK_LOG_ERROR("(unknown severity) ", callback_data->pMessage);
    }
    return VK_FALSE;
}
#endif // QUARK_DEBUG

static b8_t vk_recreate_swapchain() {
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(s_current_window, &framebuffer_width, &framebuffer_height);

    while (framebuffer_width == 0 || framebuffer_height == 0) {
        glfwGetFramebufferSize(s_current_window, &framebuffer_width, &framebuffer_height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context.device.logical_device);

    destroy_vulkan_graphics_pipeline();

    destroy_vulkan_swapchain(&context);

    if (!create_vulkan_swapchain(&context, (u32_t) framebuffer_width, (u32_t) framebuffer_height)) {
        QUARK_LOG_ERROR("Failed to recreate swapchain");
        return false;
    }

    if (!create_vulkan_graphics_pipeline()) {
        QUARK_LOG_ERROR("Failed to recreate graphics pipeline");
        return false;
    }

    context.swapchain.framebuffer_resized = false;

    QUARK_LOG_DEBUG("Swapchain recreated with dimensions %ux%u", framebuffer_width, framebuffer_height);
    return true;
}

static b8_t read_binary_file(const char* path, u8_t** out_data, usize_t* out_size) {
    QUARK_ASSERT_RETURN(false, out_data != nullptr && out_size != nullptr, "Invalid output pointers");

    *out_data = nullptr;
    *out_size = 0;

    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0 || file == nullptr) {
        QUARK_LOG_ERROR("Failed to open shader file: %s", path);
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        QUARK_LOG_ERROR("Failed to seek shader file: %s", path);
        return false;
    }

    const long file_size = ftell(file);
    if (file_size <= 0) {
        fclose(file);
        QUARK_LOG_ERROR("Shader file is empty or invalid: %s", path);
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        QUARK_LOG_ERROR("Failed to rewind shader file: %s", path);
        return false;
    }

    u8_t* data = mem_heap_alloc((usize_t) file_size);
    if (data == nullptr) {
        fclose(file);
        QUARK_LOG_ERROR("Failed to allocate memory for shader file: %s", path);
        return false;
    }

    const usize_t bytes_read = fread(data, 1, (usize_t) file_size, file);
    fclose(file);

    if (bytes_read != (usize_t) file_size) {
        mem_heap_free(data);
        QUARK_LOG_ERROR("Failed to read shader file: %s", path);
        return false;
    }

    *out_data = data;
    *out_size = (usize_t) file_size;
    return true;
}

static b8_t create_shader_module_from_file(const char* path, VkShaderModule* out_module) {
    QUARK_ASSERT_RETURN(false, out_module != nullptr, "Invalid output shader module pointer");

    u8_t* shader_code = nullptr;
    usize_t shader_code_size = 0;
    if (!read_binary_file(path, &shader_code, &shader_code_size)) {
        return false;
    }

    const VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shader_code_size,
        .pCode = (const u32_t*) shader_code,
    };

    VK_CHECK_X(
        vkCreateShaderModule(
            context.device.logical_device,
            &shader_module_create_info,
            context.allocator,
            out_module
        ),
        {
            mem_heap_free(shader_code);
            return false;
        }
    );

    mem_heap_free(shader_code);
    return true;
}

static b8_t create_vulkan_graphics_pipeline() {
    QUARK_ASSERT_RETURN(false, context.swapchain.render_pass != VK_NULL_HANDLE, "Render pass must be created before the graphics pipeline");

    VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
    VkShaderModule fragment_shader_module = VK_NULL_HANDLE;

    if (!create_shader_module_from_file(vertex_shader_path, &vertex_shader_module)) {
        return false;
    }

    if (!create_shader_module_from_file(fragment_shader_path, &fragment_shader_module)) {
        vkDestroyShaderModule(context.device.logical_device, vertex_shader_module, context.allocator);
        return false;
    }

    const VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) context.swapchain.extent.width,
        .height = (float) context.swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor = {
        .offset = {.x = 0, .y = 0},
        .extent = context.swapchain.extent,
    };

    const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {0},
        .back = {0},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                          | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT
                          | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    const VkPushConstantRange camera_push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = (u32_t) sizeof(mat4),
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &camera_push_constant_range,
    };

    VK_CHECK_X(
        vkCreatePipelineLayout(
            context.device.logical_device,
            &pipeline_layout_create_info,
            context.allocator,
            &context.pipeline_layout
        ),
        {
            vkDestroyShaderModule(context.device.logical_device, fragment_shader_module, context.allocator);
            vkDestroyShaderModule(context.device.logical_device, vertex_shader_module, context.allocator);
            return false;
        }
    );

    const VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = (u32_t) (sizeof(shader_stages) / sizeof(shader_stages[0])),
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = nullptr,
        .layout = context.pipeline_layout,
        .renderPass = context.swapchain.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VK_CHECK_X(
        vkCreateGraphicsPipelines(
            context.device.logical_device,
            VK_NULL_HANDLE,
            1,
            &graphics_pipeline_create_info,
            context.allocator,
            &context.graphics_pipeline
        ),
        {
            vkDestroyPipelineLayout(context.device.logical_device, context.pipeline_layout, context.allocator);
            context.pipeline_layout = VK_NULL_HANDLE;
            vkDestroyShaderModule(context.device.logical_device, fragment_shader_module, context.allocator);
            vkDestroyShaderModule(context.device.logical_device, vertex_shader_module, context.allocator);
            return false;
        }
    );

    vkDestroyShaderModule(context.device.logical_device, fragment_shader_module, context.allocator);
    vkDestroyShaderModule(context.device.logical_device, vertex_shader_module, context.allocator);

    QUARK_LOG_DEBUG("Created Vulkan graphics pipeline for hardcoded triangle");
    return true;
}

static void destroy_vulkan_graphics_pipeline() {
    if (context.device.logical_device == VK_NULL_HANDLE) {
        return;
    }

    vkDeviceWaitIdle(context.device.logical_device);

    if (context.graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(context.device.logical_device, context.graphics_pipeline, context.allocator);
        context.graphics_pipeline = VK_NULL_HANDLE;
    }

    if (context.pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context.device.logical_device, context.pipeline_layout, context.allocator);
        context.pipeline_layout = VK_NULL_HANDLE;
    }
}
