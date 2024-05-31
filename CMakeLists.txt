cmake_minimum_required(VERSION 3.1...3.25)
project(
	LearnWebGPU
	VERSION 0.1.0
	LANGUAGES CXX C
)

include(utils.cmake)

# We add an option to enable different settings when developping the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)
add_subdirectory(imgui)

add_executable(Template
	main.cpp
	Renderer.h
	Renderer.cpp
	Primitives.h
	Primitives.cpp
	ResourceManager.h
	pipelines/InstancingPipeline.cpp
	pipelines/InstancingPipeline.h
	pipelines/LinePipeline.cpp
	pipelines/LinePipeline.h
	pipelines/PostProcessingPipeline.cpp
	pipelines/PostProcessingPipeline.h
	ResourceManager.cpp
	implementations.cpp
	Simulator.cpp
)

if(DEV_MODE)
	# In dev mode, we load resources from the source tree, so that when we
	# dynamically edit resources (like shaders), these are correctly
	# versionned.
	target_compile_definitions(Template PRIVATE
		RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
	)
else()
	# In release mode, we just load resources relatively to wherever the
	# executable is launched from, so that the binary is portable
	target_compile_definitions(Template PRIVATE
		RESOURCE_DIR="./resources"
	)
endif()

target_include_directories(Template PRIVATE .)

target_link_libraries(Template PRIVATE glfw webgpu glfw3webgpu imgui)

set_target_properties(Template PROPERTIES
	CXX_STANDARD 17
	VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
)
target_treat_all_warnings_as_errors(Template)
target_copy_webgpu_binaries(Template)

if (MSVC)
	# Ignore a warning that GLM requires to bypass
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(Template PUBLIC /wd4201)
	# Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
	target_compile_options(Template PUBLIC /wd4305)

	# Ignore a warning that stb_image requires to bypass
	# Disable warning C4244: conversion from 'int' to 'short', possible loss of data
	target_compile_options(Template PUBLIC /wd4244)
endif (MSVC)
