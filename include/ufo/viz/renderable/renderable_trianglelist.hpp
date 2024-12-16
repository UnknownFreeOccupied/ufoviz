
#ifndef UFO_VIZ_RENDERABLE_TRIANGLELIST_HPP
#define UFO_VIZ_RENDERABLE_TRIANGLELIST_HPP

// UFO
#include <ufo/compute/compute.hpp>
#include <ufo/vision/color.hpp>
#include <ufo/viz/renderable.hpp>
#include <ufo/viz/renderable/triangulate.hpp>

namespace ufo
{
class RenderableTrianglelist
{
 public:
	RenderableTrianglelist(ufo::Color color) : color_(color)
	{
		// TODO or should they all come with and load their own shader?
	}

	~RenderableTrianglelist() { release(); }

	void init(WGPUDevice device)
	{
		initBuffers(device);
		initBindGroups(device);
		initPipeline(device);
	}

	void release()
	{
		// TODO: Implement
	}

	void update(WGPUDevice device, WGPUCommandEncoder encoder,
	            WGPUTextureView render_texture, WGPUTextureView depth_texture,
	            Camera const& camera)
	{
		// Camera
		uniform_.projection = camera.projectionPerspective();
		uniform_.view       = ufo::Mat4x4f(camera.pose);
		uniform_.color      = color_;
		wgpuQueueWriteBuffer(compute::queue(device), uniform_buffer_, 0, &uniform_,
		                     sizeof(uniform_));

		// Render
		auto render_pass_desc = renderPassDesc(render_texture, depth_texture);
		WGPURenderPassEncoder render_pass =
		    wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);

		// Select which render pipeline to use
		wgpuRenderPassEncoderSetPipeline(render_pass, pipeline_);

		// Set binding group here!
		wgpuRenderPassEncoderSetBindGroup(render_pass, 0, bind_group_, 0, nullptr);

		// Bind the vertex buffer
		wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer_, 0,
		                                     WGPU_WHOLE_SIZE);

