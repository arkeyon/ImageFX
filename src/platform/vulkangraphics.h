#pragma once

#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.h>
#include <fstream>
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.hpp>

namespace saf {

	namespace vkhelper {

        [[nodiscard]] inline uint32_t FindQueueFamily(vk::Instance instance, vk::PhysicalDevice physical_device, vk::QueueFlags flags, vk::SurfaceKHR surface)
        {
            IFX_TRACE("Window FindQueueFamily");
            if (!physical_device)
            {
                IFX_ERROR("Vulkan no physical device set");
                return UINT32_MAX;
            }

            std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();

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
                if (surface && physical_device)
                {
                    //if (m_vkPhysicalDevice.getSurfaceSupportKHR(i, surface) == VK_SUCCESS) can_preset = true;
                    if (glfwGetPhysicalDevicePresentationSupport(static_cast<VkInstance>(instance), static_cast<VkPhysicalDevice>(physical_device), i) == GLFW_TRUE) can_preset = true;
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

        [[nodiscard]] inline vk::Pipeline CreateGraphicsPipeline(
            vk::Device                                                device,
            vk::PipelineCache                                         pipeline_cache,
            std::vector<vk::PipelineShaderStageCreateInfo> const&     shader_stages,
            vk::PipelineVertexInputStateCreateInfo const&             vertex_input_state,
            vk::PrimitiveTopology                                     primitive_topology,
            uint32_t                                                  patch_control_points,
            vk::PolygonMode                                           polygon_mode,
            vk::CullModeFlags                                         cull_mode,
            vk::FrontFace                                             front_face,
            std::vector<vk::PipelineColorBlendAttachmentState> const& blend_attachment_states,
            vk::PipelineDepthStencilStateCreateInfo const&            depth_stencil_state,
            vk::PipelineLayout                                        pipeline_layout,
            vk::Format                                                swapchain_format)
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

            vk::PipelineRenderingCreateInfo pipeline_rendering_create_info({}, { swapchain_format }, vk::Format::eUndefined, vk::Format::eUndefined);
            vk::GraphicsPipelineCreateInfo();
            // Final fullscreen composition pass pipeline
            vk::GraphicsPipelineCreateInfo pipeline_create_info(
                {},
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
                nullptr,
                {},
                {},
                -1
            );
            pipeline_create_info.pNext = &pipeline_rendering_create_info;

            vk::Result   result;
            vk::Pipeline pipeline;
            std::tie(result, pipeline) = device.createGraphicsPipeline(pipeline_cache, pipeline_create_info);
            assert(result == vk::Result::eSuccess);
            return pipeline;
        }

        [[nodiscard]] inline vk::ShaderModule CreateShaderModule(vk::Device device, std::string path)
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

        [[nodiscard]] inline vk::Instance CreateInstance(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
        {
            IFX_TRACE("Window CreateInstance");

            VULKAN_HPP_DEFAULT_DISPATCHER.init();


            auto supported_extentions = vk::enumerateInstanceExtensionProperties();

            auto required_extention_not_found = std::find_if(extensions.begin(),
                extensions.end(),
                [&supported_extentions](auto extension) {
                    return std::find_if(supported_extentions.begin(),
                        supported_extentions.end(),
                        [&extension](auto const& ep) {
                            return strcmp(ep.extensionName, extension) == 0;
                        }) == supported_extentions.end();
                });

            if (required_extention_not_found != extensions.end())
            {
                IFX_ERROR("Vulkan did not find the following required extentions:");
                for (; required_extention_not_found != extensions.end(); ++required_extention_not_found)
                {
                    IFX_ERROR("\t{0}", *required_extention_not_found);
                }
                IFX_ERROR("Vulkan missing required extention(s)");
            }

            auto supported_layers = vk::enumerateInstanceLayerProperties();

            auto required_layer_not_found = std::find_if(layers.begin(),
                layers.end(),
                [&supported_layers](auto extension) {
                    return std::find_if(supported_layers.begin(),
                        supported_layers.end(),
                        [&extension](auto const& ep) {
                            return strcmp(ep.layerName, extension) == 0;
                        }) == supported_layers.end();
                });

            if (required_layer_not_found != layers.end())
            {
                IFX_ERROR("Vulkan did not find the following required layers:");
                for (; required_layer_not_found != layers.end(); ++required_layer_not_found)
                {
                    IFX_ERROR("\t{0}", *required_layer_not_found);
                }
                IFX_ERROR("Vulkan missing required layer(s)");
            }

            const vk::ApplicationInfo applicationInfo(
                g_ApplicationName,
                VK_MAKE_API_VERSION(0, 1, 0, 0),
                g_EngineName,
                VK_MAKE_API_VERSION(0, 1, 0, 0),
                VK_API_VERSION_1_3
            );

            const vk::InstanceCreateInfo createinfo(
                vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
                &applicationInfo,
                layers.size(),
                layers.data(),
                extensions.size(),
                extensions.data()
            );

            vk::Instance instance = vk::createInstance(createinfo);

            VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

            return instance;
        }

        [[nodiscard]] inline vk::DebugUtilsMessengerEXT CreateDebugMessenger(vk::Instance instance)
        {
            IFX_TRACE("Window CreateDebugMessenger");
            vk::DebugUtilsMessengerCreateInfoEXT messengercreateinfo({}, vk::FlagTraits<vk::DebugUtilsMessageSeverityFlagBitsEXT>::allFlags, vk::FlagTraits<vk::DebugUtilsMessageTypeFlagBitsEXT>::allFlags,
                [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
                {


                    switch (messageSeverity)
                    {
                    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:   IFX_TRACE("VKDBG {0}", pCallbackData->pMessage); break;
                    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:      IFX_INFO("VKDBG {0}", pCallbackData->pMessage); break;
                    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:   IFX_WARN("VKDBG {0}", pCallbackData->pMessage); break;
                    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:     IFX_ERROR("VKDBG {0}", pCallbackData->pMessage); break;
                    case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
                    }

                    return VK_FALSE;
                });
            vk::DebugUtilsMessengerEXT messager = instance.createDebugUtilsMessengerEXT(messengercreateinfo);
            if (!messager) IFX_ERROR("Vulkan failed to create debug messenger");

            return messager;
        }

        [[nodiscard]] inline vk::PhysicalDevice SelectPhysicalDevice(vk::Instance instance)
        {
            std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
            if (devices.empty()) IFX_ERROR("Vulkan didnt find any physical devices");

            vk::PhysicalDevice physical_device = *std::find_if(devices.begin(), devices.end(), [] (vk::PhysicalDevice device)
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
            );

            if (!physical_device) IFX_ERROR("Vulkan failed to select physical device");

            return physical_device;
        }

        [[nodiscard]] inline vk::Device CreateLogicalDevice(vk::Instance instance, vk::PhysicalDevice physical_device, uint32_t graphics_queue_family, std::vector<const char*> layers = {}, std::vector<const char*> extensions = {})
        {

            const float queue_priority = 1.f;
            const vk::DeviceQueueCreateInfo queue_create_info({}, graphics_queue_family, 1U, &queue_priority );
            vk::PhysicalDeviceFeatures device_features{};
            device_features.samplerAnisotropy = vk::True;

            vk::PhysicalDeviceDynamicRenderingFeatures device_dynamic_features{};
            device_dynamic_features.dynamicRendering = VK_TRUE;
            const vk::DeviceCreateInfo device_create_info(
                {},
                1,
                &queue_create_info,
                static_cast<uint32_t>(layers.size()),
                layers.data(),
                static_cast<uint32_t>(extensions.size()),
                extensions.data(),
                &device_features,
                &device_dynamic_features
            );

            vk::Device device = physical_device.createDevice(device_create_info);

            if (!device)
            {
                IFX_ERROR("Vulkan failed to create logical device");
            }

            VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

            return device;
        }

        inline void immediate_submit(vk::Device device, uint32_t queue_index, std::function<void(vk::CommandBuffer cmd)> func)
        {
            vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
            vk::Fence fence = device.createFence(fence_create_info);
            device.resetFences(fence);

            vk::CommandPoolCreateInfo commandpool_create_info = vk::CommandPoolCreateInfo({ vk::CommandPoolCreateFlagBits::eTransient, queue_index });
            vk::CommandPool command_pool = device.createCommandPool(commandpool_create_info);

            vk::CommandBufferAllocateInfo command_buffer_allocate_info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::CommandBuffer cmd = device.allocateCommandBuffers(command_buffer_allocate_info).front();

            vk::CommandBufferBeginInfo command_buffer_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

            cmd.begin(command_buffer_begin_info);

            func(cmd);

            cmd.end();

            vk::SubmitInfo info(0U, nullptr, nullptr, 1, &cmd, 0, nullptr);

            device.getQueue(queue_index, 0).submit(info, fence);
            
            (void)device.waitForFences(fence, true, UINT64_MAX);

            device.destroyFence(fence);
            device.destroyCommandPool(command_pool);
        }

        [[nodiscard]] inline vk::Buffer create_buffer(uint32_t size, vk::BufferUsageFlags buffer_usage, vk::SharingMode sharing_mode, vma::MemoryUsage memory_usage, vma::AllocationCreateFlags memory_flags, vma::Allocator allocator, vma::Allocation& allocation)
        {
            const vk::BufferCreateInfo bufferInfo({}, size, buffer_usage, sharing_mode);

            vma::AllocationCreateInfo allocInfo = {};
            allocInfo.usage = memory_usage;
            allocInfo.flags = memory_flags;

            vk::Buffer buffer;

            if (allocator.createBuffer(
                &bufferInfo,
                &allocInfo,
                &buffer,
                &allocation,
                nullptr
            ) != vk::Result::eSuccess)
                IFX_ERROR("Failed to create buffer");

            return buffer;
        }

        [[nodiscard]] inline vk::Image CreateImage(vk::Device device, uint32_t queue_index, vma::Allocator allocator, uint32_t width, uint32_t height, uint32_t depth, uint8_t* rawimagedata, vma::Allocation& image_allocation)
        {
            vk::ImageCreateInfo image_create_info(
                {},
                vk::ImageType::e2D,
                {},
                vk::Extent3D(width, height, 1),
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                vk::SharingMode::eExclusive,
                {},
                {},
                vk::ImageLayout::eUndefined
            );

            switch (depth)
            {
            case 1: image_create_info.format = vk::Format::eR8Unorm; break;
            case 2: image_create_info.format = vk::Format::eR8G8Unorm; break;
            case 3: image_create_info.format = vk::Format::eR8G8B8Unorm; break;
            case 4: image_create_info.format = vk::Format::eR8G8B8A8Unorm; break;
            default: IFX_ERROR("Unsupported image depth"); break;
            }

            vma::AllocationCreateInfo alloc_create_info{};
            vk::Image image;

            alloc_create_info.flags = vma::AllocationCreateFlagBits::eDedicatedMemory;
            alloc_create_info.usage = vma::MemoryUsage::eGpuOnly;

            if (allocator.createImage(&image_create_info, &alloc_create_info, &image, &image_allocation, nullptr) != vk::Result::eSuccess)
                IFX_ERROR("Failed to create image");

            uint8_t* imagedata;

            vma::Allocation staging_allocation;
            vk::Buffer stageing_buffer = vkhelper::create_buffer(sizeof(uint8_t) * width * height * depth, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, vma::MemoryUsage::eCpuToGpu, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, allocator, staging_allocation);

            ERR_GUARD_VULKAN(vmaMapMemory(allocator, staging_allocation, reinterpret_cast<void**>(&imagedata)));
            memcpy(imagedata, rawimagedata, sizeof(uint8_t) * width * height * depth);
            vmaUnmapMemory(allocator, staging_allocation);

            vkhelper::immediate_submit(device, queue_index, [width, height, stageing_buffer, image](vk::CommandBuffer cmd)
                {
                    vk::ImageMemoryBarrier image_memory_barrier_pre(
                        vk::AccessFlagBits::eNone,
                        vk::AccessFlagBits::eTransferWrite,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image,
                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    );

                    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier_pre);

                    vk::BufferImageCopy buffer_image_copy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));

                    cmd.copyBufferToImage(stageing_buffer, image, vk::ImageLayout::eTransferDstOptimal, buffer_image_copy);

                    vk::ImageMemoryBarrier image_memory_barrier_post(
                        vk::AccessFlagBits::eTransferWrite,
                        vk::AccessFlagBits::eShaderRead,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        VK_QUEUE_FAMILY_IGNORED,
                        VK_QUEUE_FAMILY_IGNORED,
                        image,
                        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    );

                    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, image_memory_barrier_post);
                });

            allocator.destroyBuffer(stageing_buffer, staging_allocation);
            
            return image;
        }

        [[nodiscard]] inline vk::Sampler CreateFontSampler(vk::Device device)
        {
            vk::SamplerCreateInfo sampler_create_info{};
            sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
            sampler_create_info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            sampler_create_info.addressModeW = vk::SamplerAddressMode::eClampToEdge;

            sampler_create_info.maxLod = 0;
            sampler_create_info.minLod = 0;
            sampler_create_info.mipLodBias = 0.f;
            sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;

            sampler_create_info.compareEnable = vk::False;
            sampler_create_info.compareOp = vk::CompareOp::eAlways;

            sampler_create_info.minFilter = vk::Filter::eNearest;
            sampler_create_info.magFilter = vk::Filter::eLinear;
            sampler_create_info.anisotropyEnable = vk::True;
            sampler_create_info.maxAnisotropy = 1.f;
            sampler_create_info.unnormalizedCoordinates = vk::False;
            sampler_create_info.borderColor = vk::BorderColor::eFloatTransparentBlack;

            return device.createSampler(sampler_create_info);
        }
	}

}