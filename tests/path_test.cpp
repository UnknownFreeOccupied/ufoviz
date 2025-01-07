
// UFO
#include <ufo/compute/compute.hpp>
#include <ufo/plan/graph.hpp>
#include <ufo/plan/path.hpp>
#include <ufo/vision/color.hpp>
#include <ufo/viz/renderable/path.hpp>
#include <ufo/viz/viz.hpp>

// Catch2
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ufo;

PlanPath<3, float> makeTestPlan()
{
	Vec3f a(0, 0, 0);
	Vec3f b(1, 0, 0);
	Vec3f c(1, 2, 2);
	Vec3f d(3, 3, 3);
	Vec3f e(2, 4, 2);

	PlanGraph<3, float> plan_graph_;

	auto a_node = plan_graph_.addNode(a, 1);
	auto b_node = plan_graph_.addNode(b, 1);
	auto c_node = plan_graph_.addNode(c, 1);
	auto d_node = plan_graph_.addNode(d, 1);
	auto e_node = plan_graph_.addNode(e, 1);

	PlanPath<3, float> plan;
	plan.push_back(a_node);
	plan.push_back(b_node);
	plan.push_back(c_node);
	plan.push_back(d_node);
	plan.push_back(e_node);

	return plan;
}

TEST_CASE("[Viz] Path")
{
	Viz v("UFOViz");

	auto path = makeTestPlan();
	v.addRenderable(RenderablePath(path, Color(255, 0, 0), 0.2f, 16));

	v.run();
}