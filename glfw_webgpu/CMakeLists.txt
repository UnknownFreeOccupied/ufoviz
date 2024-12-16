add_library(glfw_webgpu STATIC src/glfw_surface.c)

target_include_directories(glfw_webgpu
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

message(CHECK_START "Finding glfw")
find_package(glfw3 3.4 QUIET)
if(glfw3_FOUND)
	message(CHECK_PASS "found, it is installed on the system")
else()
	message(CHECK_FAIL "not found, will fetch it instead")
	
	Include(FetchContent)

	FetchContent_Declare(
	  glfw
	  GIT_REPOSITORY https://github.com/glfw/glfw.git
	  GIT_TAG        7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # 3.4
	  GIT_PROGRESS   TRUE
	)

	FetchContent_MakeAvailable(glfw)
endif()

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