		// Bind the index buffer
		wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer_,
		                                    WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);

		wgpuRenderPassEncoderDrawIndexed(render_pass, triangles_.indices.size(), 1, 0, 0, 0);

		wgpuRenderPassEncoderEnd(render_pass);
		wgpuRenderPassEncoderRelease(render_pass);

		// TODO Not sure if this should be here or outside?
		// Encode and submit the render pass
		// 		WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
		// 		cmdBufferDescriptor.nextInChain                 = nullptr;
		// 		cmdBufferDescriptor.label                       = "Command buffer";
		// 		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder,
		// &cmdBufferDescriptor); 		wgpuCommandEncoderRelease(encoder);

		// 		// std::cout << "Submitting command..." << std::endl;
		// 		wgpuQueueSubmit(Compute::queue(), 1, &command);
		// 		wgpuCommandBufferRelease(command);
		// 		// std::cout << "Command submitted." << std::endl;

		// 		wgpuTextureViewRelease(render_texture);

		// #ifndef __EMSCRIPTEN__
		// 		wgpuSurfacePresent(Compute::surface());
		// #endif

		// #if defined(WEBGPU_BACKEND_DAWN)
		// 		wgpuDeviceTick(device);
		// #elif defined(WEBGPU_BACKEND_WGPU)
		// 		wgpuDevicePoll(device, false, nullptr);
		// #endif
	}

 protected:
	virtual WGPUBindGroupLayout bindGroupLayout(WGPUDevice device)
	{
		std::array<WGPUBindGroupLayoutEntry, 1> binding_layout{};

		// Uniform
		compute::setDefault(binding_layout[0]);
		binding_layout[0].binding     = 0;
		binding_layout[0].visibility  = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
		binding_layout[0].buffer.type = WGPUBufferBindingType_Uniform;
		binding_layout[0].buffer.minBindingSize = sizeof(uniform_);

		// Create a bind group layout
		bind_group_layout_desc             = {};
		bind_group_layout_desc.label       = "Compute Bind Group Layout";
		bind_group_layout_desc.nextInChain = nullptr;
		bind_group_layout_desc.entryCount  = binding_layout.size();
		bind_group_layout_desc.entries     = binding_layout.data();

		return wgpuDeviceCreateBindGroupLayout(device, &bind_group_layout_desc);
	}

	virtual void initBindGroups(WGPUDevice device)
	{
		std::array<WGPUBindGroupEntry, 1> binding{};

		// Uniforms
		binding[0].nextInChain = nullptr;
		binding[0].binding     = 0;
		binding[0].buffer      = uniform_buffer_;
		binding[0].offset      = 0;
		binding[0].size        = sizeof(uniform_);

		// A bind group contains one or multiple bindings
		bind_group_desc             = {};
		bind_group_desc.nextInChain = nullptr;
		bind_group_layout_          = bindGroupLayout(device);
		bind_group_desc.layout      = bind_group_layout_;
		// There must be as many bindings as declared in the layout!
		bind_group_desc.entryCount = binding.size();
		bind_group_desc.entries    = binding.data();

		bind_group_ = wgpuDeviceCreateBindGroup(device, &bind_group_desc);
	}

	virtual WGPURenderPassColorAttachment renderPassColorAttachment(
	    WGPUTextureView render_texture)
	{
		WGPURenderPassColorAttachment color_attachment = {};

		color_attachment.view          = render_texture;
		color_attachment.resolveTarget = nullptr;
		color_attachment.loadOp        = WGPULoadOp_Load;
		color_attachment.storeOp       = WGPUStoreOp_Store;
		color_attachment.clearValue    = WGPUColor{0.5, 0.5, 0.5, 1.0};

		return color_attachment;
	}

	virtual WGPURenderPassDepthStencilAttachment depthAttachment(
	    WGPUTextureView depth_texture)
	{
		WGPURenderPassDepthStencilAttachment depth_attachment = {};

		depth_attachment.view            = depth_texture;
		depth_attachment.depthClearValue = 1.0;
		depth_attachment.depthLoadOp     = WGPULoadOp_Load;
		depth_attachment.depthStoreOp    = WGPUStoreOp_Store;

		depth_attachment.depthReadOnly = false;
		// Stencil setup, mandatory but unused
		depth_attachment.stencilClearValue = 0.0;
		depth_attachment.stencilLoadOp     = WGPULoadOp_Clear;
		depth_attachment.stencilStoreOp    = WGPUStoreOp_Store;
		depth_attachment.stencilReadOnly   = true;

		return depth_attachment;
	}

	virtual WGPURenderPassDescriptor renderPassDesc(WGPUTextureView render_texture,
	                                                WGPUTextureView depth_texture)
	{
		WGPURenderPassDescriptor render_pass_desc = {};

		render_pass_desc.nextInChain            = nullptr;
		render_pass_desc.depthStencilAttachment = nullptr;
		render_pass_desc.colorAttachmentCount   = 1;
		color_attachment                        = renderPassColorAttachment(render_texture);
		render_pass_desc.colorAttachments       = &color_attachment;
		render_pass_desc.timestampWrites        = nullptr;
		depth_attachment                        = depthAttachment(depth_texture);
		render_pass_desc.depthStencilAttachment = &depth_attachment;

		return render_pass_desc;
	}

	virtual WGPUPipelineLayout pipelineLayout(WGPUDevice device)
	{
		pipeline_layout_desc = {};

		pipeline_layout_desc.nextInChain          = nullptr;
		pipeline_layout_desc.bindGroupLayoutCount = 1;
		pipeline_layout_desc.bindGroupLayouts     = &bind_group_layout_;

		return wgpuDeviceCreatePipelineLayout(device, &pipeline_layout_desc);
	}

	virtual WGPURenderPipelineDescriptor pipelineDesc(WGPUDevice device)
	{
		WGPURenderPipelineDescriptor pipeline_desc;
		pipeline_desc.layout = pipelineLayout(device);
		// The face orientation is defined by assuming that when looking
		// from the front of the face, its corner vertices are enumerated
		// in the counter-clockwise (CCW) order.
		pipeline_desc.primitive.frontFace = WGPUFrontFace_CCW;

		// But the face orientation does not matter much because we do not
		// cull (i.e. "hide") the faces pointing away from us (which is often
		// used for optimization).
		pipeline_desc.primitive.cullMode = WGPUCullMode_None;
		// Samples per pixel
		pipeline_desc.multisample.count = 1;

		// Default value for the mask, meaning "all bits on"
		pipeline_desc.multisample.mask = ~0u;

		// Default value as well (irrelevant for count = 1 anyways)
		pipeline_desc.multisample.alphaToCoverageEnabled = false;

		// shader
		depth_stencil_state                      = {};
		depth_stencil_state.format               = WGPUTextureFormat_Depth32Float;
		depth_stencil_state.depthWriteEnabled    = true;
		depth_stencil_state.depthCompare         = WGPUCompareFunction_Less;
		depth_stencil_state.stencilFront.compare = WGPUCompareFunction_Always;
		depth_stencil_state.stencilBack.compare  = WGPUCompareFunction_Always;
		depth_stencil_state.stencilReadMask      = 0xFFFFFFFF;
		depth_stencil_state.stencilWriteMask     = 0xFFFFFFFF;

		pipeline_desc.depthStencil = &depth_stencil_state;

		// Set the vertex state
		WGPUVertexState vertexState = {};
		vertexState.module          = shader_module_;
		vertexState.entryPoint      = "vertMain";
		vertexState.bufferCount     = 1;
		vertexState.buffers = &vertex_buffer_layout;  // Attach the vertex buffer layout

		pipeline_desc.vertex = vertexState;

		// Set the fragment state
		color_target = {};
		color_target.format =
		    WGPUTextureFormat_RGBA8UnormSrgb;  // Match the swap chain format
		color_target.writeMask = WGPUColorWriteMask_All;

		fragment_state               = {};
		fragment_state.module        = shader_module_;
		fragment_state.entryPoint    = "fragMain";
		fragment_state.constantCount = 0;
		fragment_state.constants     = nullptr;
		fragment_state.targetCount   = 1;
		fragment_state.targets       = &color_target;

		pipeline_desc.fragment                           = &fragment_state;
		pipeline_desc.primitive.topology                 = WGPUPrimitiveTopology_TriangleList;
		pipeline_desc.multisample.count                  = 1;
		pipeline_desc.multisample.mask                   = ~0u;
		pipeline_desc.multisample.alphaToCoverageEnabled = false;

		return pipeline_desc;
	}

	virtual void initPipeline(WGPUDevice device)
	{
		// Needs:
		// - pipelineLayout
		//		- bind_group_layout_ (uniform)
		pipeline_desc = pipelineDesc(device);
		pipeline_     = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);
	}

	virtual void initBuffers(WGPUDevice device)
	{
		initVertexBuffer(device);
		initIndexBuffer(device);
		initUniformBuffer(device);
	}

	virtual void initVertexBuffer(WGPUDevice device)
	{
		vertex_buffer_desc       = {};
		vertex_buffer_desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
		vertex_buffer_desc.size  = triangles_.vertices.size() * sizeof(Vertex);
		vertex_buffer_desc.mappedAtCreation = true;

		vertex_buffer_ = wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);

		// Copy the vertex data into the buffer
		void* mapped_data =
		    wgpuBufferGetMappedRange(vertex_buffer_, 0, vertex_buffer_desc.size);
		memcpy(mapped_data, triangles_.vertices.data(), vertex_buffer_desc.size);
		wgpuBufferUnmap(vertex_buffer_);

		// Layout
		// Position attribute
		WGPUVertexAttribute position_attr = {};
		position_attr.shaderLocation      = 0;  // Matches @location(0) in the vertex shader
		position_attr.format              = WGPUVertexFormat_Float32x3;  // vec3<f32> in WGSL
		position_attr.offset              = offsetof(Vertex, pos);

		// Normal attribute
		WGPUVertexAttribute normal_attr = {};
		normal_attr.shaderLocation      = 1;  // Matches @location(1) in the vertex shader
		normal_attr.format              = WGPUVertexFormat_Float32x3;  // vec3<f32> in WGSL
		normal_attr.offset              = offsetof(Vertex, normal);

		vertex_buffer_layout                = {};
		vertex_buffer_layout.arrayStride    = sizeof(Vertex);
		vertex_buffer_layout.attributeCount = 2;
		vertex_attributes                   = {position_attr, normal_attr};
		vertex_buffer_layout.attributes     = vertex_attributes.data();
	}

	virtual void initIndexBuffer(WGPUDevice device)
	{
		index_buffer_desc                  = {};
		index_buffer_desc.usage            = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
		index_buffer_desc.size             = triangles_.indices.size() * sizeof(uint32_t);
		index_buffer_desc.mappedAtCreation = true;

		index_buffer_ = wgpuDeviceCreateBuffer(device, &index_buffer_desc);

		// Copy the index data into the buffer
		void* mapped_data =
		    wgpuBufferGetMappedRange(index_buffer_, 0, index_buffer_desc.size);
		memcpy(mapped_data, triangles_.indices.data(), index_buffer_desc.size);
		wgpuBufferUnmap(index_buffer_);
	}

	virtual void initUniformBuffer(WGPUDevice device)
	{
		uniform_buffer_desc                  = {};
		uniform_buffer_desc.mappedAtCreation = false;
		uniform_buffer_desc.size             = sizeof(uniform_);
		uniform_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
		uniform_buffer_desc.mappedAtCreation = false;
		uniform_buffer_ = wgpuDeviceCreateBuffer(device, &uniform_buffer_desc);
	}

 protected:
	struct alignas(16) UBO {
		ufo::Mat4x4f projection;
		ufo::Mat4x4f view;
		ufo::Color   color;
	} uniform_;
	// Have the compiler check byte alignment
	static_assert(sizeof(UBO) % 16 == 0);

	ufo::Color color_;

	WGPURenderPipeline           pipeline_     = nullptr;
	WGPURenderPipelineDescriptor pipeline_desc = {};

	// TODO release the buffers somewhere
	WGPUBuffer           vertex_buffer_     = nullptr;
	WGPUBufferDescriptor vertex_buffer_desc = {};
	WGPUBuffer           index_buffer_      = nullptr;
	WGPUBufferDescriptor index_buffer_desc{};
	WGPUBuffer           uniform_buffer_ = nullptr;
	WGPUBufferDescriptor uniform_buffer_desc{};

	WGPUBindGroupLayout bind_group_layout_ = nullptr;
	WGPUBindGroup       bind_group_        = nullptr;

	// TODO deal with these
	WGPURenderPassColorAttachment        color_attachment;
	WGPURenderPassDepthStencilAttachment depth_attachment;
	WGPUDepthStencilState                depth_stencil_state;
	WGPUVertexBufferLayout               vertex_buffer_layout;
	WGPUFragmentState                    fragment_state{};
	std::vector<WGPUVertexAttribute>     vertex_attributes;
	WGPUColorTargetState                 color_target;
	WGPUBindGroupLayoutDescriptor        bind_group_layout_desc{};
	WGPUBindGroupDescriptor              bind_group_desc{};
	WGPUPipelineLayoutDescriptor         pipeline_layout_desc = {};

 public:
	WGPUShaderModule     shader_module_ = nullptr;
	TriangleList<Vertex> triangles_;
};

}  // namespace ufo

#endif  // UFO_VIZ_RENDERABLE_PATH_HPP