#include "glm/glm/fwd.hpp"
#include "safpch.h"
#include "graphics.h"

#include <cstring>
#include <fstream>
#include <shaderc/shaderc.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace saf {

    vk::ShaderModule create_shader_module(vk::Device device, std::string path)
    {
        static const std::map<std::string, shaderc_shader_kind> shader_stage_map = { {"comp", shaderc_shader_kind::shaderc_compute_shader},
                                                                                        {"frag", shaderc_shader_kind::shaderc_fragment_shader},
                                                                                        {"geom", shaderc_shader_kind::shaderc_geometry_shader},
                                                                                        {"tesc", shaderc_shader_kind::shaderc_tess_control_shader},
                                                                                        {"tese", shaderc_shader_kind::shaderc_tess_evaluation_shader},
                                                                                        {"vert", shaderc_shader_kind::shaderc_vertex_shader} };
        auto compiler = shaderc_compiler_initialize();

        std::ifstream file;
        file.open(path);
        if (!file) IFX_ERROR("Failed to load file: {0}", path);

        std::string str;
        std::string file_contents;
        while (std::getline(file, str))
        {
            file_contents += str;
            file_contents.push_back('\n');
        }

        IFX_TRACE("Shader src:\n{0}", file_contents);

        std::string file_ext = path;

        // Extract extension name from the glsl shader file
        file_ext = file_ext.substr(file_ext.find_last_of(".") + 1);

        // Compile the GLSL source
        auto stageIt = shader_stage_map.find(file_ext);
        if (stageIt == shader_stage_map.end())
        {
            IFX_ERROR("Vulkan file extension {0} does not have a vulkan shader stage.", file_ext);
        }
        shaderc_compilation_result_t compilation_result = shaderc_compile_into_spv(compiler, file_contents.c_str(), file_contents.size(), stageIt->second, path.c_str(), "main", nullptr);
        if (shaderc_result_get_compilation_status(compilation_result) != shaderc_compilation_status_success) IFX_ERROR("ShaderC failed to compile: {0}", path);

        size_t length = shaderc_result_get_length(compilation_result);
        const uint32_t* spirv = (const uint32_t*)shaderc_result_get_bytes(compilation_result);

        vk::ShaderModuleCreateInfo shader_module_create_info({}, length, spirv);

        shaderc_compiler_release(compiler);

        return device.createShaderModule(shader_module_create_info);
    }

    vk::Pipeline create_graphics_pipeline(vk::Device       device,
        vk::PipelineCache                                         pipeline_cache,
        std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stages,
        vk::PipelineVertexInputStateCreateInfo const& vertex_input_state,
        vk::PrimitiveTopology                                     primitive_topology,
        uint32_t                                                  patch_control_points,
        vk::PolygonMode                                           polygon_mode,
        vk::CullModeFlags                                         cull_mode,
        vk::FrontFace                                             front_face,
        std::vector<vk::PipelineColorBlendAttachmentState> const& blend_attachment_states,
        vk::PipelineDepthStencilStateCreateInfo const& depth_stencil_state,
        vk::PipelineLayout                                        pipeline_layout,
        vk::RenderPass                                            render_pass)
    {
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state({}, primitive_topology, false);

        vk::PipelineTessellationStateCreateInfo tessellation_state({}, patch_control_points);

        vk::PipelineViewportStateCreateInfo viewport_state({}, 1, nullptr, 1, nullptr);

        vk::PipelineRasterizationStateCreateInfo rasterization_state;
        rasterization_state.polygonMode = polygon_mode;
        rasterization_state.cullMode = cull_mode;
        rasterization_state.frontFace = front_face;
        rasterization_state.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisample_state({}, vk::SampleCountFlagBits::e1);

        vk::PipelineColorBlendStateCreateInfo color_blend_state({}, false, {}, blend_attachment_states);

        std::array<vk::DynamicState, 2>    dynamic_state_enables = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamic_state({}, dynamic_state_enables);

        // Final fullscreen composition pass pipeline
        vk::GraphicsPipelineCreateInfo pipeline_create_info({},
            shader_stages,
            &vertex_input_state,
            &input_assembly_state,
            &tessellation_state,
            &viewport_state,
            &rasterization_state,
            &multisample_state,
            &depth_stencil_state,
            &color_blend_state,
            &dynamic_state,
            pipeline_layout,
            render_pass,
            {},
            {},
            -1);

        vk::Result   result;
        vk::Pipeline pipeline;
        std::tie(result, pipeline) = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info);
        assert(result == vk::Result::eSuccess);
        return pipeline;
    }

    Graphics::Graphics()
    {
        m_vkLayers = { "VK_LAYER_KHRONOS_validation" };
        m_vkExtensions = { "VK_KHR_portability_enumeration", "VK_EXT_debug_utils" };
    }

    Graphics::~Graphics()
    {
        Destroy();
    }

	void Graphics::Init(GLFWwindow* glfw_window, uint32_t width, uint32_t height)
	{
        m_Width = width;
        m_Height = height;
        CreateInstance();
        CreateDebugMessenger();
        CreateSurface(glfw_window);
        SetPhysicalDevice();

        if ((m_vkGraphicsQueueIndex = FindQueueFamily(vk::QueueFlagBits::eGraphics, m_vkSurface)) == UINT32_MAX)
        {
            IFX_ERROR("Window failed FindQueueFamily present");
        }
        else IFX_TRACE("Window FindQueueFamily present found at index: {0}", m_vkGraphicsQueueIndex);

        CreateDevice();

        m_vkQueue = m_vkDevice.getQueue(m_vkGraphicsQueueIndex, 0);

        CreateSwapchain();

        CreateRenderPass();
        CreateFramebuffers();

        m_vkPipelineLayout = m_vkDevice.createPipelineLayout({});

        CreatePipeline();

        CreateVertexBuffer();
	}

	void Graphics::Destroy()
	{

        if (m_vkVertexBufferMemory)
        {
            m_vkDevice.unmapMemory(m_vkVertexBufferMemory);
            m_vkDevice.freeMemory(m_vkVertexBufferMemory);
        }
        
        if (m_vkDevice) m_vkDevice.waitIdle();

        if (m_vkVertexBuffer)m_vkDevice.destroyBuffer(m_vkVertexBuffer);
        for (auto semiphore : m_vkRecycleSemaphores) if (semiphore) m_vkDevice.destroySemaphore(semiphore);
        if (m_vkSwapchainData.swapchain) DestroySwapchain(m_vkSwapchainData.swapchain);
        if (m_vkPipeline) m_vkDevice.destroy(m_vkPipeline);
        if (m_vkPipelineLayout) m_vkDevice.destroyPipelineLayout(m_vkPipelineLayout);
        if (m_vkRenderpass) m_vkDevice.destroyRenderPass(m_vkRenderpass);
        if (m_vkDevice) m_vkDevice.destroy();
        if (m_vkSurface) m_vkInstance.destroySurfaceKHR(m_vkSurface);
        if (m_vkMessenger) m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkMessenger);
        if (m_vkInstance) m_vkInstance.destroy();
	}

    void printFPS() {
        static auto oldTime = std::chrono::high_resolution_clock::now();
        static int fps; fps++;

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - oldTime) >= std::chrono::seconds{ 1 }) {
            oldTime = std::chrono::high_resolution_clock::now();
            IFX_TRACE("FPS: {0}", fps);
            fps = 0;
        }
    }

    void Graphics::Render()
    {

        //printFPS();

        static float angle = 0.f;
        angle += 0.001f;

        //Vertex* vertices = static_cast<Vertex*>(m_vkDevice.mapMemory(m_vkVertexBufferMemory, 0, m_VertexBuffer.size() * sizeof(Vertex)));

        for (int i = 0; i < 3; i++)
        {
            auto& pos = m_VertexBuffer[i].pos;
            m_VertexArrayTransformed[i].pos.x = pos.x * cosf(angle) - pos.y * sinf(angle);
            m_VertexArrayTransformed[i].pos.y = pos.x * sinf(angle) + pos.y * cosf(angle);

            m_VertexArrayTransformed[i].color = m_VertexBuffer[i].color;
        }

        //m_vkDevice.unmapMemory(m_vkVertexBufferMemory);

        vk::Semaphore acquire_semaphore;
        if (m_vkRecycleSemaphores.empty())
        {
            acquire_semaphore = m_vkDevice.createSemaphore({});
        }
        else
        {
            acquire_semaphore = m_vkRecycleSemaphores.back();
            m_vkRecycleSemaphores.pop_back();
        }

        vk::Result res;
        uint32_t   image;
        std::tie(res, image) = m_vkDevice.acquireNextImageKHR(m_vkSwapchainData.swapchain, UINT64_MAX, acquire_semaphore);

        if (res != vk::Result::eSuccess)
        {
            m_vkRecycleSemaphores.push_back(acquire_semaphore);
            IFX_WARN("Vulkan failed acquireNextImage");
            m_vkQueue.waitIdle();
            return;
        }
        else
        {
            // If we have outstanding fences for this swapchain image, wait for them to complete first.
            // After begin frame returns, it is safe to reuse or delete resources which
            // were used previously.
            //
            // We wait for fences which completes N frames earlier, so we do not stall,
            // waiting for all GPU work to complete before this returns.
            // Normally, this doesn't really block at all,
            // since we're waiting for old frames to have been completed, but just in case.
            if (m_vkFramesData[image].queue_submit_fence)
            {
                (void)m_vkDevice.waitForFences(m_vkFramesData[image].queue_submit_fence, true, UINT64_MAX);
                m_vkDevice.resetFences(m_vkFramesData[image].queue_submit_fence);
            }

            if (m_vkFramesData[image].primary_command_pool)
            {
                m_vkDevice.resetCommandPool(m_vkFramesData[image].primary_command_pool);
            }

            // Recycle the old semaphore back into the semaphore manager.
            vk::Semaphore old_semaphore = m_vkFramesData[image].swapchain_acquire_semaphore;

            if (old_semaphore)
            {
                m_vkRecycleSemaphores.push_back(old_semaphore);
            }

            m_vkFramesData[image].swapchain_acquire_semaphore = acquire_semaphore;

        }

        RenderTri(image);

        // Present swapchain image
        vk::PresentInfoKHR present_info(m_vkFramesData[image].swapchain_release_semaphore, m_vkSwapchainData.swapchain, image);
        res = m_vkQueue.presentKHR(present_info);

        if (res != vk::Result::eSuccess)
        {
            IFX_ERROR("Failed to present swapchain image.");
        }
    }

    void Graphics::RenderTri(uint32_t index)
    {
        // Render to this framebuffer.
        vk::Framebuffer framebuffer = m_vkSwapchainData.framebuffers[index];

        // Allocate or re-use a primary command buffer.
        vk::CommandBuffer cmd = m_vkFramesData[index].primary_command_buffer;

        // We will only submit this once before it's recycled.
        vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        // Begin command recording
        cmd.begin(begin_info);

        // Set clear color values.
        vk::ClearValue clear_value;
        clear_value.color = vk::ClearColorValue(std::array<float, 4>({ {0.01f, 0.01f, 0.033f, 1.0f} }));

        // Begin the render pass.
        vk::RenderPassBeginInfo rp_begin(m_vkRenderpass, framebuffer, { {0, 0}, {m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height} },
            clear_value);
        // We will add draw commands in the same command buffer.
        cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);

        // Bind the graphics pipeline.
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkPipeline);

        vk::Viewport vp(0.0f, 0.0f, static_cast<float>(m_vkSwapchainData.extent.width), static_cast<float>(m_vkSwapchainData.extent.height), 0.0f, 1.0f);
        // Set viewport dynamically
        cmd.setViewport(0, vp);

        vk::Rect2D scissor({ 0, 0 }, { m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height });
        // Set scissor dynamically
        cmd.setScissor(0, scissor);

        cmd.bindVertexBuffers(0, {m_vkVertexBuffer}, {0});

        // Draw three vertices with one instance.
        cmd.draw(3, 1, 0, 0);

        // Complete render pass.
        cmd.endRenderPass();

        // Complete the command buffer.
        cmd.end();

        // Submit it to the queue with a release semaphore.
        if (!m_vkFramesData[index].swapchain_release_semaphore)
        {
            m_vkFramesData[index].swapchain_release_semaphore = m_vkDevice.createSemaphore({});
        }

        vk::PipelineStageFlags wait_stage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        vk::SubmitInfo info(m_vkFramesData[index].swapchain_acquire_semaphore, wait_stage, cmd,
            m_vkFramesData[index].swapchain_release_semaphore);
        // Submit command buffer to graphics queue
        m_vkQueue.submit(info, m_vkFramesData[index].queue_submit_fence);
    }

    void Graphics::CreateInstance()
    {
        IFX_TRACE("Window CreateInstance");
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        for (int n = 0; n < glfw_extension_count; ++n) m_vkExtensions.emplace_back(glfw_extensions[n]);

        auto supported_extentions = vk::enumerateInstanceExtensionProperties();

        auto required_extention_not_found = std::find_if(m_vkExtensions.begin(),
            m_vkExtensions.end(),
            [&supported_extentions](auto extension) {
                return std::find_if(supported_extentions.begin(),
                    supported_extentions.end(),
                    [&extension](auto const& ep) {
                        return strcmp(ep.extensionName, extension) == 0;
                    }) == supported_extentions.end();
            });

        if (required_extention_not_found != m_vkExtensions.end())
        {
            IFX_ERROR("Vulkan did not find the following required extentions:");
            for (; required_extention_not_found != m_vkExtensions.end(); ++required_extention_not_found)
            {
                IFX_ERROR("\t{0}", *required_extention_not_found);
            }
            IFX_ERROR("Vulkan missing required extention(s)");
        }

        auto supported_layers = vk::enumerateInstanceLayerProperties();

        auto required_layer_not_found = std::find_if(m_vkLayers.begin(),
            m_vkLayers.end(),
            [&supported_layers](auto extension) {
                return std::find_if(supported_layers.begin(),
                    supported_layers.end(),
                    [&extension](auto const& ep) {
                        return strcmp(ep.layerName, extension) == 0;
                    }) == supported_layers.end();
            });

        if (required_layer_not_found != m_vkLayers.end())
        {
            IFX_ERROR("Vulkan did not find the following required layers:");
            for (; required_layer_not_found != m_vkLayers.end(); ++required_layer_not_found)
            {
                IFX_ERROR("\t{0}", *required_layer_not_found);
            }
            IFX_ERROR("Vulkan missing required layer(s)");
        }

        const vk::ApplicationInfo applicationInfo(g_EngineName, VK_MAKE_API_VERSION(0, 1, 0, 0), g_ApplicationName, VK_MAKE_API_VERSION(0, 1, 0, 0), VK_API_VERSION_1_1);
        const vk::InstanceCreateInfo createinfo(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &applicationInfo, m_vkLayers.size(), m_vkLayers.data(), m_vkExtensions.size(), m_vkExtensions.data());
        m_vkInstance = vk::createInstance(createinfo);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkInstance);
    }

    void Graphics::CreateDebugMessenger()
    {
        IFX_TRACE("Window CreateDebugMessenger");
        vk::DebugUtilsMessengerCreateInfoEXT messengercreateinfo({}, vk::FlagTraits<vk::DebugUtilsMessageSeverityFlagBitsEXT>::allFlags, vk::FlagTraits<vk::DebugUtilsMessageTypeFlagBitsEXT>::allFlags,
            [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
            {


                switch (messageSeverity)
                {
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: IFX_TRACE("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: IFX_INFO("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: IFX_WARN("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: IFX_ERROR("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
                }

                return VK_FALSE;
            });
        m_vkMessenger = m_vkInstance.createDebugUtilsMessengerEXT(messengercreateinfo);
        if (!m_vkMessenger) IFX_ERROR("Vulkan failed to create debug messenger");
    }

    bool IsDevicesSuitable(vk::PhysicalDevice device)
    {
        auto properties = device.getProperties();
        IFX_TRACE("Device: {0}", properties.deviceName.data());

        switch (properties.deviceType)
        {
        case vk::PhysicalDeviceType::eCpu:              IFX_TRACE("\tType: CPU"); break;
        case vk::PhysicalDeviceType::eDiscreteGpu:      IFX_TRACE("\tType: Descrete GPU"); break;
        case vk::PhysicalDeviceType::eIntegratedGpu:    IFX_TRACE("\tType: Integrated GPU"); break;
        case vk::PhysicalDeviceType::eVirtualGpu:       IFX_TRACE("\tType: Virtual GPU"); break;
        case vk::PhysicalDeviceType::eOther:            IFX_TRACE("\tType: Unknown"); break;
        }

        std::string vendor = "UNKNOWN";

        switch (properties.vendorID)
        {
        case 0x1002: IFX_TRACE("\tVendorID: 0x1002 (AMD)"); vendor = "AMD"; break;
        case 0x1010: IFX_TRACE("\tVendorID: 0x1010 (ImgTec)"); vendor = "ImgTec"; break;
        case 0x10DE: IFX_TRACE("\tVendorID: 0x10DE (NVIDIA)"); vendor = "NVIDIA"; break;
        case 0x13B5: IFX_TRACE("\tVendorID: 0x13B5 (ARM)"); vendor = "ARM"; break;
        case 0x5143: IFX_TRACE("\tVendorID: 0x5143 (Qualcomm)"); vendor = "Qualcomm"; break;
        case 0x8086: IFX_TRACE("\tVendorID: 0x8086 (INTEL)"); vendor = "INTEL"; break;
        }

        IFX_TRACE("\tVersion: {0}.{1}.{2}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion));
        IFX_TRACE("\tAPIVersion: {0}.{1}.{2}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

        return true;
    }

    void Graphics::SetPhysicalDevice()
    {
        IFX_TRACE("Window SetPhysicalDevice");
        std::vector<vk::PhysicalDevice> devices = m_vkInstance.enumeratePhysicalDevices();
        if (devices.empty()) IFX_ERROR("Vulkan didnt find any physical devices");

        m_vkPhysicalDevice = *std::find_if(devices.begin(), devices.end(), IsDevicesSuitable);
    }

    uint32_t Graphics::FindQueueFamily(vk::QueueFlags flags, vk::SurfaceKHR surface)
    {
        IFX_TRACE("Window FindQueueFamily");
        if (!m_vkPhysicalDevice)
        {
            IFX_ERROR("Vulkan no physical device set");
            return UINT32_MAX;
        }

        std::vector<vk::QueueFamilyProperties> queue_family_properties = m_vkPhysicalDevice.getQueueFamilyProperties();

        if (queue_family_properties.empty())
        {
            IFX_WARN("Vulkan no queue families found on current physical device");
            return UINT32_MAX;
        }

        static bool first = true;

        if (first) IFX_TRACE("Vulkan {0} queue families found on current physical device", queue_family_properties.size());

        bool queue_found = false;
        int queue_index = UINT32_MAX;
        int i = 0;
        for (auto qfp : queue_family_properties)
        {
            if (qfp.queueCount == 0) continue;

            bool can_preset = false;
            bool has_flags = false;

            if (qfp.queueFlags & flags) has_flags = true;
            if (surface && m_vkPhysicalDevice)
            {
                //if (m_vkPhysicalDevice.getSurfaceSupportKHR(i, surface) == VK_SUCCESS) can_preset = true;
                if (glfwGetPhysicalDevicePresentationSupport(static_cast<VkInstance>(m_vkInstance), static_cast<VkPhysicalDevice>(m_vkPhysicalDevice), i) == GLFW_TRUE) can_preset = true;
            }

            if (has_flags && (can_preset || !surface))
            {
                queue_found = true;
                queue_index = i;
            }

            if (first)
            {
                IFX_TRACE("Queue family {0}", i);
                std::string supports;
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eCompute) supports += "compute";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eGraphics) supports += supports.empty() ? "graphics" : ", graphics";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eOpticalFlowNV) supports += supports.empty() ? "opticalflownv" : ", opticalflownv";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eProtected) supports += supports.empty() ? "protected" : ", protected";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eSparseBinding) supports += supports.empty() ? "sparsebinding" : ", sparsebinding";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eTransfer) supports += supports.empty() ? "transfer" : ", transfer";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR) supports += supports.empty() ? "videodecode" : ", videodecode";
                //if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR) supports += supports.empty() ? "videoencode" : ", videoencode";
                IFX_TRACE("\tSupports: {0}", supports);
                IFX_TRACE("\tFamily supports {0} queue{1}", qfp.queueCount, qfp.queueCount > 1 ? "s" : "");

            }
            else if (queue_found) return i;

            ++i;
        }
        first = false;

        if (queue_found) return queue_index;

        return UINT32_MAX;
    }

    void Graphics::CreateDevice()
    {
        IFX_TRACE("Window CreateDevice");
        std::array<const char*, 1> extensions = { "VK_KHR_swapchain" };

        float queue_priority = 1.f;
        const vk::DeviceQueueCreateInfo queue_create_info({}, m_vkGraphicsQueueIndex, 1, &queue_priority);
        const vk::PhysicalDeviceFeatures device_features{};
        const vk::DeviceCreateInfo device_create_info({}, 1, &queue_create_info, static_cast<uint32_t>(m_vkLayers.size()), m_vkLayers.data(), static_cast<uint32_t>(extensions.size()), extensions.data(), &device_features);
        m_vkDevice = m_vkPhysicalDevice.createDevice(device_create_info);

        if (!m_vkDevice)
        {
            IFX_ERROR("Vulkan failed to create logical device");
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkDevice);
    }

    void Graphics::CreateSurface(GLFWwindow* glfw_window)
    {
        IFX_TRACE("Window CreateSurface");
        VkResult result = glfwCreateWindowSurface(m_vkInstance, glfw_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_vkSurface));
        if (result != VK_SUCCESS || !m_vkSurface)
        {
            IFX_ERROR("Vulkan failed to create glfw surface");
        }
    }

    void Graphics::CreateSwapchain()
    {
        IFX_TRACE("Window CreateSwapchain");
        vk::SurfaceCapabilitiesKHR capabilities = m_vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkSurface);
        std::vector<vk::SurfaceFormatKHR> supported_formats = m_vkPhysicalDevice.getSurfaceFormatsKHR(m_vkSurface);
        std::vector<vk::PresentModeKHR> supported_preset_modes = m_vkPhysicalDevice.getSurfacePresentModesKHR(m_vkSurface);

        if (supported_formats.size() == 0) IFX_ERROR("Vulkan surface doesnt support any formats?");
        if (supported_preset_modes.size() == 0) IFX_ERROR("Vulkan surface doesnt support any presentModes?");

        std::vector<vk::Format> prefered_formats = { vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32 };

        auto format_it = std::find_if(supported_formats.begin(), supported_formats.end(), [&prefered_formats](vk::SurfaceFormatKHR surface_format)
            {
                return std::any_of(prefered_formats.begin(), prefered_formats.end(), [&surface_format](vk::Format pref_format)
                    {
                        return pref_format == surface_format.format;
                    });
            });

        vk::SurfaceFormatKHR image_format = (format_it == supported_formats.end()) ? *format_it : supported_formats[0];
        m_vkSwapchainData.format = image_format.format;

        uint32_t desired_swapchain_images = capabilities.minImageCount + 1;
        if ((capabilities.maxImageCount > 0) && (desired_swapchain_images > capabilities.maxImageCount))
        {
            // Application must settle for fewer images than desired.
            desired_swapchain_images = capabilities.maxImageCount;
            IFX_WARN("Vulkan settling for {0} swapchain images, {1} desired", capabilities.maxImageCount, desired_swapchain_images);
        }

        vk::SurfaceTransformFlagBitsKHR pre_transform =
            (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
            vk::SurfaceTransformFlagBitsKHR::eIdentity : capabilities.currentTransform;

        vk::CompositeAlphaFlagBitsKHR composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::eInherit;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
        }

        vk::Extent2D extent{
            std::min(m_Width, capabilities.currentExtent.width),
            std::min(m_Height, capabilities.currentExtent.height) };

        m_vkSwapchainData.extent = extent;
        vk::SwapchainKHR old_swapchain = m_vkSwapchainData.swapchain;

        // FIFO must be supported by all implementations.
        vk::PresentModeKHR swapchain_present_mode = vk::PresentModeKHR::eFifo;

        vk::SwapchainCreateInfoKHR swapchain_create_info;
        swapchain_create_info.surface = m_vkSurface;
        swapchain_create_info.minImageCount = desired_swapchain_images;
        swapchain_create_info.imageFormat = image_format.format;
        swapchain_create_info.imageColorSpace = image_format.colorSpace;
        swapchain_create_info.imageExtent = extent;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_create_info.preTransform = pre_transform;
        swapchain_create_info.compositeAlpha = composite;
        swapchain_create_info.presentMode = swapchain_present_mode;
        swapchain_create_info.clipped = true;
        swapchain_create_info.oldSwapchain = old_swapchain;

        m_vkSwapchainData.swapchain = m_vkDevice.createSwapchainKHR(swapchain_create_info);
        if (!m_vkSwapchainData.swapchain) IFX_ERROR("Vulkan failed to create swapchain");

        DestroySwapchain(old_swapchain);

        std::vector<vk::Image> swapchain_images = m_vkDevice.getSwapchainImagesKHR(m_vkSwapchainData.swapchain);
        size_t image_count = swapchain_images.size();

        // Initialize per-frame resources.
        // Every swapchain image has its own command pool and fence manager.
        // This makes it very easy to keep track of when we can reset command buffers and such.
        m_vkFramesData.clear();
        m_vkFramesData.resize(image_count);

        for (auto& frame_data : m_vkFramesData)
        {
            frame_data.queue_submit_fence = m_vkDevice.createFence({ vk::FenceCreateFlagBits::eSignaled });
            frame_data.primary_command_pool = m_vkDevice.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, m_vkGraphicsQueueIndex });
            vk::CommandBufferAllocateInfo command_buffer_allocate_info(frame_data.primary_command_pool, vk::CommandBufferLevel::ePrimary, 1);
            frame_data.primary_command_buffer = m_vkDevice.allocateCommandBuffers(command_buffer_allocate_info).front();
        }

        for (size_t i = 0; i < image_count; i++)
        {
            // Create an image view which we can render into.
            vk::ImageViewCreateInfo image_view_create_info({},
                swapchain_images[i],
                vk::ImageViewType::e2D,
                m_vkSwapchainData.format,
                { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

            vk::ImageView image_view = m_vkDevice.createImageView(image_view_create_info);
            if (!image_view) IFX_ERROR("Vulkan failed to create imageview");

            m_vkSwapchainData.image_views.push_back(image_view);
        }
    }

    void Graphics::CreateRenderPass()
    {
        IFX_TRACE("Window CreateRenderPass");
        vk::AttachmentDescription attachment({},
            m_vkSwapchainData.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR);

        vk::AttachmentReference color_ref(0, vk::ImageLayout::eColorAttachmentOptimal);

        // We will end up with two transitions.
        // The first one happens right before we start subpass #0, where
        // eUndefined is transitioned into eColorAttachmentOptimal.
        // The final layout in the render pass attachment states ePresentSrcKHR, so we
        // will get a final transition from eColorAttachmentOptimal to ePresetSrcKHR.
        vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, color_ref);

        // Create a dependency to external events.
        // We need to wait for the WSI semaphore to signal.
        // Only pipeline stages which depend on eColorAttachmentOutput will
        // actually wait for the semaphore, so we must also wait for that pipeline stage.
        vk::SubpassDependency dependency(/*srcSubpass   */ VK_SUBPASS_EXTERNAL,
            /*dstSubpass   */ 0,
            /*srcStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
            /*dstStageMask */ vk::PipelineStageFlagBits::eColorAttachmentOutput,
            // Since we changed the image layout, we need to make the memory visible to
            // color attachment to modify.
            /*srcAccessMask*/{},
            /*dstAccessMask*/ vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

        // Finally, create the renderpass.
        vk::RenderPassCreateInfo rp_info({}, attachment, subpass, dependency);
        m_vkRenderpass = m_vkDevice.createRenderPass(rp_info);



        if (!m_vkRenderpass) IFX_ERROR("Vulkan failed to create renderpass");

    }

    void Graphics::CreatePipeline()
    {
        IFX_TRACE("Window CreatePipeline");
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, create_shader_module(m_vkDevice, "assets/shaders/triangle.vert"), "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, create_shader_module(m_vkDevice, "assets/shaders/triangle.frag"), "main") };

        std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attrb_descs{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
        };

        auto vertex_input_binding_descs = Vertex::getBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_input_binding_descs, vertex_input_attrb_descs);

        // Our attachment will write to all color channels, but no blending is enabled.
        vk::PipelineColorBlendAttachmentState blend_attachment;
        blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        // Disable all depth testing.
        vk::PipelineDepthStencilStateCreateInfo depth_stencil;

        m_vkPipeline = create_graphics_pipeline(m_vkDevice,
            nullptr,
            shader_stages,
            vertex_input,
            vk::PrimitiveTopology::eTriangleList,        // We will use triangle lists to draw geometry.
            0,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eClockwise,
            { blend_attachment },
            depth_stencil,
            m_vkPipelineLayout,        // We need to specify the pipeline layout
            m_vkRenderpass);           // and the render pass up front as well

        if (!m_vkPipeline) IFX_ERROR("Vulkan failed to create graphics pipeline");
        // Pipeline is baked, we can delete the shader modules now.
        m_vkDevice.destroyShaderModule(shader_stages[0].module);
        m_vkDevice.destroyShaderModule(shader_stages[1].module);

    }

    void Graphics::CreateFramebuffers()
    {
        IFX_TRACE("Window CreateFramebuffers");
        if (!m_vkSwapchainData.framebuffers.empty())
        {
            for (vk::Framebuffer framebuffer : m_vkSwapchainData.framebuffers) { m_vkDevice.destroyFramebuffer(framebuffer); }
        }
        m_vkSwapchainData.framebuffers.clear();

        for (auto image_view : m_vkSwapchainData.image_views)
        {
            vk::FramebufferCreateInfo framebuffer_create_info({}, m_vkRenderpass, { image_view }, m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height, 1);
            m_vkSwapchainData.framebuffers.push_back(m_vkDevice.createFramebuffer(framebuffer_create_info));
        }
    }

    uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Graphics::CreateVertexBuffer()
    {
        auto vertex_buffer_create_info = vk::BufferCreateInfo({}, sizeof(Vertex) * m_VertexBuffer.size(), vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, 1);
        m_vkVertexBuffer = m_vkDevice.createBuffer(vertex_buffer_create_info);
        if (!m_vkVertexBuffer) IFX_ERROR("Vulkan failed to create vertex buffer");

        vk::MemoryRequirements memory_requirements = m_vkDevice.getBufferMemoryRequirements(m_vkVertexBuffer);

        uint32_t memory_type_index = findMemoryType(m_vkPhysicalDevice, memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
        vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size, memory_type_index);

        m_vkVertexBufferMemory = m_vkDevice.allocateMemory(memory_allocate_info);

        m_vkDevice.bindBufferMemory(m_vkVertexBuffer, m_vkVertexBufferMemory, 0);

        m_VertexArrayTransformed = static_cast<Vertex*>(m_vkDevice.mapMemory(m_vkVertexBufferMemory, 0, vertex_buffer_create_info.size));
        //memcpy(pdata, m_VertexBuffer.data(), vertex_buffer_create_info.size);
        //m_vkDevice.unmapMemory(m_vkVertexBufferMemory);

    }

    void Graphics::DestroySwapchain(vk::SwapchainKHR swapchain)
    {
        if (!swapchain) return;

        for (vk::ImageView image_view : m_vkSwapchainData.image_views) { m_vkDevice.destroyImageView(image_view); }
        m_vkSwapchainData.image_views.clear();

        for (auto& frame_data : m_vkFramesData) {
            if (frame_data.queue_submit_fence)
            {
                m_vkDevice.destroyFence(frame_data.queue_submit_fence);
                frame_data.queue_submit_fence = nullptr;
            }

            if (frame_data.primary_command_buffer)
            {
                m_vkDevice.freeCommandBuffers(frame_data.primary_command_pool, frame_data.primary_command_buffer);
                frame_data.primary_command_buffer = nullptr;
            }

            if (frame_data.primary_command_pool)
            {
                m_vkDevice.destroyCommandPool(frame_data.primary_command_pool);
                frame_data.primary_command_pool = nullptr;
            }

            if (frame_data.swapchain_acquire_semaphore)
            {
                m_vkDevice.destroySemaphore(frame_data.swapchain_acquire_semaphore);
                frame_data.swapchain_acquire_semaphore = nullptr;
            }

            if (frame_data.swapchain_release_semaphore)
            {
                m_vkDevice.destroySemaphore(frame_data.swapchain_release_semaphore);
                frame_data.swapchain_release_semaphore = nullptr;
            }
        }
        m_vkFramesData.clear();

        for (vk::Framebuffer framebuffer : m_vkSwapchainData.framebuffers) { m_vkDevice.destroyFramebuffer(framebuffer); }
        m_vkSwapchainData.framebuffers.clear();

        m_vkDevice.destroySwapchainKHR(swapchain);
    }
}