#pragma once

#include <vector>

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

#include "render/graphics.h"

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

        uint32_t m_Width, m_Height;
        const char* m_Title;

        GLFWwindow* m_glfwWindow = nullptr;
        std::unique_ptr<Graphics> m_Graphics;

        std::string str;
    };

}