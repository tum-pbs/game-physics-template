cmake_minimum_required(VERSION 3.1...3.25)
project(
	GamePhysicsTemplate
	VERSION 0.1.0
	LANGUAGES CXX C
)

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
	ResourceManager.cpp
	implementations.cpp
	Simulator.cpp
	Camera.h
	Camera.cpp
)



file(GLOB_RECURSE SCENE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.cpp
)

file(GLOB_RECURSE SCENE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Scenes/*.h
)
target_sources(Template PRIVATE ${SCENE_SOURCES} ${SCENE_HEADERS})

file(GLOB_RECURSE PIPELINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pipelines/*.cpp
)

file(GLOB_RECURSE PIPELINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/pipelines/*.h
)
target_sources(Template PRIVATE ${PIPELINE_SOURCES} ${PIPELINE_HEADERS})



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

target_copy_webgpu_binaries(Template)

if (MSVC)
	# Ignore a warning that GLM requires to bypass
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(Template PUBLIC /wd4201)
	# Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
	target_compile_options(Template PUBLIC /wd4305)
endif (MSVC)
