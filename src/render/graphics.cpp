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

#include <stb/stb_truetype.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <glm/glm/gtx/matrix_transform.hpp>

namespace saf {

    // Font Atlas settings:
    const uint32_t codePointOfFirstChar = 32;      // ASCII of ' '(Space)
    const uint32_t charsToIncludeInFontAtlas = 95; // Include 95 charecters

    const uint32_t fontAtlasWidth = 512;
    const uint32_t fontAtlasHeight = 512;

    const float fontSize = 64.0f;

    stbtt_packedchar packedChars[charsToIncludeInFontAtlas];
    stbtt_aligned_quad alignedQuads[charsToIncludeInFontAtlas];


    Graphics::Graphics()
    {
        m_vkLayers = {
#ifdef SAF_DEBUG
            "VK_LAYER_KHRONOS_validation"
#endif
        };

        m_vkExtensions = {
            "VK_KHR_portability_enumeration",
#ifdef SAF_DEBUG
            "VK_EXT_debug_utils"

#endif
        };
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
#ifdef SAF_DEBUG
        m_vkMessenger = vkhelper::CreateDebugMessenger(m_vkInstance);
#endif
        
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

        vk::PipelineRenderingCreateInfo pipeline_rendering_create_info({}, { m_vkSwapchainData.format }, vk::Format::eUndefined, vk::Format::eUndefined);
        
        CreateDescriptors();

        ImGui_ImplVulkan_InitInfo imgui_vulkan_impl_info{};
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
                                                                 
        CreatePipeline();                                        
                                                                 
        CreateVertexBuffer();                                    
        CreateIndexBuffer();
	}

