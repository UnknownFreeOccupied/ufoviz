// UFO
#include <ufo/viz/viz.hpp>

// Catch2
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ufo;

TEST_CASE("Viz")
{
	using namespace std::chrono_literals;

	Viz v("UFOViz");

	v.run();
}