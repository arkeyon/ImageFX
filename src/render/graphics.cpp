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

#include <glm/glm/gtc/matrix_transform.hpp>

#include "globals.h"

namespace saf {

    FrameManager::FrameManager(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {

    }

    void FrameManager::Init()
    {
        CreateSwapchain();
    }
    
    void FrameManager::Destroy()
    {
        for (auto semiphore : m_vkRecycleSemaphores) if (semiphore) global::g_Device.destroySemaphore(semiphore);
        if (m_vkSwapchainData.swapchain) DestroySwapchain(m_vkSwapchainData.swapchain);
    }

    void FrameManager::Resize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        global::g_Device.waitIdle();
        global::g_Renderer2D->m_Width = width;
        global::g_Renderer2D->m_Height = height;
        CreateSwapchain();
    }

    bool FrameManager::Render(std::shared_ptr<Renderer2D> renderer2d)
    {

        vk::Semaphore acquire_semaphore;
        if (m_vkRecycleSemaphores.empty())
        {
            acquire_semaphore = global::g_Device.createSemaphore({});
        }
        else
        {
            acquire_semaphore = m_vkRecycleSemaphores.back();
            m_vkRecycleSemaphores.pop_back();
        }

        vk::Result res;
        uint32_t   index;
        std::tie(res, index) = global::g_Device.acquireNextImageKHR(m_vkSwapchainData.swapchain, UINT64_MAX, acquire_semaphore);

        if (res != vk::Result::eSuccess)
        {
            m_vkRecycleSemaphores.push_back(acquire_semaphore);
            IFX_WARN("Vulkan failed acquireNextImage");
            global::g_GraphicsQueue.waitIdle();
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
            if (m_vkFramesData[index].queue_submit_fence)
            {
                (void)global::g_Device.waitForFences(m_vkFramesData[index].queue_submit_fence, true, UINT64_MAX);
                global::g_Device.resetFences(m_vkFramesData[index].queue_submit_fence);
            }

            if (m_vkFramesData[index].primary_command_pool)
            {
                global::g_Device.resetCommandPool(m_vkFramesData[index].primary_command_pool);
            }

            // Recycle the old semaphore back into the semaphore manager.
            vk::Semaphore old_semaphore = m_vkFramesData[index].swapchain_acquire_semaphore;

            if (old_semaphore)
            {
                m_vkRecycleSemaphores.push_back(old_semaphore);
            }

            m_vkFramesData[index].swapchain_acquire_semaphore = acquire_semaphore;
        }

        vk::CommandBuffer cmd = m_vkFramesData[index].primary_command_buffer;

        // We will only submit this once before it's recycled.
        vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        // Begin command recording

        vk::Image swapchain_image = global::g_Device.getSwapchainImagesKHR(m_vkSwapchainData.swapchain)[index];

        cmd.begin(begin_info);

        vk::ImageMemoryBarrier image_memory_barrier(
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
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

        vk::Viewport vp(0.0f, 0.0f, static_cast<float>(m_vkSwapchainData.extent.width), static_cast<float>(m_vkSwapchainData.extent.height), 0.0f, 1.0f);
        // Set viewport dynamically
        cmd.setViewport(0, vp);

        vk::Rect2D scissor({ 0, 0 }, { m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height });
        // Set scissor dynamically
        cmd.setScissor(0, scissor);

        renderer2d->Flush(cmd);

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
            m_vkFramesData[index].swapchain_release_semaphore = global::g_Device.createSemaphore({});
        }

        vk::PipelineStageFlags wait_stage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        vk::SubmitInfo info(
            m_vkFramesData[index].swapchain_acquire_semaphore,
            wait_stage,
            cmd,
            m_vkFramesData[index].swapchain_release_semaphore
        );
        // Submit command buffer to graphics queue
        global::g_GraphicsQueue.submit(info, m_vkFramesData[index].queue_submit_fence);

        // Present swapchain image
        vk::PresentInfoKHR present_info(m_vkFramesData[index].swapchain_release_semaphore, m_vkSwapchainData.swapchain, index);
        res = global::g_GraphicsQueue.presentKHR(present_info);

        if (res != vk::Result::eSuccess)
        {
            IFX_ERROR("Failed to present swapchain image.");
        }
        
        return true;
    }

