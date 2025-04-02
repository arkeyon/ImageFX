

#include <glm/fwd.hpp>
#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.hpp>

#include <deque>

#include "render/renderer2d.h"

namespace saf { 

    struct SurfaceDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;

        std::vector<vk::SurfaceFormatKHR> formats;

        std::vector<vk::PresentModeKHR> presetmodes;
    };

    struct SwapchainData
    {
        vk::Extent2D                 extent{ UINT32_MAX, UINT32_MAX };
        vk::Format                   format = vk::Format::eUndefined;
        vk::SwapchainKHR             swapchain = nullptr;
        std::vector<vk::ImageView>   image_views;
    };

    struct FrameData
    {
        vk::Fence         queue_submit_fence;
        vk::CommandPool   primary_command_pool;
        vk::CommandBuffer primary_command_buffer;
        vk::Semaphore     swapchain_acquire_semaphore;
        vk::Semaphore     swapchain_release_semaphore;
    };

    class FrameManager
    {
    public:
        FrameManager(uint32_t width, uint32_t height);

        void Init();

        void Resize(uint32_t width, uint32_t height);
        void CreateSwapchain();
        void DestroySwapchain(vk::SwapchainKHR old_swapchain);

        bool Render(std::shared_ptr<Renderer2D> renderer2d);

        void Destroy();
    private:
        uint32_t m_Width{};
        uint32_t m_Height{};

        SwapchainData m_vkSwapchainData{};
        std::vector<FrameData> m_vkFramesData{};
        std::vector<vk::Semaphore> m_vkRecycleSemaphores{};
    };
}