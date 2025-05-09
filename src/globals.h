#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>

#include "core/input.h"
#include "core/window.h"

namespace saf {

	namespace global {

		extern vk::Instance g_Instance;
		extern vk::PhysicalDevice g_PhysicalDevice;
		extern vk::Device g_Device;
		extern vk::DebugUtilsMessengerEXT g_Messenger;

		extern uint32_t g_GraphicsQueueIndex;
		extern uint32_t g_PresentQueueIndex;
		extern uint32_t g_TransferQueueIndex;
		extern uint32_t g_ComputeQueueIndex;

		extern vk::Queue g_GraphicsQueue;
		extern vk::Queue g_ComputeQueue;

		extern vk::SurfaceKHR g_Surface;
		extern vk::SurfaceFormatKHR g_SurfaceFormat;
		extern vma::Allocator g_Allocator;
		extern vk::DescriptorPool g_DescriptorPool;

		extern std::vector<const char*> g_Layers;
		extern std::vector<const char*> g_Extensions;

		extern std::shared_ptr<const Input> g_Input;
		extern std::shared_ptr<const Window> g_Window;
	}

}