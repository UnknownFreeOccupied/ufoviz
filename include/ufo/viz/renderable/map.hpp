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

#ifndef UFO_VIZ_RENDERABLE_MAP_HPP
#define UFO_VIZ_RENDERABLE_MAP_HPP

// UFO
#include <ufo/compute/compute.hpp>
#include <ufo/map/tree/map.hpp>
#include <ufo/utility/type_traits.hpp>
#include <ufo/vision/camera.hpp>
#include <ufo/viz/renderable.hpp>

// STL
#include <cstdlib>
#include <iostream>

namespace ufo
{
template <class Map>
class RenderableMap : public Renderable
{
 private:
	struct Uniform {
		Mat4x4f   projection;
		Mat4x4f   view;
		Vec2u     dim;
		float     near_clip;
		float     far_clip;
		TreeIndex node;
		float     _pad0[2];
		Vec3f     node_center;
		float     node_half_length;

		bool operator==(Uniform const& rhs) const
		{
			return projection == rhs.projection && view == rhs.view && dim == rhs.dim &&
			       near_clip == rhs.near_clip && far_clip == rhs.far_clip && node == rhs.node &&
			       node_center == rhs.node_center && node_half_length == rhs.node_half_length;
		}

		bool operator!=(Uniform const& rhs) const { return !(*this == rhs); }
	};
	// Have the compiler check byte alignment
	static_assert(sizeof(Uniform) % 16 == 0);

 public:
	RenderableMap(Map const& map)  // : map_(map)
	{
		// TODO: Implement
	}

	~RenderableMap() override = default;

	void init(WGPUDevice device, WGPUTextureFormat texture_format) override
	{
		compute_bind_group_layout_ = createBindGroupLayout(device, texture_format);
		compute_pipeline_layout_   = createPipelineLayout(device, compute_bind_group_layout_);
		compute_pipeline_          = createComputePipeline(device, compute_pipeline_layout_);

		map_.gpuInit(device);

		uniform_buffer_ =
		    compute::createBuffer(map_.gpuDevice(), sizeof(uniform_),
		                          WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform, false);
	}

	void release() override
	{
		if (nullptr != compute_bind_group_) {
			wgpuBindGroupRelease(compute_bind_group_);
			compute_bind_group_ = nullptr;
		}
		if (nullptr != compute_pipeline_layout_) {
			wgpuPipelineLayoutRelease(compute_pipeline_layout_);
			compute_pipeline_layout_ = nullptr;
		}
		if (nullptr != compute_bind_group_layout_) {
			wgpuBindGroupLayoutRelease(compute_bind_group_layout_);
			compute_bind_group_layout_ = nullptr;
		}

		// TODO: Buffers

		if (nullptr != compute_pipeline_) {
			wgpuComputePipelineRelease(compute_pipeline_);
			compute_pipeline_ = nullptr;
		}
	}

	void update(WGPUDevice device, WGPUCommandEncoder encoder,
	            WGPUTextureView render_texture, WGPUTextureView depth_texture,
	            Camera const& camera) override
	{
		Uniform uniform = createUniform(map_, camera);

		if (uniform_ != uniform) {
			map_.gpuUpdateBuffers();

			uniform_ = uniform;
			hits_.resize(uniform.dim.x * uniform.dim.y);

			hits_buffer_ =
			    compute::createBuffer(map_.gpuDevice(), hits_.size() * sizeof(TreeIndex),
			                          WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, false);

			if (nullptr != compute_bind_group_) {
				wgpuBindGroupRelease(compute_bind_group_);
			}

			compute_bind_group_ = createBindGroup(device);
		}

		// TODO: Implement
	}

	void onGui() override
	{
		// TODO: Implement
	}

 private:
	[[nodiscard]] WGPUBindGroupLayout createBindGroupLayout(
	    WGPUDevice device, WGPUTextureFormat /* texture_format */)
	{
		std::array<WGPUBindGroupLayoutEntry, 4> binding_layout{};

		// Output texture
		compute::setDefault(binding_layout[0]);
		binding_layout[0].binding     = 0;
		binding_layout[0].visibility  = WGPUShaderStage_Compute;
		binding_layout[0].buffer.type = WGPUBufferBindingType_Storage;

		// Uniform
		compute::setDefault(binding_layout[1]);
		binding_layout[1].binding               = 1;
		binding_layout[1].visibility            = WGPUShaderStage_Compute;
		binding_layout[1].buffer.type           = WGPUBufferBindingType_Uniform;
		binding_layout[1].buffer.minBindingSize = sizeof(uniform_);

		// Tree buffer
		compute::setDefault(binding_layout[2]);
		binding_layout[2].binding     = 2;
		binding_layout[2].visibility  = WGPUShaderStage_Compute;
		binding_layout[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;

		// Occupancy buffer
		compute::setDefault(binding_layout[3]);
		binding_layout[3].binding     = 3;
		binding_layout[3].visibility  = WGPUShaderStage_Compute;
		binding_layout[3].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;

		// Create a bind group layout
		WGPUBindGroupLayoutDescriptor bind_group_layout_desc{};
		bind_group_layout_desc.label       = "";
		bind_group_layout_desc.nextInChain = nullptr;
		bind_group_layout_desc.entryCount  = binding_layout.size();
		bind_group_layout_desc.entries     = binding_layout.data();

		return wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);
	}

