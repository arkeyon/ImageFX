#include "glm/glm/fwd.hpp"
#include "safpch.h"
#include "window.h"

#include "utils/log.h"

#include <cstdint>
#include <stddef.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "globals.h"
#include "platform/vulkangraphics.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace saf {

    Window::Window(uint32_t width, uint32_t height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
        m_FrameManager = std::make_unique<FrameManager>(m_Width, m_Height);
        str = "Test";
    }

    Window::~Window()
    {
        IFX_INFO("Window Shutdown");

        if (global::g_Device) global::g_Device.waitIdle();

        m_FrameManager->Destroy();
        ShutdownVulkan();

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_glfwWindow) glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
    }

    void Window::Init()
    {
        InitGLFW();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplGlfw_InitForVulkan(m_glfwWindow, true);

        InitVulkan();
        m_FrameManager->Init();
    }

    void Window::InitGLFW()
    {

        glfwSetErrorCallback([](int error_code, const char* description)
            {
                IFX_ERROR(description);
            });

        GLint glfwstatus = glfwInit();

        if (glfwstatus == GLFW_FALSE)
        {
            IFX_ERROR("GLFW failed to initialize");
        }

        if (!glfwVulkanSupported())
        {
            IFX_ERROR("Vulkan not support");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_glfwWindow = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        if (!m_glfwWindow)
        {
            IFX_ERROR("GLFW failed to create window");
        }

        glfwSetWindowUserPointer(m_glfwWindow, static_cast<void*>(this));

        glfwSetCharCallback(m_glfwWindow, [](GLFWwindow* window, unsigned int codepoint)
            {
                Window* mywindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
                if (mywindow)
                {
                    char c = static_cast<char>(codepoint);
                    mywindow->str += c;
                }
            });

        glfwSetKeyCallback(m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                Window* mywindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

                if (action == GLFW_PRESS || action == GLFW_REPEAT)
                {
                    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
                    else if (key == GLFW_KEY_BACKSPACE)
                    {
                        mywindow->str = mywindow->str.substr(0, mywindow->str.length() - 1);
                    }
                    else if (key == GLFW_KEY_ENTER)
                    {
                        mywindow->str += '\n';
                    }
                }
            });

        glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            });

        glfwSetWindowSizeCallback(m_glfwWindow, [](GLFWwindow* window, int width, int height)
            {
                Window* mywindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
                mywindow->m_Width = width;
                mywindow->m_Height = height;
                mywindow->m_FrameManager->Resize(width, height);
            });
    }

    void Window::InitVulkan()
    {
        global::g_Layers = {
#ifdef SAF_DEBUG
            "VK_LAYER_KHRONOS_validation"
#endif
        };

        global::g_Extensions = {
            "VK_KHR_portability_enumeration",
#ifdef SAF_DEBUG
            "VK_EXT_debug_utils",
#endif
        };

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        if (!glfw_extensions) IFX_ERROR("Vulkan not supported, glfw failed to fetch required extensions!");
        for (int n = 0; n < glfw_extension_count; ++n) global::g_Extensions.emplace_back(glfw_extensions[n]);

        global::g_Instance = vkhelper::CreateInstance(global::g_Extensions, global::g_Layers);
#ifdef SAF_DEBUG
        global::g_Messenger = vkhelper::CreateDebugMessenger(global::g_Instance);
#endif

        ERR_GUARD_VULKAN(glfwCreateWindowSurface(global::g_Instance, m_glfwWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(&global::g_Surface)));
        if (!global::g_Surface) IFX_ERROR("Vulkan failed to create glfw surface");

        global::g_PhysicalDevice = vkhelper::SelectPhysicalDevice(global::g_Instance);

        if ((global::g_GraphicsQueueIndex = vkhelper::FindQueueFamily(global::g_Instance, global::g_PhysicalDevice, vk::QueueFlagBits::eGraphics, global::g_Surface)) == UINT32_MAX)
        {
            IFX_ERROR("Window failed FindQueueFamily present");
        }
        else IFX_TRACE("Window FindQueueFamily present found at index: {0}", global::g_GraphicsQueueIndex);

        global::g_Device = vkhelper::CreateLogicalDevice(
            global::g_Instance,
            global::g_PhysicalDevice,
            global::g_GraphicsQueueIndex,
            {},
            {
                "VK_KHR_swapchain",
                "VK_KHR_dynamic_rendering",
                "VK_KHR_dedicated_allocation"
            });


            std::vector<vk::SurfaceFormatKHR> supported_formats = global::g_PhysicalDevice.getSurfaceFormatsKHR(global::g_Surface);
            if (supported_formats.size() == 0) IFX_ERROR("Vulkan surface doesnt support any formats?");

            std::vector<vk::Format> prefered_formats = { vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32 };

            auto format_it = std::find_if(supported_formats.begin(), supported_formats.end(), [&prefered_formats](vk::SurfaceFormatKHR surface_format)
                {
                    return std::any_of(prefered_formats.begin(), prefered_formats.end(), [&surface_format](vk::Format pref_format)
                        {
                            return pref_format == surface_format.format;
                        });
                });

            vk::SurfaceFormatKHR image_format = (format_it == supported_formats.end()) ? *format_it : supported_formats[0];
            global::g_SurfaceFormat = image_format;

            vma::VulkanFunctions vulkanFunctions = {};
            vulkanFunctions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
            vulkanFunctions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

            vma::AllocatorCreateInfo vma_alloc_create_info = {};
            vma_alloc_create_info.device = global::g_Device;
            vma_alloc_create_info.flags = {};
            vma_alloc_create_info.instance = global::g_Instance;
            vma_alloc_create_info.physicalDevice = global::g_PhysicalDevice;
            vma_alloc_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
            vma_alloc_create_info.pVulkanFunctions = &vulkanFunctions;

            if (vma::createAllocator(&vma_alloc_create_info, &global::g_Allocator) != vk::Result::eSuccess)
                IFX_ERROR("Failed to create allocator");

            vk::PipelineRenderingCreateInfo pipeline_rendering_create_info({}, { global::g_SurfaceFormat.format }, vk::Format::eUndefined, vk::Format::eUndefined);

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

            global::g_DescriptorPool = global::g_Device.createDescriptorPool(pool_create_info);

            global::g_GraphicsQueue = global::g_Device.getQueue(global::g_GraphicsQueueIndex, 0);

            ImGui_ImplVulkan_InitInfo imgui_vulkan_impl_info{};
            imgui_vulkan_impl_info.ApiVersion = VK_API_VERSION_1_3;
            imgui_vulkan_impl_info.Instance = global::g_Instance;
            imgui_vulkan_impl_info.PhysicalDevice = global::g_PhysicalDevice;
            imgui_vulkan_impl_info.Device = global::g_Device;
            imgui_vulkan_impl_info.QueueFamily = global::g_GraphicsQueueIndex;
            imgui_vulkan_impl_info.Queue = global::g_GraphicsQueue;
            imgui_vulkan_impl_info.MinImageCount = 3;
            imgui_vulkan_impl_info.ImageCount = 3;
            imgui_vulkan_impl_info.UseDynamicRendering = true;
            imgui_vulkan_impl_info.DescriptorPool = global::g_DescriptorPool;
            imgui_vulkan_impl_info.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfoKHR>(pipeline_rendering_create_info);
            imgui_vulkan_impl_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            ImGui_ImplVulkan_Init(&imgui_vulkan_impl_info);

            ImGui_ImplVulkan_CreateFontsTexture();
    }

    void Window::ShutdownVulkan()
    {
        IFX_INFO("Vulkan Shutdown");

        if (global::g_Device) global::g_Device.waitIdle();

        ImGui_ImplVulkan_Shutdown();

        if (global::g_DescriptorPool) global::g_Device.destroyDescriptorPool(global::g_DescriptorPool);

        if (global::g_Allocator) global::g_Allocator.destroy();
        if (global::g_Device) global::g_Device.destroy();
        if (global::g_Surface) global::g_Instance.destroySurfaceKHR(global::g_Surface);
        if (global::g_Messenger) global::g_Instance.destroyDebugUtilsMessengerEXT(global::g_Messenger);
        if (global::g_Instance) global::g_Instance.destroy();
    }

    int printFPS() {
        static auto oldTime = std::chrono::high_resolution_clock::now();
        static int fps;
        fps++;


        static int second_fps = 0;

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - oldTime) >= std::chrono::seconds{ 1 }) {
            oldTime = std::chrono::high_resolution_clock::now();

            second_fps = fps;
            fps = 0;
        }

        return second_fps;
    }

    void Window::Update()
    {
        glfwPollEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static int fps = 0;

        ImGui::Begin("Debug");
        ImGui::Text("FPS: %d", fps);
        ImGui::End();

        if (m_FrameManager->Render(global::g_Renderer2D)) fps = printFPS();
    }

}