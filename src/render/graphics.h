

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

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
        std::vector<vk::Framebuffer> framebuffers;
    };

    struct FrameData
    {
        vk::Fence         queue_submit_fence;
        vk::CommandPool   primary_command_pool;
        vk::CommandBuffer primary_command_buffer;
        vk::Semaphore     swapchain_acquire_semaphore;
        vk::Semaphore     swapchain_release_semaphore;
    };

	class Graphics {
	public:
        Graphics();
        Graphics& operator&(const Graphics&) = delete;
        Graphics& operator&(Graphics&&) = delete;
        Graphics(const Graphics&) = delete;
        Graphics(Graphics&&) = delete;
        ~Graphics();

		void Init(GLFWwindow* glfw_window, uint32_t width, uint32_t height);
        void Destroy();

        void Render();
	private:
        void CreateInstance();
        void CreateDebugMessenger();
        void SetPhysicalDevice();
        uint32_t FindQueueFamily(vk::QueueFlags flags, vk::SurfaceKHR surface = nullptr);
        void CreateDevice();
        void CreateSurface(GLFWwindow* glfw_window);
        void CreateSwapchain();
        void CreateRenderPass();
        void CreatePipeline();
        void CreateFramebuffers();
        void RenderTri(uint32_t index);

        void DestroySwapchain(vk::SwapchainKHR old_swapchain);

        uint32_t m_Width;
        uint32_t m_Height;

        std::vector<const char*> m_vkLayers{};
        std::vector<const char*> m_vkExtensions{};
        vk::Instance m_vkInstance = nullptr;
        vk::DebugUtilsMessengerEXT m_vkMessenger = nullptr;
        vk::PhysicalDevice m_vkPhysicalDevice = nullptr;
        uint32_t m_vkGraphicsQueueIndex = UINT32_MAX;
        vk::Device m_vkDevice = nullptr;
        vk::SurfaceKHR m_vkSurface = nullptr;
        SwapchainData m_vkSwapchainData{};
        vk::Queue m_vkQueue = nullptr;
        vk::RenderPass m_vkRenderpass = nullptr;
        vk::PipelineLayout m_vkPipelineLayout{};
        vk::Pipeline m_vkPipeline = nullptr;
        std::vector<FrameData> m_vkFramesData{};
        std::vector<vk::Semaphore> m_vkRecycleSemaphores{};
	};

}