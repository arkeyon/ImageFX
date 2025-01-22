set(VULKAN_SDK_PATH "/home/arkeyon/Downloads/1.4.304.0/x86_64")

cmake_minimum_required(VERSION 3.21...3.25)
project(ImageFXproject VERSION 1.0
		DESCRIPTION "Image Effects Project"
		LANGUAGES CXX)

add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/glfw)

find_package(Vulkan REQUIRED)

add_executable(ImageFX
	main.cpp
	src/core/entrypoint.cpp
	src/core/application.cpp src/core/application.h
	src/utils/stbimpl.cpp
	src/utils/argumentmanager.cpp src/utils/argumentmanager.h
	src/utils/log.cpp src/utils/log.h
	src/core/window.cpp src/core/window.h
)

target_compile_features(ImageFX PUBLIC cxx_std_20)
target_compile_options(ImageFX PUBLIC "-Wall")
target_precompile_headers(ImageFX PUBLIC src/safpch.h)

target_include_directories(ImageFX PUBLIC src)
target_include_directories(ImageFX PUBLIC dependencies/spdlog/include)
target_include_directories(ImageFX PUBLIC dependencies/glfw/include)
target_include_directories(ImageFX PUBLIC dependencies/stb)
target_include_directories(ImageFX PUBLIC dependencies)

target_link_libraries(ImageFX PRIVATE glfw)
target_link_libraries(ImageFX PRIVATE Vulkan::Vulkan)