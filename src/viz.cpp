/*!
 * UFOMap: An Efficient Probabilistic 3D Mapping Framework That Embraces the Unknown
 *
 * @author Daniel Duberg (dduberg@kth.se)
 * @see https://github.com/UnknownFreeOccupied/ufomap
 * @version 1.0
 * @date 2022-05-13
 *
 * @copyright Copyright (c) 2022, Daniel Duberg, KTH Royal Institute of Technology
 *
 * BSD 3-Clause License
 *
 * Copyright (c) 2022, Daniel Duberg, KTH Royal Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// UFO
#include <ufo/glfw_webgpu/glfw_surface.h>

#include <ufo/viz/viz.hpp>

namespace ufo
{
Viz::Viz(std::string const& window_name, WGPUPowerPreference power_preference,
         WGPUBackendType backend_type)
    : window_name_(window_name)
{
	init(power_preference, backend_type);

	// TODO: Load config
}

Viz::~Viz() { stop(); }

void Viz::start(WGPUPowerPreference power_preference, WGPUBackendType backend_type)
{
	if (running()) {
		return;
	}

	if (nullptr == window_) {
		init(power_preference, backend_type);
	}
}

void Viz::stop()
{
	if (!running()) {
		return;
	}

	if (render_thread_.joinable()) {
		render_thread_.join();
	}

	for (auto& renderable : renderables_) {
		renderable->release();
	}

	wgpuSurfaceCapabilitiesFreeMembers(surface_capa_);
	wgpuQueueRelease(queue_);
	queue_ = nullptr;
	wgpuDeviceRelease(device_);
	device_ = nullptr;
	wgpuAdapterRelease(adapter_);
	adapter_ = nullptr;
	wgpuSurfaceRelease(surface_);
	surface_ = nullptr;
	glfwDestroyWindow(window_);
	window_ = nullptr;
	wgpuInstanceRelease(instance_);
	instance_ = nullptr;
	glfwTerminate();
}

void Viz::run()
{
#if defined(__EMSCRIPTEN__)
	auto callback = [](void* arg) {
		Viz* v = static_cast<Viz*>(arg);
		v->update();
	};
	emscripten_set_main_loop_arg(callback, this, 0, true);
#else
	while (running()) {
		update();
	}
#endif
}

void Viz::runAsync() { render_thread_ = std::thread(&Viz::run, this); }

bool Viz::running() const
{
	// TODO: Lock so window_ does not become nullptr
	return nullptr != window_ && !glfwWindowShouldClose(window_);
}

void Viz::update()
{
	float      cx     = std::cos(angles_.x);
	float      cy     = std::cos(angles_.y);
	float      sx     = std::sin(angles_.x);
	float      sy     = std::sin(angles_.y);
	ufo::Vec3f offset = ufo::Vec3f(cx * cy, sx * cy, sy) * std::exp(-zoom_);
	camera_.pose      = static_cast<ufo::Transform3f>(
      ufo::lookAt<float, true>(center_ + offset, center_, camera_.up));

	glfwPollEvents();
	float new_time   = glfwGetTime();
	float delta_time = new_time - prev_time_;
	prev_time_       = new_time;

	WGPUSurfaceTexture surface_texture;
	wgpuSurfaceGetCurrentTexture(surface_, &surface_texture);
	switch (surface_texture.status) {
		case WGPUSurfaceGetCurrentTextureStatus_Success:
			// All good, could check for `surface_texture.suboptimal` here.
			break;
		case WGPUSurfaceGetCurrentTextureStatus_Timeout:
		case WGPUSurfaceGetCurrentTextureStatus_Outdated:
		case WGPUSurfaceGetCurrentTextureStatus_Lost: {
			// Skip this frame, and re-configure surface.
			if (nullptr != surface_texture.texture) {
				wgpuTextureRelease(surface_texture.texture);
			}
			int width, height;
			glfwGetWindowSize(window_, &width, &height);
			if (0 != width && 0 != height) {
				surface_config_.width  = width;
				surface_config_.height = height;
				camera_.rows           = surface_config_.height;
				camera_.cols           = surface_config_.width;
				wgpuSurfaceConfigure(surface_, &surface_config_);
			}
			return;
		}
		case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
		case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
		case WGPUSurfaceGetCurrentTextureStatus_Force32:
			// Fatal error
			printf("get_current_texture status=%#.8x\n", surface_texture.status);
			abort();
	}
	assert(surface_texture.texture);

	// Create a view for this surface texture
	WGPUTextureViewDescriptor view_desc;
	view_desc.nextInChain     = nullptr;
	view_desc.label           = (window_name_ + " Surface Texture View").c_str();
	view_desc.format          = wgpuTextureGetFormat(surface_texture.texture);
	view_desc.dimension       = WGPUTextureViewDimension_2D;
	view_desc.baseMipLevel    = 0;
	view_desc.mipLevelCount   = 1;
	view_desc.baseArrayLayer  = 0;
	view_desc.arrayLayerCount = 1;
	view_desc.aspect          = WGPUTextureAspect_All;

	WGPUTextureView frame = wgpuTextureCreateView(surface_texture.texture, &view_desc);
	assert(frame);

	WGPUCommandEncoderDescriptor command_encoder_desc{};
	command_encoder_desc.nextInChain = nullptr;
	command_encoder_desc.label       = "command_encoder";

	WGPUCommandEncoder command_encoder =
	    wgpuDeviceCreateCommandEncoder(device_, &command_encoder_desc);
	assert(command_encoder);

	WGPUTextureDescriptor depth_texture_desc{};
	depth_texture_desc.label                   = "Depth Texture";
	depth_texture_desc.usage                   = WGPUTextureUsage_RenderAttachment;
	depth_texture_desc.dimension               = WGPUTextureDimension_2D;
	depth_texture_desc.size.width              = surface_config_.width;
	depth_texture_desc.size.height             = surface_config_.height;
	depth_texture_desc.size.depthOrArrayLayers = 1;
	depth_texture_desc.sampleCount             = 1;
	depth_texture_desc.format                  = WGPUTextureFormat_Depth32Float;
	depth_texture_desc.mipLevelCount           = 1;

	WGPUTexture depth_texture = wgpuDeviceCreateTexture(device_, &depth_texture_desc);

	WGPUTextureView depth_frame = wgpuTextureCreateView(depth_texture, nullptr);

	WGPURenderPassDepthStencilAttachment render_pass_depth_attachment{};
	render_pass_depth_attachment.view            = depth_frame;
	render_pass_depth_attachment.depthClearValue = 1.0;
	render_pass_depth_attachment.depthLoadOp     = WGPULoadOp_Clear;
	render_pass_depth_attachment.depthStoreOp    = WGPUStoreOp_Store;

	WGPURenderPassDescriptor render_pass_desc{};
	render_pass_desc.label                  = "render_pass_encoder";
	render_pass_desc.colorAttachmentCount   = 1;
	render_pass_desc.depthStencilAttachment = &render_pass_depth_attachment;

	WGPURenderPassColorAttachment render_pass_color_attachment{};
	render_pass_color_attachment.view          = frame;
	render_pass_color_attachment.resolveTarget = nullptr;
	render_pass_color_attachment.loadOp        = WGPULoadOp_Clear;
	render_pass_color_attachment.storeOp       = WGPUStoreOp_Store;
	render_pass_color_attachment.depthSlice    = WGPU_DEPTH_SLICE_UNDEFINED;
	render_pass_color_attachment.clearValue    = WGPUColor{0.0, 0.0, 0.0, 1.0};

	render_pass_desc.colorAttachments = &render_pass_color_attachment;

	WGPURenderPassEncoder render_pass_encoder =
	    wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);
	assert(render_pass_encoder);

	wgpuRenderPassEncoderEnd(render_pass_encoder);
	wgpuRenderPassEncoderRelease(render_pass_encoder);

	{
		std::scoped_lock lock(renderables_mutex_);
		for (auto& renderable : renderables_) {
			renderable->update(device_, command_encoder, frame, depth_frame, camera_);
		}
	}

	WGPUCommandBufferDescriptor command_buffer_desc{};
	command_buffer_desc.nextInChain = nullptr;
	command_buffer_desc.label       = "command_buffer";

	WGPUCommandBuffer command_buffer =
	    wgpuCommandEncoderFinish(command_encoder, &command_buffer_desc);
	assert(command_buffer);

	wgpuQueueSubmit(queue_, 1, &command_buffer);
	wgpuSurfacePresent(surface_);

	wgpuCommandBufferRelease(command_buffer);
	wgpuCommandEncoderRelease(command_encoder);
	wgpuTextureViewRelease(depth_frame);
	wgpuTextureRelease(depth_texture);
	wgpuTextureViewRelease(frame);
	wgpuTextureRelease(surface_texture.texture);
}

WGPUInstance Viz::instance() const { return instance_; }

WGPUAdapter Viz::adapter() const { return adapter_; }

WGPUDevice Viz::device() const { return device_; }

void Viz::addRenderable(std::shared_ptr<Renderable> const& renderable)
{
	std::scoped_lock lock(renderables_mutex_);
	renderables_.insert(renderable);

	if (nullptr != device_) {
		renderable->init(device_, surface_capa_.formats[0]);
	}
}

void Viz::eraseRenderable(std::shared_ptr<Renderable> const& renderable)
{
	std::scoped_lock lock(renderables_mutex_);
	renderables_.erase(renderable);

	renderable->release();
}

void Viz::clearRenderable()
{
	std::scoped_lock lock(renderables_mutex_);
	renderables_.clear();
}

void Viz::loadConfig()
{
	// TODO: Implement
}

void Viz::saveConfig() const
{
	// TODO: Implement
}

void Viz::init(WGPUPowerPreference power_preference, WGPUBackendType backend_type)
{
	surface_config_.width  = 640;
	surface_config_.height = 480;

	if (GLFW_TRUE != glfwInit()) {
		// TODO: Throw
	}

	instance_ = compute::createInstance();
	window_   = createWindow();
	surface_  = glfwSurface(instance_, window_);
	adapter_  = compute::createAdapter(instance_, surface_, power_preference, backend_type);
	auto required_limits = requiredLimits(adapter_);
	device_              = compute::createDevice(adapter_, required_limits);
	queue_               = compute::queue(device_);
	surface_capa_        = surfaceCapabilities(surface_, adapter_);
	surface_config_      = surfaceConfiguration(window_, device_, surface_capa_);

	wgpuSurfaceConfigure(surface_, &surface_config_);

	camera_.vertical_fov = ufo::radians(60.0f);
	camera_.near_clip    = 0.01;
	camera_.far_clip     = 10000.0;
	camera_.rows         = surface_config_.height;
	camera_.cols         = surface_config_.width;

	for (auto& renderable : renderables_) {
		renderable->init(device_, surface_capa_.formats[0]);
	}
}

GLFWwindow* Viz::createWindow() const
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(surface_config_.width, surface_config_.height,
	                                      window_name_.c_str(), nullptr, nullptr);

	// FIXME: Should this function be const with a const_cast?
	glfwSetWindowUserPointer(window, static_cast<void*>(const_cast<Viz*>(this)));

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		if (0 == width && 0 == height) {
			return;
		}

		Viz* v = static_cast<Viz*>(glfwGetWindowUserPointer(window));
		if (nullptr == v) {
			return;
		}

		v->surface_config_.width  = width;
		v->surface_config_.height = height;

		wgpuSurfaceConfigure(v->surface_, &v->surface_config_);
	});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x_pos, double y_pos) {
		Viz* v = static_cast<Viz*>(glfwGetWindowUserPointer(window));
		if (nullptr == v) {
			return;
		}
		v->onMouseMove(x_pos, y_pos);
	});

	glfwSetMouseButtonCallback(
	    window, [](GLFWwindow* window, int button, int action, int mods) {
		    Viz* v = static_cast<Viz*>(glfwGetWindowUserPointer(window));
		    if (nullptr == v) {
			    return;
		    }
		    v->onMouseButton(button, action, mods);
	    });

	glfwSetScrollCallback(window, [](GLFWwindow* window, double x_offset, double y_offset) {
		Viz* v = static_cast<Viz*>(glfwGetWindowUserPointer(window));
		if (nullptr == v) {
			return;
		}
		v->onScroll(x_offset, y_offset);
	});

	glfwSetKeyCallback(window,
	                   [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		                   Viz* v = static_cast<Viz*>(glfwGetWindowUserPointer(window));
		                   if (nullptr == v) {
			                   return;
		                   }
		                   v->onKey(key, scancode, action, mods);
	                   });

	return window;
}

WGPUSurfaceCapabilities Viz::surfaceCapabilities(WGPUSurface surface,
                                                 WGPUAdapter adapter) const
{
	WGPUSurfaceCapabilities surface_capabilities{};
	wgpuSurfaceGetCapabilities(surface, adapter, &surface_capabilities);
	return surface_capabilities;
}

WGPUSurfaceConfiguration Viz::surfaceConfiguration(
    GLFWwindow* window, WGPUDevice device, WGPUSurfaceCapabilities capabilities) const
{
	WGPUSurfaceConfiguration config{};
	config.nextInChain = nullptr;
	config.device      = device;
	config.usage       = WGPUTextureUsage_RenderAttachment;
	config.format      = capabilities.formats[0];
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode   = capabilities.alphaModes[0];

	// config.viewFormatCount = 0;
	// config.viewFormats     = nullptr;

	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		config.width  = width;
		config.height = height;
	}

	return config;
}

WGPURequiredLimits Viz::requiredLimits(WGPUAdapter adapter) const
{
	// TODO: Implement

	// Get adapter supported limits, in case we need them
	WGPUSupportedLimits supported{};
	supported.nextInChain = nullptr;
	wgpuAdapterGetLimits(adapter, &supported);

	WGPURequiredLimits required{};
	compute::setDefault(required.limits);

	// These two limits are different because they are "minimum" limits,
	// they are the only ones we may forward from the adapter's supported limits.
	required.limits.minUniformBufferOffsetAlignment =
	    supported.limits.minUniformBufferOffsetAlignment;
	required.limits.minStorageBufferOffsetAlignment =
	    supported.limits.minStorageBufferOffsetAlignment;

	required.limits.maxBindGroups = 2;

	required.limits.maxBufferSize               = 2'147'483'648;
	required.limits.maxStorageBufferBindingSize = 2'147'483'648;

	required.limits.maxBufferSize =
	    std::min(required.limits.maxBufferSize, supported.limits.maxBufferSize);
	required.limits.maxStorageBufferBindingSize =
	    std::min(required.limits.maxStorageBufferBindingSize,
	             supported.limits.maxStorageBufferBindingSize);

	required.limits.maxComputeWorkgroupSizeX          = 32;
	required.limits.maxComputeWorkgroupSizeY          = 4;
	required.limits.maxComputeWorkgroupSizeZ          = 1;
	required.limits.maxComputeInvocationsPerWorkgroup = 32;
	required.limits.maxComputeWorkgroupsPerDimension  = 31250;

	// required.limits.maxUniformBuffersPerShaderStage = 1;
	// TODO: required.limits.maxUniformBufferBindingSize     = sizeof(uniform_);

	required.limits.maxTextureDimension1D            = 4096;
	required.limits.maxTextureDimension2D            = 4096;
	required.limits.maxTextureDimension3D            = 1;
	required.limits.maxTextureArrayLayers            = 1;
	required.limits.maxSampledTexturesPerShaderStage = 1;
	required.limits.maxSamplersPerShaderStage        = 1;

	// // We use at most 2 vertex attributes
	// required.limits.maxVertexAttributes = 2;
	// // We should also tell that we use 1 vertex buffers
	// required.limits.maxVertexBuffers = 1;
	// // Maximum size of a buffer is 6 vertices of 5 float each
	// // Maximum stride between 2 consecutive vertices in the vertex buffer
	// required.limits.maxVertexBufferArrayStride = 5 * sizeof(float);

	// // There is a maximum of 3 float forwarded from vertex to fragment shader
	// required.limits.maxInterStageShaderComponents = 3;

	// // We use at most 1 bind group for now
	// required.limits.maxBindGroups = 1;
	// // We use at most 1 uniform buffer per stage
	// required.limits.maxUniformBuffersPerShaderStage = 1;
	// // Uniform structs have a size of maximum 16 float (more than what we need)
	// required.limits.maxUniformBufferBindingSize = 16 * 4;

	return required;
}

void Viz::onMouseMove(double x_pos, double y_pos)
{
	// TODO: Implement
}

void Viz::onMouseButton(int button, int action, int modifiers)
{
	// TODO: Implement
}

void Viz::onScroll(double x_offset, double y_offset)
{
	// TODO: Implement
}

void Viz::onKey(int key, int scancode, int action, int mods)
{
	// TODO: Implement
}
}  // namespace ufo