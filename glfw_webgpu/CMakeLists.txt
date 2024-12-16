add_library(glfw_webgpu STATIC src/glfw_surface.c)

target_include_directories(glfw_webgpu
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

if (WIN32)
	target_compile_definitions(glfw_webgpu
		PRIVATE
			GLFW_EXPOSE_NATIVE_WIN32
	)
	set(OS_LIBRARIES d3dcompiler ws2_32 userenv bcrypt ntdll opengl32)
elseif(UNIX AND NOT APPLE)
	target_compile_definitions(glfw_webgpu
		PRIVATE
			GLFW_EXPOSE_NATIVE_X11
			GLFW_EXPOSE_NATIVE_WAYLAND
	)
	set(OS_LIBRARIES "-lm -ldl")
elseif(APPLE)
	target_compile_options(glfw_webgpu PRIVATE -x objective-c)
	target_compile_definitions(glfw_webgpu
		PRIVATE
			GLFW_EXPOSE_NATIVE_COCOA
	)
	set(OS_LIBRARIES "-framework CoreFoundation -framework QuartzCore -framework Metal")
endif()

target_link_libraries(glfw_webgpu
	PUBLIC 
		glfw
		webgpu
	PRIVATE
		${OS_LIBRARIES}
)