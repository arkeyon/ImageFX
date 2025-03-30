

#include "glm/glm/fwd.hpp"
#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.hpp>

#include <deque>

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
        //std::vector<vk::Framebuffer> framebuffers;
    };

    struct FrameData
    {
        vk::Fence         queue_submit_fence;
        vk::CommandPool   primary_command_pool;
        vk::CommandBuffer primary_command_buffer;
        vk::Semaphore     swapchain_acquire_semaphore;
        vk::Semaphore     swapchain_release_semaphore;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec4 color;
        glm::vec2 tex_coord;

        static std::array<vk::VertexInputBindingDescription, 1> getBindingDescription() {
            return { vk::VertexInputBindingDescription(0, sizeof(Vertex)) };
        }
    };

    struct Uniform {
        glm::mat4 projection_view;
        glm::mat4 model;
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

        bool Render();
        void DrawString(std::string text, glm::vec3 position, float scale = 1.f);
        inline void ResetVertexBuffer() { m_QuadCount = 0; }

        glm::vec3 color = glm::vec3(0.5f, 0.5f, 0.f);
	private:
        void CreateSwapchain();
        void CreateDescriptors();
        void CreatePipeline();
        void CreateVertexBuffer();
        void CreateIndexBuffer();
        uint8_t* SetupFont(const std::string& fontFile);

        void RenderTri(uint32_t index);
        void DestroySwapchain(vk::SwapchainKHR old_swapchain);

        uint32_t m_Width{};
        uint32_t m_Height{};

        const uint32_t m_MaxQuads = 1000;
        Vertex* m_VertexBuffer;
        uint32_t* m_IndexBuffer;
        uint32_t m_QuadCount = 0;

        vk::Buffer m_vkVertexBuffer = nullptr;
        vk::Buffer m_vkIndexBuffer = nullptr;

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
        //vk::RenderPass m_vkRenderpass = nullptr;
        vk::PipelineLayout m_vkPipelineLayout{};
        vk::Pipeline m_vkPipeline = nullptr;
        std::vector<FrameData> m_vkFramesData{};
        std::vector<vk::Semaphore> m_vkRecycleSemaphores{};

        //std::deque<std::function<void()>> m_vkDeletionQueue;


        vk::DescriptorPool m_vkDescriptorPool;

        vma::Allocator m_vmaAllocator{};
        vma::Allocation m_vmaVertexBufferAllocation{};
        vma::Allocation m_vmaIndexBufferAllocation{};
        vma::Allocation m_vmaFontAtlasAllocation;
        vk::Image m_vkFontAtlas{};
        vk::ImageView m_vkFontAtlasImageView;
        vk::DescriptorSet m_vkFontAtlasDescriptorSet;
        vk::DescriptorSetLayout m_vkFontDescriptorSetLayout;
        vk::Sampler m_vkFontSampler;
	};

}