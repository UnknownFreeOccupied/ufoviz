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

#ifndef UFO_VIZ_VIZ_HPP
#define UFO_VIZ_VIZ_HPP

// UFO
#include <ufo/compute/compute.hpp>
#include <ufo/vision/camera.hpp>
#include <ufo/viz/renderable.hpp>

// STL
#include <mutex>
#include <vector>
#include <string>
#include <thread>

// EMSCRIPTEN
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif  // __EMSCRIPTEN__

// ImGUI
// #include <imgui.h>

// Forward declare
struct GLFWwindow;

namespace ufo
{
class Viz
{
 public:
	Viz(std::string const&  window_name      = "UFOViz",
	    WGPUPowerPreference power_preference = WGPUPowerPreference_Undefined,
	    WGPUBackendType     backend_type     = WGPUBackendType_Undefined);

	~Viz();

	void start(WGPUPowerPreference power_preference = WGPUPowerPreference_Undefined,
	           WGPUBackendType     backend_type     = WGPUBackendType_Undefined);

	void stop();

	void run();

	void runAsync();

	[[nodiscard]] bool running() const;

	void update();

	[[nodiscard]] WGPUInstance instance() const;

	[[nodiscard]] WGPUAdapter adapter() const;

	[[nodiscard]] WGPUDevice device() const;

	void addRenderable(Renderable const& renderable);

	// void eraseRenderable(std::shared_ptr<Renderable> const& renderable);

	void clearRenderable();

	void loadConfig();

	void saveConfig() const;

 private:
	void init(WGPUPowerPreference power_preference, WGPUBackendType backend_type);

	[[nodiscard]] GLFWwindow* createWindow() const;

	[[nodiscard]] WGPUSurfaceCapabilities surfaceCapabilities(WGPUSurface surface,
	                                                          WGPUAdapter adapter) const;

	[[nodiscard]] WGPUSurfaceConfiguration surfaceConfiguration(
	    GLFWwindow* window, WGPUDevice device, WGPUSurfaceCapabilities capabilities) const;

	[[nodiscard]] WGPURequiredLimits requiredLimits(WGPUAdapter adapter) const;

	void initGui();

	void updateGui(WGPURenderPassEncoder render_pass);

	// A function called when the window is resized.
	// void onResize(int width, int height);

	// Party events
	void onMouseMove(double x_pos, double y_pos);

	void onMouseButton(int button, int action, int modifiers);

	void onScroll(double x_offset, double y_offset);

	void onKey(int key, int scancode, int action, int mods);

 private:
	GLFWwindow* window_ = nullptr;

	std::string window_name_ = "UFOViz";

	WGPUInstance instance_ = nullptr;
	WGPUSurface  surface_  = nullptr;
	WGPUAdapter  adapter_  = nullptr;
	WGPUDevice   device_   = nullptr;
	WGPUQueue    queue_    = nullptr;

	WGPUSurfaceCapabilities  surface_capa_;
	WGPUSurfaceConfiguration surface_config_;

	std::thread render_thread_;

	float prev_time_{};

	std::mutex                            renderables_mutex_;
	std::vector<std::unique_ptr<Renderable>> renderables_;

	Camera     camera_;
	ufo::Vec2f angles_{0.0f, 0.0f};
	ufo::Vec3f center_{4.4f, 0.0f, 1.7f};
	float      zoom_ = 0.0f;
};
}  // namespace ufo

#endif  // UFO_VIZ_VIZ_HPP