    void FrameManager::CreateSwapchain()
    {
        IFX_TRACE("Window CreateSwapchain");
        vk::SurfaceCapabilitiesKHR capabilities = global::g_PhysicalDevice.getSurfaceCapabilitiesKHR(global::g_Surface);
        std::vector<vk::PresentModeKHR> supported_preset_modes = global::g_PhysicalDevice.getSurfacePresentModesKHR(global::g_Surface);

        if (supported_preset_modes.size() == 0) IFX_ERROR("Vulkan surface doesnt support any presentModes?");

        m_vkSwapchainData.format = global::g_SurfaceFormat.format;

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
            global::g_Surface,                                        //surface_               = {}
            desired_swapchain_images,                           //minImageCount_         = {}
            global::g_SurfaceFormat.format,                                //imageFormat_           = VULKAN_HPP_NAMESPACE::Format::eUndefined
            global::g_SurfaceFormat.colorSpace,                            //imageColorSpace_       = VULKAN_HPP_NAMESPACE::ColorSpaceKHR::eSrgbNonlinear
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

        m_vkSwapchainData.swapchain = global::g_Device.createSwapchainKHR(swapchain_create_info);
        if (!m_vkSwapchainData.swapchain) IFX_ERROR("Vulkan failed to create swapchain");

        DestroySwapchain(old_swapchain);

        std::vector<vk::Image> swapchain_images = global::g_Device.getSwapchainImagesKHR(m_vkSwapchainData.swapchain);
        size_t image_count = swapchain_images.size();

        // Initialize per-frame resources.
        // Every swapchain image has its own command pool and fence manager.
        // This makes it very easy to keep track of when we can reset command buffers and such.
        m_vkFramesData.clear();
        m_vkFramesData.resize(image_count);

        for (auto& frame_data : m_vkFramesData)
        {
            frame_data.queue_submit_fence = global::g_Device.createFence({ vk::FenceCreateFlagBits::eSignaled });
            frame_data.primary_command_pool = global::g_Device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, global::g_GraphicsQueueIndex });
            vk::CommandBufferAllocateInfo command_buffer_allocate_info(frame_data.primary_command_pool, vk::CommandBufferLevel::ePrimary, 1);
            frame_data.primary_command_buffer = global::g_Device.allocateCommandBuffers(command_buffer_allocate_info).front();
        }

        for (size_t i = 0; i < image_count; i++)
        {
            // Create an image view which we can render into.
            vk::ImageViewCreateInfo image_view_create_info({},
                swapchain_images[i],
                vk::ImageViewType::e2D,
                global::g_SurfaceFormat.format,
                { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

            vk::ImageView image_view = global::g_Device.createImageView(image_view_create_info);
            if (!image_view) IFX_ERROR("Vulkan failed to create imageview");

            m_vkSwapchainData.image_views.push_back(image_view);
        }
    }

    void FrameManager::DestroySwapchain(vk::SwapchainKHR swapchain)
    {
        if (!swapchain) return;

        for (vk::ImageView image_view : m_vkSwapchainData.image_views) { global::g_Device.destroyImageView(image_view); }
        m_vkSwapchainData.image_views.clear();

        for (auto& frame_data : m_vkFramesData) {
            if (frame_data.queue_submit_fence)
            {
                global::g_Device.destroyFence(frame_data.queue_submit_fence);
                frame_data.queue_submit_fence = nullptr;
            }

            if (frame_data.primary_command_buffer)
            {
                global::g_Device.freeCommandBuffers(frame_data.primary_command_pool, frame_data.primary_command_buffer);
                frame_data.primary_command_buffer = nullptr;
            }

            if (frame_data.primary_command_pool)
            {
                global::g_Device.destroyCommandPool(frame_data.primary_command_pool);
                frame_data.primary_command_pool = nullptr;
            }

            if (frame_data.swapchain_acquire_semaphore)
            {
                global::g_Device.destroySemaphore(frame_data.swapchain_acquire_semaphore);
                frame_data.swapchain_acquire_semaphore = nullptr;
            }

            if (frame_data.swapchain_release_semaphore)
            {
                global::g_Device.destroySemaphore(frame_data.swapchain_release_semaphore);
                frame_data.swapchain_release_semaphore = nullptr;
            }
        }
        m_vkFramesData.clear();

        global::g_Device.destroySwapchainKHR(swapchain);
    }
}