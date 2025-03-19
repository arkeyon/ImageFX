#include "glm/glm/fwd.hpp"
#include "safpch.h"
#include "graphics.h"

#include <cstring>
#include <fstream>

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <vk_mem_alloc.h>

#include "platform/vulkangraphics.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace saf {

    Graphics::Graphics()
    {
        m_vkLayers = { "VK_LAYER_KHRONOS_validation" };
        m_vkExtensions = { "VK_KHR_portability_enumeration", "VK_EXT_debug_utils" };
    }

    Graphics::~Graphics()
    {

    }

	void Graphics::Init(GLFWwindow* glfw_window, uint32_t width, uint32_t height)
	{
        m_Width = width;
        m_Height = height;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        if (!glfw_extensions) IFX_ERROR("Vulkan not supported, glfw failed to fetch required extensions!");
        for (int n = 0; n < glfw_extension_count; ++n) m_vkExtensions.emplace_back(glfw_extensions[n]);

        m_vkInstance = vkhelper::CreateInstance(m_vkExtensions, m_vkLayers);
        m_vkMessenger = vkhelper::CreateDebugMessenger(m_vkInstance);
        
        ERR_GUARD_VULKAN(glfwCreateWindowSurface(m_vkInstance, glfw_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_vkSurface)));
        if (!m_vkSurface) IFX_ERROR("Vulkan failed to create glfw surface");

        m_vkPhysicalDevice = vkhelper::SelectPhysicalDevice(m_vkInstance);

        if ((m_vkGraphicsQueueIndex = vkhelper::FindQueueFamily(m_vkInstance, m_vkPhysicalDevice, vk::QueueFlagBits::eGraphics, m_vkSurface)) == UINT32_MAX)
        {
            IFX_ERROR("Window failed FindQueueFamily present");
        }
        else IFX_TRACE("Window FindQueueFamily present found at index: {0}", m_vkGraphicsQueueIndex);

        m_vkDevice = vkhelper::CreateLogicalDevice(
            m_vkInstance,
            m_vkPhysicalDevice,
            m_vkGraphicsQueueIndex,
            {},
            {
                "VK_KHR_swapchain",
                "VK_KHR_dynamic_rendering",
                "VK_KHR_dedicated_allocation"
            });

        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo vma_alloc_create_info = {};
        vma_alloc_create_info.device = m_vkDevice;
        vma_alloc_create_info.flags = {};
        vma_alloc_create_info.instance = m_vkInstance;
        vma_alloc_create_info.physicalDevice = m_vkPhysicalDevice;
        vma_alloc_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        vma_alloc_create_info.pVulkanFunctions = &vulkanFunctions;

        ERR_GUARD_VULKAN(vmaCreateAllocator(&vma_alloc_create_info, &m_vmaAllocator));

        m_vkQueue = m_vkDevice.getQueue(m_vkGraphicsQueueIndex, 0);

        CreateSwapchain();

        m_vkPipelineLayout = m_vkDevice.createPipelineLayout({});

        CreatePipeline();

        CreateVertexBuffer();

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info({}, { m_vkSwapchainData.format }, vk::Format::eUndefined, vk::Format::eUndefined);

        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
    
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
    
        m_vkDescriptorPool = m_vkDevice.createDescriptorPool(pool_info);

        ImGui_ImplVulkan_InitInfo imgui_vulkan_impl_info {};
        imgui_vulkan_impl_info.ApiVersion = VK_API_VERSION_1_3;
        imgui_vulkan_impl_info.Instance = m_vkInstance;
        imgui_vulkan_impl_info.PhysicalDevice = m_vkPhysicalDevice;
        imgui_vulkan_impl_info.Device = m_vkDevice;
        imgui_vulkan_impl_info.QueueFamily = m_vkGraphicsQueueIndex;
        imgui_vulkan_impl_info.Queue = m_vkQueue;
        imgui_vulkan_impl_info.MinImageCount = 3;
        imgui_vulkan_impl_info.ImageCount = 3;
        imgui_vulkan_impl_info.UseDynamicRendering = true;
        imgui_vulkan_impl_info.DescriptorPool = m_vkDescriptorPool;
        imgui_vulkan_impl_info.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfoKHR>(pipeline_rendering_create_info);
        imgui_vulkan_impl_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        //imgui_vulkan_impl_info.Allocation = m_vkAll

        ImGui_ImplVulkan_Init(&imgui_vulkan_impl_info);

        ImGui_ImplVulkan_CreateFontsTexture();
	}

	void Graphics::Destroy()
	{
        IFX_INFO("Vulkan Shutdown");

        if (m_vkDevice) m_vkDevice.waitIdle();

        ImGui_ImplVulkan_Shutdown();

        if (m_vkDescriptorPool) m_vkDevice.destroyDescriptorPool(m_vkDescriptorPool);
        if (m_VertexArrayTransformed) vmaUnmapMemory(m_vmaAllocator, m_vmaAllocation);
        if (m_vkVertexBuffer) vmaDestroyBuffer(m_vmaAllocator, m_vkVertexBuffer, m_vmaAllocation);
        /*if (m_vkVertexBufferMemory)
        {
            m_vkDevice.unmapMemory(m_vkVertexBufferMemory);
            m_vkDevice.freeMemory(m_vkVertexBufferMemory);
        }*/
        for (auto semiphore : m_vkRecycleSemaphores) if (semiphore) m_vkDevice.destroySemaphore(semiphore);
        if (m_vkSwapchainData.swapchain) DestroySwapchain(m_vkSwapchainData.swapchain);
        if (m_vkPipeline) m_vkDevice.destroy(m_vkPipeline);
        if (m_vkPipelineLayout) m_vkDevice.destroyPipelineLayout(m_vkPipelineLayout);
        //if (m_vkRenderpass) m_vkDevice.destroyRenderPass(m_vkRenderpass);
        if (m_vmaAllocator) vmaDestroyAllocator(m_vmaAllocator);
        if (m_vkDevice) m_vkDevice.destroy();
        if (m_vkSurface) m_vkInstance.destroySurfaceKHR(m_vkSurface);
        if (m_vkMessenger) m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkMessenger);
        if (m_vkInstance) m_vkInstance.destroy();
	}

    void Graphics::Render()
    {

        static float angle = 0.f;
        angle += 0.001f;

        //Vertex* vertices = static_cast<Vertex*>(m_vkDevice.mapMemory(m_vkVertexBufferMemory, 0, m_VertexBuffer.size() * sizeof(Vertex)));

        for (int i = 0; i < m_VertexBuffer.size(); i++)
        {
            auto& pos = m_VertexBuffer[i].pos;
            m_VertexArrayTransformed[i].pos.x = pos.x * cosf(angle) - pos.y * sinf(angle);
            m_VertexArrayTransformed[i].pos.y = pos.x * sinf(angle) + pos.y * cosf(angle);

            m_VertexArrayTransformed[i].color = color;
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
        //vk::Framebuffer framebuffer = m_vkSwapchainData.framebuffers[index];

        // Allocate or re-use a primary command buffer.
        vk::CommandBuffer cmd = m_vkFramesData[index].primary_command_buffer;

        // We will only submit this once before it's recycled.
        vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        // Begin command recording

        vk::Image swapchain_image = m_vkDevice.getSwapchainImagesKHR(m_vkSwapchainData.swapchain)[index];

        cmd.begin(begin_info);

        vk::ImageMemoryBarrier image_memory_barrier(
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            0U,
            0U,
            swapchain_image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,0U, 1U, 0U, 1U
            });

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, { image_memory_barrier });
        
        // Set clear color values.
        vk::ClearValue clear_value;
        clear_value.color = vk::ClearColorValue(std::array<float, 4>({ {0.01f, 0.01f, 0.033f, 1.0f} }));

        const vk::RenderingAttachmentInfo color_attachment_info(m_vkSwapchainData.image_views[index], vk::ImageLayout::eAttachmentOptimalKHR, vk::ResolveModeFlagBits::eNone, {}, vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clear_value);

        vk::RenderingInfo rendering_info({}, { {0, 0}, {m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height} }, 1, 0, { color_attachment_info }, nullptr, nullptr);

        cmd.beginRenderingKHR(rendering_info);

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
        cmd.draw(6, 1, 0, 0);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        cmd.endRenderingKHR();

        vk::ImageMemoryBarrier image_memory_barrier2(
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eNone,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            0U,
            0U,
            swapchain_image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,0U, 1U, 0U, 1U
            });

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, { image_memory_barrier2 });

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

    bool IsDevicesSuitable(vk::PhysicalDevice device)
    {
        

        return true;
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

        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
        for (const auto mode : supported_preset_modes)
        {
            if (mode == vk::PresentModeKHR::eMailbox)
            {
                present_mode = mode;
                break;
            }
        }

        if (present_mode == vk::PresentModeKHR::eFifo) IFX_TRACE("Vulkan physical device doesnt support mailbox present mode, defaulting to FiFo");
        else IFX_TRACE("Vulkan using mailbox present mode");

        vk::SwapchainCreateInfoKHR swapchain_create_info(
            {},                                                 //flags_                 = {}
            m_vkSurface,                                        //surface_               = {}
            desired_swapchain_images,                           //minImageCount_         = {}
            image_format.format,                                //imageFormat_           = VULKAN_HPP_NAMESPACE::Format::eUndefined
            image_format.colorSpace,                            //imageColorSpace_       = VULKAN_HPP_NAMESPACE::ColorSpaceKHR::eSrgbNonlinear
            extent,                                             //imageExtent_           = {}
            1,                                                  //imageArrayLayers_      = {}
            vk::ImageUsageFlagBits::eColorAttachment,           //imageUsage_            = {},
            vk::SharingMode::eExclusive,                        //imageSharingMode_      = VULKAN_HPP_NAMESPACE::SharingMode::eExclusive,
            {},                                                 //queueFamilyIndexCount_ = {},
            {},                                                 //pQueueFamilyIndices_   = {},
            pre_transform,                                      //preTransform_          = VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR::eIdentity,
            composite,                                          //compositeAlpha_        = VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,                                       //presentMode_           = VULKAN_HPP_NAMESPACE::PresentModeKHR::eImmediate,
            VK_TRUE,                                            //clipped_               = {},
            old_swapchain                                       //oldSwapchain_          = {},
        );

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

    void Graphics::CreatePipeline()
    {
        IFX_TRACE("Window CreatePipeline");
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
        {
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eVertex,
                vkhelper::CreateShaderModule(m_vkDevice, "assets/shaders/triangle.vert"),
                "main"
            ),
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eFragment,
                vkhelper::CreateShaderModule(m_vkDevice, "assets/shaders/triangle.frag"),
                "main"
            )
        };

        std::array<vk::VertexInputAttributeDescription, 2> vertex_input_attrb_descs{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
        };

        auto vertex_input_binding_descs = Vertex::getBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_input_binding_descs, vertex_input_attrb_descs);

        // Our attachment will write to all color channels, but no blending is enabled.
        vk::PipelineColorBlendAttachmentState blend_attachment;
        blend_attachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA;

        // Disable all depth testing.
        vk::PipelineDepthStencilStateCreateInfo depth_stencil;

        m_vkPipeline = vkhelper::CreateGraphicsPipeline(
            m_vkDevice,
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
            m_vkPipelineLayout,
            m_vkSwapchainData.format
        );        // We need to specify the pipeline layout

        if (!m_vkPipeline) IFX_ERROR("Vulkan failed to create graphics pipeline");
        // Pipeline is baked, we can delete the shader modules now.
        m_vkDevice.destroyShaderModule(shader_stages[0].module);
        m_vkDevice.destroyShaderModule(shader_stages[1].module);

    }

    void Graphics::CreateVertexBuffer()
    {
        /*
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
        */

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(Vertex) * m_VertexBuffer.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkResult res = vmaCreateBuffer(
            m_vmaAllocator,
            &bufferInfo,
            &allocInfo,
            reinterpret_cast<VkBuffer*>(&m_vkVertexBuffer),
            &m_vmaAllocation,
            nullptr
        );

        ERR_GUARD_VULKAN(vmaMapMemory(m_vmaAllocator, m_vmaAllocation, reinterpret_cast<void**>(&m_VertexArrayTransformed)));
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

        m_vkDevice.destroySwapchainKHR(swapchain);
    }
}