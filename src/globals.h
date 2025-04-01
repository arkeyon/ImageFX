#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "render/renderer2d.h"

namespace saf {

	namespace global {

		extern vk::Instance g_Instance;
		extern vk::PhysicalDevice g_PhysicalDevice;
		extern vk::Device g_Device;
		extern vk::DebugUtilsMessengerEXT g_Messenger;

		extern uint32_t g_GraphicsQueueIndex;
		extern uint32_t g_PresentQueueIndex;
		extern uint32_t g_TransferQueueIndex;

		extern vk::Queue g_GraphicsQueue;

		extern vk::SurfaceKHR g_Surface;
		extern vk::SurfaceFormatKHR g_SurfaceFormat;
		extern vma::Allocator g_Allocator;
		extern vk::DescriptorPool g_DescriptorPool;

		extern std::vector<const char*> g_Layers;
		extern std::vector<const char*> g_Extensions;

		extern std::shared_ptr<Renderer2D> g_Renderer2D;

	}

}