#include "safpch.h"
#include "window.h"

#include "utils/log.h"

#include <stddef.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace saf {

    Window::Window(uint32_t width, uint32_t height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
        m_Graphics = std::make_unique<Graphics>();

    }

    Window::~Window()
    {
        IFX_INFO("Window Shutdown");

        m_Graphics->Destroy();

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

        m_Graphics->Init(m_glfwWindow, m_Width, m_Height);
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_glfwWindow = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        if (!m_glfwWindow)
        {
            IFX_ERROR("GLFW failed to create window");
        }

        glfwSetKeyCallback(m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
            });

        glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            });
    }

    void Window::Update()
    {
        glfwPollEvents();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow(); // Show demo window! :)

        m_Graphics->Render();
    }

}