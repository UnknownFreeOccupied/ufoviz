// UFO
#include <ufo/viz/viz.hpp>

// Catch2
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ufo;

TEST_CASE("Viz")
{
	Viz v("UFOViz", true, false);

	while (v.running()) {
		v.onFrame();
		// using namespace std::chrono_literals;
		// std::this_thread::sleep_for(100ms);
	}
}