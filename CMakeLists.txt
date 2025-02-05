
cmake_minimum_required(VERSION 3.28)
project(ImageFX VERSION 1.0
DESCRIPTION "Image Effects Project"
LANGUAGES CXX)


set($ENV{VULKAN_SDK} "/home/arkeyon/Downloads/1.4.304.0/x86_64")
set($ENV{Vulkan_INCLUDE_DIRS} "${VULKAN_SDK}/include")
set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK}/include")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/glfw)

add_executable(${PROJECT_NAME}
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

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
#target_compile_options(ImageFX PUBLIC "-Wall")

if (WIN32)
	target_compile_options(${PROJECT_NAME} PUBLIC "/utf-8")
endif(WIN32)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/safpch.h)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PUBLIC dependencies/spdlog/include)
target_include_directories(${PROJECT_NAME} PUBLIC dependencies/glfw/include)
target_include_directories(${PROJECT_NAME} PRIVATE dependencies/stb)
target_include_directories(${PROJECT_NAME} PRIVATE dependencies)

# find Vulkan SDK
find_package( Vulkan REQUIRED )

message( STATUS "Vulkan_LIBRARY: ${Vulkan_LIBRARY}" )
message( STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}" )
message( STATUS "Vulkan_VERSION: ${Vulkan_VERSION}" )

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
# Require Vulkan version ≥ 1.3.256 (earliest version when the Vulkan module was available)
if( ${Vulkan_VERSION} VERSION_LESS "1.3.256" OR NOT MSVC)
	message( WARNING "VulkanHppModule unsupported directling linking to vulkan")
	target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

	target_compile_definitions( ${PROJECT_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
else()

	add_library( VulkanHppModule )
	# set up Vulkan C++ module as a library
	target_sources( VulkanHppModule PRIVATE
  		FILE_SET CXX_MODULES
		BASE_DIRS ${Vulkan_INCLUDE_DIR}
		FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
	)
	target_compile_features( VulkanHppModule PUBLIC cxx_std_20 )
	target_link_libraries( VulkanHppModule PUBLIC Vulkan::Vulkan )
	target_compile_definitions( VulkanHppModule PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 )

	target_link_libraries(${PROJECT_NAME} PUBLIC VulkanHppModule )

endif()