	void Graphics::Destroy()
	{
        IFX_INFO("Vulkan Shutdown");

        if (m_vkDevice) m_vkDevice.waitIdle();

        ImGui_ImplVulkan_Shutdown();

        if (m_vkDescriptorPool) m_vkDevice.destroyDescriptorPool(m_vkDescriptorPool);

        if (m_vkVertexBuffer) vmaDestroyBuffer(m_vmaAllocator, m_vkVertexBuffer, m_vmaVertexBufferAllocation);
        if (m_vkIndexBuffer) vmaDestroyBuffer(m_vmaAllocator, m_vkIndexBuffer, m_vmaIndexBufferAllocation);

        for (auto semiphore : m_vkRecycleSemaphores) if (semiphore) m_vkDevice.destroySemaphore(semiphore);
        if (m_vkSwapchainData.swapchain) DestroySwapchain(m_vkSwapchainData.swapchain);
        if (m_vkPipeline) m_vkDevice.destroy(m_vkPipeline);
        if (m_vkPipelineLayout) m_vkDevice.destroyPipelineLayout(m_vkPipelineLayout);
        if (m_vmaAllocator) vmaDestroyAllocator(m_vmaAllocator);
        if (m_vkDevice) m_vkDevice.destroy();
        if (m_vkSurface) m_vkInstance.destroySurfaceKHR(m_vkSurface);
        if (m_vkMessenger) m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkMessenger);
        if (m_vkInstance) m_vkInstance.destroy();
	}

    bool Graphics::Render()
    {

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
            return false;
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
        
        return true;
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
        cmd.bindIndexBuffer(m_vkIndexBuffer, 0, vk::IndexType::eUint32);

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_vkPipelineLayout, 0, { m_vkDescriptorSet }, {});

        // Draw three vertices with one instance.
        cmd.drawIndexed(6, 1, 0, 0, 0);

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

        vk::SubmitInfo info(
            m_vkFramesData[index].swapchain_acquire_semaphore,
            wait_stage,
            cmd,
            m_vkFramesData[index].swapchain_release_semaphore
        );
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

    void Graphics::CreateDescriptors()
    {
        std::array<vk::DescriptorPoolSize, 11> pool_sizes =
        {
            vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
        };

        vk::DescriptorPoolCreateInfo pool_create_info{};
        pool_create_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        pool_create_info.maxSets = 1000;
        pool_create_info.poolSizeCount = std::size(pool_sizes);
        pool_create_info.pPoolSizes = pool_sizes.data();

        m_vkDescriptorPool = m_vkDevice.createDescriptorPool(pool_create_info);

        uint8_t* rawfontdata = SetupFont("C:/Windows/Fonts/arial.ttf");

        vk::ImageCreateInfo image_create_info(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8Unorm,
            vk::Extent3D(fontAtlasWidth, fontAtlasHeight, 1),
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::SharingMode::eExclusive,
            {},
            {},
            vk::ImageLayout::eUndefined
        );
        VmaAllocationCreateInfo alloc_create_info{};
        alloc_create_info.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        alloc_create_info.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        ERR_GUARD_VULKAN(vmaCreateImage(m_vmaAllocator, reinterpret_cast<VkImageCreateInfo*>(&image_create_info), &alloc_create_info, reinterpret_cast<VkImage*>(&m_vkFontAtlas), &m_vmaFontAtlasAllocation, nullptr));

        VmaAllocation allocation;
        vk::Buffer stageing_buffer = vkhelper::create_buffer(sizeof(uint8_t) * fontAtlasWidth * fontAtlasHeight * 4, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_vmaAllocator, allocation);

        uint8_t* fontdata;
        ERR_GUARD_VULKAN(vmaMapMemory(m_vmaAllocator, allocation, reinterpret_cast<void**>(&fontdata)));
        memcpy(fontdata, rawfontdata, sizeof(uint8_t) * fontAtlasWidth * fontAtlasHeight);
        vmaUnmapMemory(m_vmaAllocator, allocation);

        vkhelper::immediate_submit(m_vkDevice, m_vkGraphicsQueueIndex, [stageing_buffer, this](vk::CommandBuffer cmd)
            {
                vk::ImageMemoryBarrier image_memory_barrier_pre(
                    vk::AccessFlagBits::eNone,
                    vk::AccessFlagBits::eTransferWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    m_vkFontAtlas,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );

                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier_pre);

                vk::BufferImageCopy buffer_image_copy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0), vk::Extent3D(fontAtlasWidth, fontAtlasHeight, 1));

                cmd.copyBufferToImage(stageing_buffer, m_vkFontAtlas, vk::ImageLayout::eTransferDstOptimal, buffer_image_copy);

                vk::ImageMemoryBarrier image_memory_barrier_post(
                    vk::AccessFlagBits::eTransferWrite,
                    vk::AccessFlagBits::eShaderRead,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    m_vkFontAtlas,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );

                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, image_memory_barrier_post);
            });

        vmaDestroyBuffer(m_vmaAllocator, stageing_buffer, allocation);

        vk::ImageViewCreateInfo imageview_create_info({}, m_vkFontAtlas, vk::ImageViewType::e2D, vk::Format::eR8Unorm, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        vk::ImageView image_view = m_vkDevice.createImageView(imageview_create_info);

        vk::SamplerCreateInfo sampler_create_info{};
        sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
        sampler_create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
        sampler_create_info.addressModeW = vk::SamplerAddressMode::eRepeat;

        sampler_create_info.maxLod = 0;
        sampler_create_info.minLod = 0;
        sampler_create_info.mipLodBias = 0.f;
        sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;

        sampler_create_info.compareEnable = vk::False;
        sampler_create_info.compareOp = vk::CompareOp::eAlways;

        sampler_create_info.minFilter = vk::Filter::eLinear;
        sampler_create_info.magFilter = vk::Filter::eLinear;
        sampler_create_info.anisotropyEnable = vk::True;
        sampler_create_info.maxAnisotropy = 1.f;
        sampler_create_info.unnormalizedCoordinates = vk::False;
        sampler_create_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;

        vk::Sampler sampler = m_vkDevice.createSampler(sampler_create_info);

        vk::DescriptorSetLayoutBinding desc_layout_binding{};
        desc_layout_binding.binding = 0;
        desc_layout_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        desc_layout_binding.descriptorCount = 1;
        desc_layout_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        desc_layout_binding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo desc_layout_info({}, { desc_layout_binding });
        vk::DescriptorSetLayout desc_layout = m_vkDevice.createDescriptorSetLayout(desc_layout_info);

        vk::DescriptorSetAllocateInfo desc_alloc_info{};
        desc_alloc_info.descriptorPool = m_vkDescriptorPool;
        desc_alloc_info.descriptorSetCount = 1;
        desc_alloc_info.pSetLayouts = &desc_layout;

        m_vkDescriptorSet = m_vkDevice.allocateDescriptorSets(desc_alloc_info)[0];

        vk::DescriptorImageInfo desc_image_info{};
        desc_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        desc_image_info.imageView = image_view;
        desc_image_info.sampler = sampler;

        vk::WriteDescriptorSet desc_set_write(m_vkDescriptorSet, desc_layout_binding.binding, 0, vk::DescriptorType::eCombinedImageSampler, desc_image_info);
        m_vkDevice.updateDescriptorSets(desc_set_write, {});

        vk::PipelineLayoutCreateInfo pipeline_layout_info({}, desc_layout);
        m_vkPipelineLayout = m_vkDevice.createPipelineLayout({ pipeline_layout_info });
    }

    void Graphics::CreatePipeline()
    {
        IFX_TRACE("Window CreatePipeline");
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
        {
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eVertex,
                vkhelper::CreateShaderModule(m_vkDevice, "assets/shaders/fonts.vert"),
                "main"
            ),
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eFragment,
                vkhelper::CreateShaderModule(m_vkDevice, "assets/shaders/fonts.frag"),
                "main"
            )
        };

        std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attrb_descs{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord))
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

        VmaAllocation allocation;
        vk::Buffer stage_buffer = vkhelper::create_buffer(sizeof(Vertex) * m_VertexBuffer.size(), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_vmaAllocator, allocation);
        void* data;
        ERR_GUARD_VULKAN(vmaMapMemory(m_vmaAllocator, allocation, &data));
        memcpy(data, static_cast<void*>(&m_VertexBuffer), sizeof(Vertex) * m_VertexBuffer.size());
        vmaUnmapMemory(m_vmaAllocator, allocation);

        m_vkVertexBuffer = vkhelper::create_buffer(sizeof(Vertex) * m_VertexBuffer.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY, VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, m_vmaAllocator, m_vmaVertexBufferAllocation);

        saf::vkhelper::immediate_submit(m_vkDevice, m_vkGraphicsQueueIndex, [&stage_buffer, this](vk::CommandBuffer cmd)
            {
                vk::BufferCopy buffer_copy(0, 0, m_VertexBuffer.size() * sizeof(Vertex));
                cmd.copyBuffer(stage_buffer, m_vkVertexBuffer, buffer_copy);
            });

        vmaDestroyBuffer(m_vmaAllocator, stage_buffer, allocation);
    }

    void Graphics::CreateIndexBuffer()
    {
        VmaAllocation allocation;
        vk::Buffer stage_buffer = vkhelper::create_buffer(sizeof(uint32_t) * m_IndexBuffer.size(), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_vmaAllocator, allocation);
        void* data;
        ERR_GUARD_VULKAN(vmaMapMemory(m_vmaAllocator, allocation, &data));
        memcpy(data, static_cast<void*>(&m_IndexBuffer), sizeof(uint32_t) * m_IndexBuffer.size());
        vmaUnmapMemory(m_vmaAllocator, allocation);

        m_vkIndexBuffer = vkhelper::create_buffer(sizeof(uint32_t) * m_IndexBuffer.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY, VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, m_vmaAllocator, m_vmaIndexBufferAllocation);

        saf::vkhelper::immediate_submit(m_vkDevice, m_vkGraphicsQueueIndex, [&stage_buffer, this](vk::CommandBuffer cmd)
            {
                vk::BufferCopy buffer_copy(0, 0, m_IndexBuffer.size() * sizeof(uint32_t));
                cmd.copyBuffer(stage_buffer, m_vkIndexBuffer, buffer_copy);
            });

        vmaDestroyBuffer(m_vmaAllocator, stage_buffer, allocation);
    }

    uint8_t* Graphics::SetupFont(const std::string& fontFile)
    {
        // Read the font file
        std::ifstream inputStream(fontFile.c_str(), std::ios::binary);

        inputStream.seekg(0, std::ios::end);
        auto&& fontFileSize = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);

        uint8_t* fontDataBuf = new uint8_t[fontFileSize];

        inputStream.read((char*)fontDataBuf, fontFileSize);

        stbtt_fontinfo fontInfo = {};

        uint32_t fontCount = stbtt_GetNumberOfFonts(fontDataBuf);
        std::cout << "Font File: " << fontFile << " has " << fontCount << " fonts\n";

        if (!stbtt_InitFont(&fontInfo, fontDataBuf, 0))
            std::cerr << "stbtt_InitFont() Failed!\n";

        uint8_t* fontAtlasTextureData = new uint8_t[fontAtlasWidth * fontAtlasHeight];

        stbtt_pack_context ctx;

        stbtt_PackBegin(
            &ctx,                                     // stbtt_pack_context (this call will initialize it) 
            (unsigned char*)fontAtlasTextureData,     // Font Atlas texture data
            fontAtlasWidth,                           // Width of the font atlas texture
            fontAtlasHeight,                          // Height of the font atlas texture
            0,                                        // Stride in bytes
            1,                                        // Padding between the glyphs
            nullptr);

        stbtt_PackFontRange(
            &ctx,                                     // stbtt_pack_context
            fontDataBuf,                              // Font Atlas texture data
            0,                                        // Font Index                                 
            fontSize,                                 // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points) 
            codePointOfFirstChar,                     // Code point of the first charecter
            charsToIncludeInFontAtlas,                // No. of charecters to be included in the font atlas 
            packedChars                    // stbtt_packedchar array, this struct will contain the data to render a glyph
        );
        stbtt_PackEnd(&ctx);

        for (int i = 0; i < charsToIncludeInFontAtlas; i++)
        {
            float unusedX, unusedY;

            stbtt_GetPackedQuad(
                packedChars,              // Array of stbtt_packedchar
                fontAtlasWidth,                      // Width of the font atlas texture
                fontAtlasHeight,                     // Height of the font atlas texture
                i,                                   // Index of the glyph
                &unusedX, &unusedY,                  // current position of the glyph in screen pixel coordinates, (not required as we have a different corrdinate system)
                &alignedQuads[i],         // stbtt_alligned_quad struct. (this struct mainly consists of the texture coordinates)
                0                                    // Allign X and Y position to a integer (doesn't matter because we are not using 'unusedX' and 'unusedY')
            );
        }

        delete[] fontDataBuf;

        // Optionally write the font atlas texture as a png file.

        stbi_write_png("fontAtlas.png", fontAtlasWidth, fontAtlasHeight, 1, fontAtlasTextureData, fontAtlasWidth);

        return fontAtlasTextureData;
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