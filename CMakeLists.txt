
cmake_minimum_required(VERSION 3.25...3.28)
project(ImageFX VERSION 1.0
DESCRIPTION "Image Effects Project"
LANGUAGES CXX)


#set($ENV{VULKAN_SDK} "/home/arkeyon/Downloads/1.4.304.0/x86_64")
#set($ENV{Vulkan_INCLUDE_DIRS} "${VULKAN_SDK}/include")
#set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK}/include")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")

add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/VulkanMemoryAllocator)
add_subdirectory(dependencies/VulkanMemoryAllocator-Hpp)

set(IMGUI_PATH ${CMAKE_CURRENT_LIST_DIR}/dependencies/imgui)
add_library(imgui
	${IMGUI_PATH}/imgui.h
	${IMGUI_PATH}/imgui.cpp
	${IMGUI_PATH}/imconfig.h
	${IMGUI_PATH}/imgui_demo.cpp
	${IMGUI_PATH}/imgui_draw.cpp
	${IMGUI_PATH}/imgui_internal.h
	${IMGUI_PATH}/imstb_rectpack.h
	${IMGUI_PATH}/imstb_textedit.h
	${IMGUI_PATH}/imstb_truetype.h
	${IMGUI_PATH}/imgui_tables.cpp
	${IMGUI_PATH}/imgui_widgets.cpp

	${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp
	${IMGUI_PATH}/backends/imgui_impl_vulkan.h
	${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
	${IMGUI_PATH}/backends/imgui_impl_glfw.h
)

target_include_directories(imgui PUBLIC ${IMGUI_PATH})
target_include_directories(imgui PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/glfw/include)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
target_link_libraries(imgui PRIVATE glfw)

add_library(${PROJECT_NAME}
	${PROJECT_SOURCE_DIR}/src/main.cpp
	${PROJECT_SOURCE_DIR}/src/utils/stbimpl.cpp
	${PROJECT_SOURCE_DIR}/src/utils/vmaimpl.cpp
	${PROJECT_SOURCE_DIR}/src/core/entrypoint.cpp
	${PROJECT_SOURCE_DIR}/src/core/window.cpp				${PROJECT_SOURCE_DIR}/src/core/window.h
	${PROJECT_SOURCE_DIR}/src/core/definitions.cpp			${PROJECT_SOURCE_DIR}/src/core/definitions.h
	${PROJECT_SOURCE_DIR}/src/core/application.cpp			${PROJECT_SOURCE_DIR}/src/core/application.h
	${PROJECT_SOURCE_DIR}/src/core/input.cpp				${PROJECT_SOURCE_DIR}/src/core/input.h
	${PROJECT_SOURCE_DIR}/src/render/graphics.cpp			${PROJECT_SOURCE_DIR}/src/render/graphics.h
	${PROJECT_SOURCE_DIR}/src/render/renderer2d.cpp			${PROJECT_SOURCE_DIR}/src/render/renderer2d.h
	${PROJECT_SOURCE_DIR}/src/render/shader.cpp				${PROJECT_SOURCE_DIR}/src/render/shader.h
	${PROJECT_SOURCE_DIR}/src/utils/argumentmanager.cpp		${PROJECT_SOURCE_DIR}/src/utils/argumentmanager.h
	${PROJECT_SOURCE_DIR}/src/utils/log.cpp					${PROJECT_SOURCE_DIR}/src/utils/log.h
	${PROJECT_SOURCE_DIR}/src/platform/vulkangraphics.cpp	${PROJECT_SOURCE_DIR}/src/platform/vulkangraphics.h
	${PROJECT_SOURCE_DIR}/src/globals.cpp					${PROJECT_SOURCE_DIR}/src/globals.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src)
#target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_PATH})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/spdlog/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/json/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dependencies)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/glfw/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/stb)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator-Hpp/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_precompile_headers(${PROJECT_NAME} PRIVATE src/safpch.h)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_options(ImageFX PUBLIC "-Wall")

if (MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC "-Wall;/utf-8")
endif()
##	file(COPY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
##else()
##	file(COPY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
##endif()
file(COPY ${PROJECT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
      $<$<CONFIG:Debug>:SAF_DEBUG>
      $<$<CONFIG:RelWithDebInfo>:SAF_DEBUG>
      $<$<CONFIG:Release>:SAF_RELEASE>
      $<$<CONFIG:MinSizeRel>:SAF_RELEASE>
	  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_link_libraries(${PROJECT_NAME} PUBLIC imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator-Hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::shaderc_combined)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)