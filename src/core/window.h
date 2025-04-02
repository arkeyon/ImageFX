#pragma once

#include "render/renderer2d.h"
#include "GLFW/glfw3.h"

namespace saf {

    /*
    Vulkan Execution:

    - create instance (checking layer / extention support)
    - set physical device
    - find graphics queue family that supports graphics and preset
    - create logical device
    - create swapchain
    - create render pass
    - create framebuffers
    - create shader pipeline
    */

    class FrameManager;

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

        bool ShouldClose() const;
        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }
        inline const char* GetTitle() const { return m_Title; }

        std::string str;
    private:
        void InitGLFW();
        void InitVulkan();
        void ShutdownVulkan();

        uint32_t m_Width, m_Height;
        const char* m_Title;

        GLFWwindow* m_glfwWindow = nullptr;
        std::unique_ptr<FrameManager> m_FrameManager;

    };

}