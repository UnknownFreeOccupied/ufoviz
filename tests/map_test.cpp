
// UFO
#include <ufo/map/color/map.hpp>
#include <ufo/map/occupancy/map.hpp>
#include <ufo/map/ufomap.hpp>
#include <ufo/viz/renderable/map.hpp>
#include <ufo/viz/viz.hpp>

// Catch2
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ufo;

TEST_CASE("[Viz] Map")
{
	Viz v("UFOViz");

	Map3D<OccupancyMap, ColorMap> map;

	v.addRenderable(RenderableMap(map));

	v.run();
}