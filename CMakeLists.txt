
cmake_minimum_required(VERSION 3.28)
project(ImageFXproject VERSION 1.0
		DESCRIPTION "Image Effects Project"
		LANGUAGES CXX)

add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/glfw)

add_executable(ImageFX
	main.cpp
	src/core/entrypoint.cpp
	src/core/application.cpp src/core/application.h
	src/utils/stbimpl.cpp
	src/utils/argumentmanager.cpp src/utils/argumentmanager.h
	src/utils/log.cpp src/utils/log.h
	src/core/window.cpp src/core/window.h
	src/render/graphics.cpp src/render/graphics.h
	src/platform/vulkangraphics.cpp src/platform/vulkangraphics.h
)

target_compile_features(ImageFX PUBLIC cxx_std_20)
#target_compile_options(ImageFX PUBLIC "-Wall")

# find Vulkan SDK
find_package( Vulkan REQUIRED )

# Require Vulkan version ≥ 1.3.256 (earliest version when the Vulkan module was available)
if( ${Vulkan_VERSION} VERSION_LESS "1.3.256" )
  message( FATAL_ERROR "Minimum required Vulkan version for C++ modules is 1.3.256. "
           "Found ${Vulkan_VERSION}."
  )
endif()

# set up Vulkan C++ module as a library
add_library( VulkanHppModule )
target_sources( VulkanHppModule PRIVATE
  FILE_SET CXX_MODULES
  BASE_DIRS ${Vulkan_INCLUDE_DIR}
  FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)
target_compile_features( VulkanHppModule PUBLIC cxx_std_20 )
target_link_libraries( VulkanHppModule PUBLIC Vulkan::Vulkan )
target_compile_definitions( VulkanHppModule PUBLIC
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_compile_definitions( ImageFX PUBLIC
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

if (WIN32)
	target_compile_options(ImageFX PUBLIC "/utf-8")
endif(WIN32)

target_precompile_headers(ImageFX PRIVATE src/safpch.h)

target_include_directories(ImageFX PRIVATE src)
target_include_directories(ImageFX PUBLIC dependencies/spdlog/include)
target_include_directories(ImageFX PUBLIC dependencies/glfw/include)
target_include_directories(ImageFX PRIVATE dependencies/stb)
target_include_directories(ImageFX PRIVATE dependencies)

target_link_libraries(ImageFX PRIVATE glfw)
target_link_libraries(ImageFX PRIVATE VulkanHppModule)