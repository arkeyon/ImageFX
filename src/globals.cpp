#include "safpch.h"

#include "globals.h"

namespace saf {

	namespace global {

		vk::Instance g_Instance;
		vk::PhysicalDevice g_PhysicalDevice;
		vk::Device g_Device;

		uint32_t g_GraphicsQueueIndex;
		uint32_t g_PresentQueueIndex;
		uint32_t g_TransferQueueIndex;

		vk::Queue g_GraphicsQueue;

		vk::SurfaceKHR g_Surface;
		vk::SurfaceFormatKHR g_SurfaceFormat;
		vk::DebugUtilsMessengerEXT g_Messenger;
		vma::Allocator g_Allocator;
		vk::DescriptorPool g_DescriptorPool;

		std::vector<const char*> g_Layers{};
		std::vector<const char*> g_Extensions{};

		std::shared_ptr<const Input> g_Input;
		std::shared_ptr<const Window> g_Window;
	}

}