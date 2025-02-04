#include "safpch.h"
#include "window.h"

#include "utils/log.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_hpp_macros.hpp>

#include "GLFW/glfw3.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace saf {

    Frame::Frame(vk::Image image)
        : m_Image(image)
    {

    }

    Window::Window(uint32_t width, uint32_t height, const char* title)
        : m_Width(width), m_Height(height), m_Title(title)
    {
        m_vkLayers = { "VK_LAYER_KHRONOS_validation" };
        m_vkExtensions = { "VK_KHR_portability_enumeration", "VK_EXT_debug_utils" };
    }

    void Window::Init()
    {
        InitGLFW();
        CreateInstance();
        CreateDebugMessenger();
        SetPhysicalDevice();

        if ((m_vkGraphicsQueueIndex = FindQueueFamily(vk::QueueFlagBits::eGraphics)) == -1)
        {
            IFX_ERROR("Window failed FindQueueFamily graphics");
        }
        else IFX_TRACE("Window FindQueueFamily graphics found at index: {0}", m_vkGraphicsQueueIndex);

        CreateDevice();
        CreateSurface();

        if ((m_vkPresentQueueIndex = FindQueueFamily({}, m_vkSurface)) == -1)
        {
            IFX_ERROR("Window failed FindQueueFamily present");
        }
        else IFX_TRACE("Window FindQueueFamily present found at index: {0}", m_vkPresentQueueIndex);
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
            throw std::exception("GLFW failed to initialize");
        }

        if (glfwVulkanSupported())
        {
            throw std::exception("Vulkan not support");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_glfwWindow = glfwCreateWindow(m_Width, m_Height, m_Title, nullptr, nullptr);

        if (!m_glfwWindow)
        {
            throw std::exception("GLFW failed to create window");
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

    void Window::CreateInstance()
    {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        for (int n = 0; n < glfw_extension_count; ++n) m_vkExtensions.emplace_back(glfw_extensions[n]);

        auto supported_extentions = vk::enumerateInstanceExtensionProperties();

        auto required_extention_not_found = std::find_if(m_vkExtensions.begin(),
            m_vkExtensions.end(),
            [&supported_extentions](auto extension) {
                return std::find_if(supported_extentions.begin(),
                    supported_extentions.end(),
                    [&extension](auto const& ep) {
                        return strcmp(ep.extensionName, extension) == 0;
                    }) == supported_extentions.end();
            });

        if (required_extention_not_found != m_vkExtensions.end())
        {
            IFX_ERROR("Vulkan did not find the following required extentions:");
            for (; required_extention_not_found != m_vkExtensions.end(); ++required_extention_not_found)
            {
                IFX_ERROR("\t{0}", *required_extention_not_found);
            }
            throw std::exception("Vulkan missing required extention(s)");
        }

        auto supported_layers = vk::enumerateInstanceLayerProperties();
        
        auto required_layer_not_found = std::find_if(m_vkLayers.begin(),
            m_vkLayers.end(),
            [&supported_layers](auto extension) {
                return std::find_if(supported_layers.begin(),
                    supported_layers.end(),
                    [&extension](auto const& ep) {
                        return strcmp(ep.layerName, extension) == 0;
                    }) == supported_layers.end();
            });

        if (required_layer_not_found != m_vkLayers.end())
        {
            IFX_ERROR("Vulkan did not find the following required layers:");
            for (; required_layer_not_found != m_vkLayers.end(); ++required_layer_not_found)
            {
                IFX_ERROR("\t{0}", *required_layer_not_found);
            }
            throw std::exception("Vulkan missing required layer(s)");
        }

        const vk::ApplicationInfo applicationInfo(m_Title, VK_MAKE_API_VERSION(0, 1, 0, 0), m_Title, VK_MAKE_API_VERSION(0, 1, 0, 0), VK_API_VERSION_1_1);
        const vk::InstanceCreateInfo createinfo(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, &applicationInfo, m_vkLayers.size(), m_vkLayers.data(), m_vkExtensions.size(), m_vkExtensions.data());
        m_vkInstance = vk::createInstance(createinfo);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkInstance);
    }

    void Window::CreateDebugMessenger()
    {
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
        m_vkMessenger = m_vkInstance.createDebugUtilsMessengerEXT(messengercreateinfo);
        if (!m_vkMessenger) throw std::exception("Vulkan failed to create debug messenger");
    }

    bool IsDevicesSuitable(vk::PhysicalDevice device)
    {
        auto properties = device.getProperties();
        IFX_TRACE("Device: {0}", properties.deviceName.data());

        switch (properties.deviceType)
        {
        case vk::PhysicalDeviceType::eCpu:              IFX_TRACE("\tType: CPU"); break;
        case vk::PhysicalDeviceType::eDiscreteGpu:      IFX_TRACE("\tType: Descrete GPU"); break;
        case vk::PhysicalDeviceType::eIntegratedGpu:    IFX_TRACE("\tType: Integrated GPU"); break;
        case vk::PhysicalDeviceType::eVirtualGpu:       IFX_TRACE("\tType: Virtual GPU"); break;
        case vk::PhysicalDeviceType::eOther:            IFX_TRACE("\tType: Unknown"); break;
        }

        std::string vendor = "UNKNOWN";

        switch (properties.vendorID)
        {
        case 0x1002: IFX_TRACE("\tVendorID: 0x1002 (AMD)"); vendor = "AMD"; break;
        case 0x1010: IFX_TRACE("\tVendorID: 0x1010 (ImgTec)"); vendor = "ImgTec"; break;
        case 0x10DE: IFX_TRACE("\tVendorID: 0x10DE (NVIDIA)"); vendor = "NVIDIA"; break;
        case 0x13B5: IFX_TRACE("\tVendorID: 0x13B5 (ARM)"); vendor = "ARM"; break;
        case 0x5143: IFX_TRACE("\tVendorID: 0x5143 (Qualcomm)"); vendor = "Qualcomm"; break;
        case 0x8086: IFX_TRACE("\tVendorID: 0x8086 (INTEL)"); vendor = "INTEL"; break;
        }

        IFX_TRACE("\tVersion: {0}.{1}.{2}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion));
        IFX_TRACE("\tAPIVersion: {0}.{1}.{2}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));

        return true;
    }

    void Window::SetPhysicalDevice()
    {
        std::vector<vk::PhysicalDevice> devices = m_vkInstance.enumeratePhysicalDevices();
        if (devices.empty()) throw std::exception("Vulkan didnt find any physical devices");

        m_vkPhysicalDevice = *std::find_if(devices.begin(), devices.end(), IsDevicesSuitable);
    }

    int32_t Window::FindQueueFamily(vk::QueueFlags flags, vk::SurfaceKHR surface)
    {
        if (!m_vkPhysicalDevice)
        {
            throw std::exception("Vulkan no physical device set");
            return -1;
        }

        std::vector<vk::QueueFamilyProperties> queue_family_properties = m_vkPhysicalDevice.getQueueFamilyProperties();
        
        if (queue_family_properties.empty())
        {
            IFX_WARN("Vulkan no queue families found on current physical device");
            return -1;
        }

        static bool first = true;

        if (first) IFX_TRACE("Vulkan {0} queue families found on current physical device", queue_family_properties.size());

        int queue_index = -1;
        int i = 0;
        for (auto qfp : queue_family_properties)
        {
            if (qfp.queueCount == 0) continue;

            bool can_preset = false;
            bool has_flags = false;

            if ((qfp.queueFlags & flags) == flags) has_flags = true;
            if (surface && m_vkPhysicalDevice)
            {
                if (m_vkPhysicalDevice.getSurfaceSupportKHR(i, surface) == VK_SUCCESS) can_preset = true;
            }

            bool queue_family_found = false;
            if (has_flags && (can_preset || !surface)) queue_family_found = true;

            if (first)
            {
                IFX_TRACE("Queue family {0}", i);
                std::string supports;
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eCompute) supports += "compute";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eGraphics) supports += supports.empty() ? "graphics" : ", graphics";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eOpticalFlowNV) supports += supports.empty() ? "opticalflownv" : ", opticalflownv";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eProtected) supports += supports.empty() ? "protected" : ", protected";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eSparseBinding) supports += supports.empty() ? "sparsebinding" : ", sparsebinding";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eTransfer) supports += supports.empty() ? "transfer" : ", transfer";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eVideoDecodeKHR) supports += supports.empty() ? "videodecode" : ", videodecode";
                if (qfp.queueFlags & qfp.queueFlags & vk::QueueFlagBits::eVideoEncodeKHR) supports += supports.empty() ? "videoencode" : ", videoencode";

                IFX_TRACE("\tSupports: {0}", supports);
                IFX_TRACE("\tFamily supports {0} queue{1}", qfp.queueCount, qfp.queueCount > 1 ? "s" : "");
            }
            else if (queue_family_found) return i;
            if (queue_family_found) queue_index = i;

            ++i;
        }

        first = false;

        return queue_index;
    }

    void Window::CreateDevice()
    {
        if (!m_vkPhysicalDevice)
        {
            throw std::exception("Vulkan no queue families found on current physical device");
        }

        std::array<const char*, 1> extensions = { "VK_KHR_swapchain" };

        float queue_priority = 1.f;
        const vk::DeviceQueueCreateInfo queue_create_info({}, m_vkGraphicsQueueIndex, 1, &queue_priority);
        const vk::PhysicalDeviceFeatures device_features{};
        const vk::DeviceCreateInfo device_create_info({}, 1, &queue_create_info, static_cast<uint32_t>(m_vkLayers.size()), m_vkLayers.data(), static_cast<uint32_t>(extensions.size()), extensions.data(), &device_features);
        m_vkDevice = m_vkPhysicalDevice.createDevice(device_create_info);

        if (!m_vkDevice)
        {
            throw std::exception("Vulkan failed to create logical device");
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkDevice);
    }

    void Window::CreateSurface()
    {
        VkResult result = glfwCreateWindowSurface(m_vkInstance, m_glfwWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_vkSurface));
        if (result != VK_SUCCESS || !m_vkSurface)
        {
            throw std::exception("Vulkan failed to create glfw surface");
        }
    }

    Window::~Window()
    {
        if (m_vkInstance)
        {
            if (m_vkSurface) m_vkInstance.destroySurfaceKHR(m_vkSurface);
            if (m_vkDevice) m_vkDevice.destroy();
            if (m_vkMessenger) m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkMessenger);
            m_vkInstance.destroy();
        }

        if (m_glfwWindow) glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
    }

    void Window::Update()
    {
        glfwPollEvents();
    }
}