#pragma once

#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <GLFW/glfw3.h>

namespace saf {

    struct SurfaceDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;

        std::vector<vk::SurfaceFormatKHR> formats;

        std::vector<vk::PresentModeKHR> presetmodes;
    };

    class Frame {
    public:
        Frame(vk::Image image);

        vk::Image m_Image;
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

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
        int32_t FindQueueFamily(vk::QueueFlags flags, vk::SurfaceKHR surface = nullptr);
        void CreateDevice();
        void CreateSurface();

        uint32_t m_Width, m_Height;
        const char* m_Title;

        GLFWwindow* m_glfwWindow = nullptr;
        std::vector<const char*> m_vkLayers;
        std::vector<const char*> m_vkExtensions;
        vk::Instance m_vkInstance = nullptr;
        vk::DebugUtilsMessengerEXT m_vkMessenger = nullptr;
        vk::PhysicalDevice m_vkPhysicalDevice = nullptr;
        uint32_t m_vkGraphicsQueueIndex = -1;
        uint32_t m_vkPresentQueueIndex = -1;
        vk::Device m_vkDevice = nullptr;
        vk::SurfaceKHR m_vkSurface = nullptr;
        SwapChainSupportDetails m_vkSwapChainDetails {};
    };

}