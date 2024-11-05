cmake_minimum_required(VERSION 3.16...3.25)

project(
	GamePhysicsTemplate
	VERSION 0.1.0
	LANGUAGES CXX C
	)
	
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)



if(UNIX)
	if (DEFINED ENV{XDG_SESSION_TYPE})
		if ($ENV{XDG_SESSION_TYPE} STREQUAL "x11")
			set(display_server_x11 TRUE)
			set(GLFW_BUILD_X11 TRUE)
			set(GLFW_BUILD_WAYLAND FALSE)
			message(" Using X11 (detected via XDG_SESSION_TYPE)" )
		else()
			set(display_server_x11 FALSE)
			set(GLFW_BUILD_X11 FALSE)
			set(GLFW_BUILD_WAYLAND TRUE)
			# add_compile_definitions(_GLFW_WAYLAND=1)
			message("Using Wayland (detected via XDG_SESSION_TYPE)")
		endif()
	else()
		set(display_server_x11 FALSE)
		set(GLFW_BUILD_WAYLAND TRUE)
		set(GLFW_BUILD_X11 FALSE)
		message("Odd Configuration detected, XDG_SESSION_TYPE not set! If you are on WSL2 this is expected, otherwise make sure your display settings are sane.")
	endif()
endif(UNIX)

option(DEV_MODE "Set up development helper settings" ON)
add_subdirectory(thirdparty/webgpu)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glfw3webgpu)
add_subdirectory(thirdparty/imgui)

option(USE_HIGH_PERFORMANCE_GPU " Use high performance gpu by default, mostly the discrete GPU" ON)
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

if(UNIX)
	if (DEFINED ENV{XDG_SESSION_TYPE})
		if ($ENV{XDG_SESSION_TYPE} STREQUAL "x11")
			target_compile_definitions(Template PUBLIC _GLFW_X11=1)
			target_compile_definitions(imgui PUBLIC _GLFW_X11=1)
			target_compile_definitions(glfw3webgpu PUBLIC _GLFW_X11=1)
		else()
			target_compile_definitions(Template PUBLIC _GLFW_WAYLAND=1)
			target_compile_definitions(imgui PUBLIC _GLFW_WAYLAND=1)
			target_compile_definitions(glfw3webgpu PUBLIC _GLFW_WAYLAND=1)
			message("Using Wayland!")
		endif()
	else()
		target_compile_definitions(Template PUBLIC _GLFW_WAYLAND=1)
		target_compile_definitions(imgui PUBLIC _GLFW_WAYLAND=1)
		target_compile_definitions(glfw3webgpu PUBLIC _GLFW_WAYLAND=1)
	endif()
endif(UNIX)

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
