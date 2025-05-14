#pragma once

#include "render/renderer2d.h"
#include "GLFW/glfw3.h"

#include "input/event.h"
#include "core/input.h"

#include <glm/glm.hpp>

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

    class Window
    {
    public:

        Window(uint32_t width, uint32_t height, std::string title);

        void Init();
        void Shutdown();
        void Update();

        bool ShouldClose() const;
        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }
        inline float fGetWidth() const { return static_cast<float>(m_Width); }
        inline float fGetHeight() const { return static_cast<float>(m_Height); }
        inline std::string GetTitle() const { return m_Title; }
        
        using EventCallbackFn = std::function<void(Event&)>;
        inline void SetEventCallback(EventCallbackFn callback) { m_EventCallback = callback;}

    private:
        void InitGLFW();
        void InitVulkan();
        void ShutdownVulkan();

        GLFWwindow* m_glfwWindow = nullptr;

        std::string m_Title;
        unsigned int m_Width, m_Height;
        EventCallbackFn m_EventCallback;
        bool m_CursorState;
        bool m_CursorStateChange;
    };

}