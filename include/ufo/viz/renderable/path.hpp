
#ifndef UFO_VIZ_RENDERABLE_PATH_HPP
#define UFO_VIZ_RENDERABLE_PATH_HPP

// UFO
#include <ufo/compute/compute.hpp>
#include <ufo/plan/graph.hpp>
#include <ufo/viz/renderable.hpp>
#include <ufo/viz/renderable/renderable_trianglelist.hpp>
#include <ufo/viz/renderable/triangulate.hpp>

// STL
#include <cstdio>

namespace ufo
{
class RenderablePath : public Renderable
{
 public:
	RenderablePath(ufo::PlanPath<3, float> const& plan, ufo::Color color, float radius,
	               int segments, bool use_arrow = true, float arrow_head_height = 0.5f)
	    : triangle_list_(color), plan_(plan)
	{
		// TODO or should they all come with and load their own shader?

		triangle_list_.triangles_ =
		    triangulate<Vertex>(plan_, radius, segments, use_arrow, arrow_head_height);
	}

	~RenderablePath() override { release(); }

	void init(WGPUDevice device) override
	{
		triangle_list_.shader_module_ =
		    compute::loadShaderModule(device, UFOVIZ_SHADER_DIR "/render.wgsl");
		if (nullptr == triangle_list_.shader_module_) {
			std::printf("Could not load shader!\n");
			exit(1);
		}
		triangle_list_.init(device);
	}

	void release() override
	{
		// TODO: Implement
	}

	void update(WGPUDevice device, WGPUCommandEncoder encoder,
	            WGPUTextureView render_texture, WGPUTextureView depth_texture,
	            Camera const& camera) override
	{
		triangle_list_.update(device, encoder, render_texture, depth_texture, camera);
	}

	void onGui() override
	{
		// TODO: Implement
	}

 private:
	RenderableTrianglelist  triangle_list_;
	ufo::PlanPath<3, float> plan_;
};
}  // namespace ufo

#endif  // UFO_VIZ_RENDERABLE_PATH_HPP