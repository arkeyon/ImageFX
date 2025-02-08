#pragma once

#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <shaderc/shaderc.h>

#include <fstream>
#include <map>

#include <GLFW/glfw3.h>

namespace saf {

    struct SurfaceDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;

        std::vector<vk::SurfaceFormatKHR> formats;

        std::vector<vk::PresentModeKHR> presetmodes;
    };

    struct SwapchainData
    {
        vk::Extent2D                 extent { UINT32_MAX, UINT32_MAX };
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

    /*
    Vulkan Execution:

    - query layer/extention support
    - create instance
    - set physical device
    - find graphics queue family that supports graphics and preset
    - create logical device
    - create swapchain
    - 
    */

    inline vk::ShaderModule create_shader_module(vk::Device device, std::string path)
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

    inline vk::Pipeline create_graphics_pipeline(vk::Device       device,
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

    class Window
    {
    public:

        Window(uint32_t width, uint32_t height, const char* title);

        Window& operator&(const Window&) = delete;
        Window& operator&(Window&&) = delete;
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        ~Window();

        void Init();
        void Update();

        inline bool ShouldClose() const { return glfwWindowShouldClose(m_glfwWindow); }
        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }
        inline const char* GetTitle() const { return m_Title; }
    private:
        void InitGLFW();
        void CreateInstance();
        void CreateDebugMessenger();
        void SetPhysicalDevice();
        uint32_t FindQueueFamily(vk::QueueFlags flags, vk::SurfaceKHR surface = nullptr);
        void CreateDevice();
        void CreateSurface();
        void CreateSwapchain();
        void CreateRenderPass();
        void CreatePipeline();
        void CreateFramebuffers();
        void RenderTri(uint32_t index);

        void DestroySwapchain(vk::SwapchainKHR old_swapchain);

        uint32_t m_Width, m_Height;
        const char* m_Title;

        GLFWwindow* m_glfwWindow = nullptr;
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