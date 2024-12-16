// UFO
#include <ufo/glfw_webgpu/glfw_surface.h>

// WebGPU
#include <webgpu/webgpu.h>

// GLFW
#if defined(GLFW_EXPOSE_NATIVE_COCOA)
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

WGPUSurface glfwSurface(WGPUInstance instance, GLFWwindow* window)
{
	WGPUSurfaceDescriptor desc;
	desc.label = NULL;

#if defined(GLFW_EXPOSE_NATIVE_WAYLAND) && defined(GLFW_EXPOSE_NATIVE_X11)
	if (GLFW_PLATFORM_X11 == glfwGetPlatform()) {
		WGPUSurfaceDescriptorFromXlibWindow desc_x11;
		desc_x11.chain.next  = NULL;
		desc_x11.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
		desc_x11.display     = glfwGetX11Display();
		desc_x11.window      = glfwGetX11Window(window);

		desc.nextInChain = &desc_x11.chain;
	}
	if (GLFW_PLATFORM_WAYLAND == glfwGetPlatform()) {
		WGPUSurfaceDescriptorFromWaylandSurface desc_wl;
		desc_wl.chain.next  = NULL;
		desc_wl.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
		desc_wl.display     = glfwGetWaylandDisplay();
		desc_wl.surface     = glfwGetWaylandWindow(window);

		desc.nextInChain = &desc_wl.chain;
	}
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
	{
		WGPUSurfaceDescriptorFromMetalLayer desc_metal;
		desc_metal.chain.next  = NULL;
		desc_metal.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;

		id        metal_layer = NULL;
		NSWindow* ns_window   = glfwGetCocoaWindow(window);
		[ns_window.contentView setWantsLayer:YES];
		metal_layer = [CAMetalLayer layer];
		[ns_window.contentView setLayer:metal_layer];

		desc_metal.layer = metal_layer;

		desc.nextInChain = &desc_metal.chain;
	}
#elif defined(GLFW_EXPOSE_NATIVE_WIN32)
	{
		WGPUSurfaceDescriptorFromWindowsHWND desc_win;
		desc_win.chain.next  = NULL;
		desc_win.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
		desc_win.hinstance   = GetModuleHandle(NULL);
		desc_win.hwnd        = glfwGetWin32Window(window);

		desc.nextInChain = &desc_win.chain;
	}
#else
#error "Unsupported GLFW native platform"
#endif

	return wgpuInstanceCreateSurface(instance, &desc);
}