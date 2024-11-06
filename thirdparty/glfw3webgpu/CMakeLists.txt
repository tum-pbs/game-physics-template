# This is only meant to be included as a subdirectory in another project.
# It assumes that targets 'glfw' and 'webgpu' exist.
# Look at examples/CMakeLists.txt to see how to use it in a project.

# The glfw3webgpu target
add_library(glfw3webgpu STATIC glfw3webgpu.c)
target_include_directories(glfw3webgpu PUBLIC .)
target_link_libraries(glfw3webgpu PUBLIC glfw webgpu)

if(UNIX AND NOT APPLE)
	if (GLFW_BUILD_X11)
      	target_compile_definitions(glfw3webgpu PUBLIC _GLFW_X11=1)
	else()
		target_compile_definitions(glfw3webgpu PUBLIC _GLFW_WAYLAND=1)
	endif()
endif(UNIX AND NOT APPLE)

if (APPLE)
  target_compile_options(glfw3webgpu PRIVATE -x objective-c)
  target_link_libraries(glfw3webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo" "-framework IOKit" "-framework QuartzCore")
endif ()