	[[nodiscard]] WGPUPipelineLayout createPipelineLayout(
	    WGPUDevice device, WGPUBindGroupLayout bind_group_layout)
	{
		WGPUPipelineLayoutDescriptor desc{};
		desc.nextInChain          = nullptr;
		desc.bindGroupLayoutCount = 1;
		desc.bindGroupLayouts     = &bind_group_layout;
		return wgpuDeviceCreatePipelineLayout(device, &desc);
	}

	[[nodiscard]] WGPUComputePipeline createComputePipeline(
	    WGPUDevice device, WGPUPipelineLayout pipeline_layout)
	{
		WGPUShaderModule shader_module;
		if constexpr (2 == Map::dimensions()) {
			shader_module =
			    compute::loadShaderModule(device, UFOVIZ_SHADER_DIR "/map_ray_trace_2d.wgsl");
		} else if constexpr (3 == Map::dimensions()) {
			shader_module =
			    compute::loadShaderModule(device, UFOVIZ_SHADER_DIR "/map_ray_trace_3d.wgsl");
		} else if constexpr (4 == Map::dimensions()) {
			shader_module =
			    compute::loadShaderModule(device, UFOVIZ_SHADER_DIR "/map_ray_trace_4d.wgsl");
		} else {
			static_assert(dependent_false_v<Map>, "Non-supported number of dimensions");
		}

		if (nullptr == shader_module) {
			std::cerr << "Could not load shader!" << std::endl;
			abort();
		}

		WGPUComputePipelineDescriptor desc{};
		desc.nextInChain           = nullptr;
		desc.compute.constantCount = 0;  // TODO: Change
		desc.compute.constants     = nullptr;
		desc.compute.entryPoint    = "main";
		desc.compute.module        = shader_module;
		desc.layout                = pipeline_layout;

		WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(device, &desc);

		wgpuShaderModuleRelease(shader_module);

		return pipeline;
	}

	[[nodiscard]] WGPUBindGroup createBindGroup(WGPUDevice device)
	{
		// Create a binding
		std::array<WGPUBindGroupEntry, 4> binding{};

		// Hits buffer
		binding[0].nextInChain = nullptr;
		binding[0].binding     = 0;
		binding[0].buffer      = hits_buffer_;
		binding[0].offset      = 0;
		binding[0].size        = hits_.size() * sizeof(TreeIndex);

		// Uniform
		binding[1].nextInChain = nullptr;
		binding[1].binding     = 1;
		binding[1].buffer      = uniform_buffer_;
		binding[1].offset      = 0;
		binding[1].size        = sizeof(uniform_);

		// Tree
		binding[2].nextInChain = nullptr;
		binding[2].binding     = 2;
		binding[2].buffer      = map_.gpuTreeBuffer();
		binding[2].offset      = 0;
		binding[2].size        = map_.gpuTreeBufferSize();

		// Occupancy
		binding[3].nextInChain = nullptr;
		binding[3].binding     = 3;
		binding[3].buffer      = map_.gpuOccupancyBuffer();
		binding[3].offset      = 0;
		binding[3].size        = map_.gpuOccupancyBufferSize();

		// A bind group contains one or multiple bindings
		WGPUBindGroupDescriptor desc{};
		desc.nextInChain = nullptr;
		desc.layout      = compute_bind_group_layout_;
		desc.entryCount  = binding.size();
		desc.entries     = binding.data();
		return wgpuDeviceCreateBindGroup(device, &desc);
	}

	[[nodiscard]] static Uniform createUniform(Map const& map, Camera const& camera)
	{
		Uniform uniform;
		uniform.projection       = inverse(camera.projectionPerspective());
		uniform.view             = inverse(Mat4x4f(camera.pose));
		uniform.dim              = Vec2u(camera.cols, camera.rows);
		uniform.near_clip        = camera.near_clip;
		uniform.far_clip         = camera.far_clip;
		uniform.node             = map.index();
		uniform.node_center      = map.center(uniform.node);
		uniform.node_half_length = map.halfLength(uniform.node);
		return uniform;
	}

 private:
	Map map_;

	WGPUBindGroupLayout compute_bind_group_layout_ = nullptr;
	WGPUPipelineLayout  compute_pipeline_layout_   = nullptr;
	WGPUComputePipeline compute_pipeline_          = nullptr;
	WGPUBindGroup       compute_bind_group_        = nullptr;

	WGPUBuffer hits_buffer_    = nullptr;
	WGPUBuffer uniform_buffer_ = nullptr;

	Uniform uniform_;

	std::vector<TreeIndex> hits_;
};
}  // namespace ufo

#endif  // UFO_VIZ_RENDERABLE_MAP_HPP