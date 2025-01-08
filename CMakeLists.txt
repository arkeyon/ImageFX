cmake_minimum_required(VERSION 3.16)
project(ImageFX VERSION 1.0
		DESCRIPTION "Image Effects Project"
		LANGUAGES CXX)

		#target_include_directories(ImageFXexe PUBLIC src)

add_executable(ImageFX src/main.cpp src/application.cpp)

target_compile_features(ImageFX PUBLIC cxx_std_20)
target_compile_options(ImageFX PUBLIC "-Wall")

target_include_directories(ImageFX PUBLIC src)