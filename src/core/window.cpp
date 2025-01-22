#include "safpch.h"
#include "window.h"

#include "GLFW/glfw3.h"

#include "utils/log.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>


namespace saf {

    Window::Window(uint32_t width, uint32_t height, const char* title)
    : m_Width(width), m_Height(height), m_Title(title)
    {
        if (Init()) IFX_TRACE("Window created");
        else IFX_ERROR("Window failed to be created");
    }

    bool Window::Init()
    {
        GLint glfwstatus = glfwInit();
        if (glfwstatus == GLFW_FALSE) 
        {
            IFX_ERROR("GLFW failed to initialise");
            return false;
        }

        if (!glfwVulkanSupported())
        {
            IFX_ERROR("Vulkan not supported");
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_glfwWindow = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        if (!m_glfwWindow)
        {
            IFX_ERROR("GLFW failed to create window");
            return false;
        }

        glfwSetKeyCallback(m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
        });

        glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        });

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        const std::array<const char*, 1> layers = {"VK_LAYER_KHRONOS_validation"};

        IFX_TRACE("Vulkan supports the following extentions: ");
        auto supported_extentions = vk::enumerateInstanceExtensionProperties();
        for (const auto& extention : supported_extentions)
        {
            IFX_TRACE("\t{0}", std::string(extention.extensionName));
        }

        IFX_TRACE("Vulkan supports the following layers: ");
        auto supported_layers = vk::enumerateInstanceLayerProperties();
        for (const auto& layers : supported_layers)
        {
            IFX_TRACE("\t{0}", std::string(layers.layerName));
        }

        std::vector<const char*> extentions;
        extentions.reserve(glfw_extension_count + 2);
        for (int n = 0; n < glfw_extension_count; ++n) extentions.emplace_back(glfw_extensions[n]);
        extentions.emplace_back("VK_KHR_portability_enumeration");
        extentions.emplace_back("VK_EXT_debug_utils");

        IFX_TRACE("Vulkan using following extentions:");
        for (const auto& extension : extentions) IFX_TRACE("\t{0}", extension);

        IFX_TRACE("Vulkan using following layers:");
        for (const auto& layer : layers) IFX_TRACE("\t{0}", layer);
        const vk::ApplicationInfo applicationInfo(m_Title, VK_MAKE_API_VERSION(0, 1, 0, 0), m_Title, VK_MAKE_API_VERSION(0, 1, 0, 0), VK_API_VERSION_1_1);
        const vk::InstanceCreateInfo createinfo(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &applicationInfo, layers.size(), layers.data(), extentions.size(), extentions.data());
        m_vkInstance = vk::createInstance(createinfo);

        /*
        vk::DebugUtilsMessengerCreateInfoEXT messengercreateinfo({}, vk::FlagTraits<vk::DebugUtilsMessageSeverityFlagBitsEXT>::allFlags, vk::FlagTraits<vk::DebugUtilsMessageTypeFlagBitsEXT>::allFlags,
            [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
            {
                switch (messageSeverity)
                {
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: IFX_TRACE("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: IFX_INFO("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: IFX_WARN("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: IFX_ERROR("Vulkan Debug {0}", pCallbackData->pMessage); break;
                case VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
                }

                return VK_FALSE;
            });
        auto mc = m_vkInstance.createDebugUtilsMessengerEXT(messengercreateinfo);*/
        
        return true;
    }

    Window::~Window()
    {
        m_vkInstance.destroy();

        glfwDestroyWindow(m_glfwWindow);

        glfwTerminate();
    }

    void Window::Update()
    {
        glfwPollEvents();
    }
}