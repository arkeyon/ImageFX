#pragma once

#include "GLFW/glfw3.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace saf {

    class Window
    {
    public:

        Window(uint32_t width, uint32_t height, const char* title);
        void Update();

        inline bool ShouldClose() const { return glfwWindowShouldClose(m_glfwWindow); }
        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }
        inline const char* GetTitle() const { return m_Title; }

        Window& operator&(const Window&) = delete;
        Window& operator&(Window&&) = delete;
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        ~Window();

    private:
        bool Init();

        uint32_t m_Width, m_Height;
        const char* m_Title;

        GLFWwindow* m_glfwWindow;
        vk::Instance m_vkInstance;
    };

}