cmake_minimum_required(VERSION 3.1...3.25)

project(
	GamePhysicsTemplate
	VERSION 0.1.0
	LANGUAGES CXX C
	)
	
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/webgpu)
add_subdirectory(thirdparty/glfw3webgpu)
add_subdirectory(thirdparty/imgui)

option(USE_HIGH_PERFORMANCE_GPU "Use high performance GPU, most likely a discrete GPU, should be left ON except for trouble shooting purposes." ON)
if (USE_HIGH_PERFORMANCE_GPU)
    add_compile_definitions(WGPU_GPU_HIGH_PERFORMANCE="ON")
endif()

add_executable(Template
	src/implementations.cpp
	src/main.cpp
	src/Renderer.h
	src/Renderer.cpp
	src/Primitives.h
	src/Primitives.cpp
	thirdparty/stb_image.h
	src/ResourceManager.h
	src/ResourceManager.cpp
	src/Simulator.cpp
	src/Camera.h
	src/Camera.cpp
	src/Colormap.h
	src/Colormap.cpp
)

target_compile_definitions(Template PRIVATE
		GLM_FORCE_RIGHT_HANDED
		GLM_FORCE_DEPTH_ZERO_TO_ONE
	)

file(GLOB_RECURSE SCENE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.cpp
)

file(GLOB_RECURSE SCENE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.h
)
target_sources(Template PRIVATE ${SCENE_SOURCES} ${SCENE_HEADERS})

file(GLOB_RECURSE PIPELINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipelines/*.cpp
)

file(GLOB_RECURSE PIPELINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pipelines/*.h
)
target_sources(Template PRIVATE ${PIPELINE_SOURCES} ${PIPELINE_HEADERS})

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
target_include_directories(Template PRIVATE src)
target_include_directories(Template PRIVATE thirdparty)

target_link_libraries(Template PRIVATE glfw webgpu glfw3webgpu imgui)

set_target_properties(Template PROPERTIES
	CXX_STANDARD 17
)

target_copy_webgpu_binaries(Template)

if (MSVC)
	# Ignore a warning that GLM requires to bypass
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(Template PUBLIC /wd4201)
	# Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
	target_compile_options(Template PUBLIC /wd4305)
endif (MSVC)
