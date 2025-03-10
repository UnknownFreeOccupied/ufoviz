
#ifndef UFO_VIZ_TRIANGULATE_HPP
#define UFO_VIZ_TRIANGULATE_HPP

// STL
#include <cstdint>
#include <vector>

// UFO
#include <ufo/geometry/cone.hpp>
#include <ufo/geometry/cylinder.hpp>
#include <ufo/geometry/line_segment.hpp>
#include <ufo/geometry/sphere.hpp>
#include <ufo/math/vec3.hpp>
#include <ufo/plan/graph.hpp>
#include <ufo/plan/path.hpp>

struct Vertex {
	Vertex(float x, float y, float z, float nx, float ny, float nz)
	    : pos{x, y, z}, _pad0(0.0f), normal{nx, ny, nz}, _pad1(0.0f)
	{
	}
	Vertex(ufo::Vec3f pos, ufo::Vec3f normal)
	    : pos(pos), _pad0(0.0f), normal(normal), _pad1(0.0f)
	{
	}

	ufo::Vec3f pos;
	float      _pad0;
	ufo::Vec3f normal;
	float      _pad1;
};

template <class Vertex>
struct TriangleList {
	std::vector<Vertex>        vertices;
	std::vector<std::uint32_t> indices;
};

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::BS3 const& sphere, int num_slices,
                                        int num_stacks);

template <>
inline TriangleList<Vertex> triangulate(ufo::BS3 const& sphere, int num_slices,
                                        int num_stacks)
{
	TriangleList<Vertex> result;

	// Vertices
	for (int i = 0; i <= num_stacks; ++i) {
		float phi = M_PI * i / num_stacks;
		for (int j = 0; j <= num_slices; ++j) {
			float theta = 2 * M_PI * j / num_slices;

			float x = sphere.radius * std::sin(phi) * std::cos(theta) + sphere.center.x;
			float y = sphere.radius * std::sin(phi) * std::sin(theta) + sphere.center.y;
			float z = sphere.radius * std::cos(phi) + sphere.center.z;

			float nx = (x - sphere.center.x) / sphere.radius;
			float ny = (y - sphere.center.y) / sphere.radius;
			float nz = (z - sphere.center.z) / sphere.radius;

			result.vertices.push_back({x, y, z, nx, ny, nz});
		}
	}

	// Indices
	for (int i = 0; i < num_stacks; ++i) {
		for (int j = 0; j < num_slices; ++j) {
			int first  = i * (num_slices + 1) + j;
			int second = first + num_slices + 1;

			result.indices.push_back(first);
			result.indices.push_back(second);
			result.indices.push_back(first + 1);

			result.indices.push_back(second);
			result.indices.push_back(second + 1);
			result.indices.push_back(first + 1);
		}
	}

	return result;
}

template <>
inline TriangleList<ufo::Vec3f> triangulate(ufo::BS3 const& sphere, int num_slices,
                                            int num_stacks)
{
	TriangleList<ufo::Vec3f> result;

	// Vertices
	for (int i = 0; i <= num_stacks; ++i) {
		float phi = M_PI * i / num_stacks;
		for (int j = 0; j <= num_slices; ++j) {
			float theta = 2 * M_PI * j / num_slices;

			float x = sphere.radius * std::sin(phi) * std::cos(theta) + sphere.center.x;
			float y = sphere.radius * std::sin(phi) * std::sin(theta) + sphere.center.y;
			float z = sphere.radius * std::cos(phi) + sphere.center.z;

			result.vertices.push_back({x, y, z});
		}
	}

	// Indices
	for (int i = 0; i < num_stacks; ++i) {
		for (int j = 0; j < num_slices; ++j) {
			int first  = i * (num_slices + 1) + j;
			int second = first + num_slices + 1;

			result.indices.push_back(first);
			result.indices.push_back(second);
			result.indices.push_back(first + 1);

			result.indices.push_back(second);
			result.indices.push_back(second + 1);
			result.indices.push_back(first + 1);
		}
	}

	return result;
}

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::Cylinder3 const& cylinder, int segments);

template <>
inline TriangleList<Vertex> triangulate(ufo::Cylinder3 const& cylinder, int segments)
{
	TriangleList<Vertex> result;

	// Vertices
	ufo::Vec3f direction = cylinder.center_2 - cylinder.center_1;
	ufo::Vec3f axis      = normalize(direction);

	// Create a local coordinate system
	ufo::Vec3f up(0.0f, 1.0f, 0.0f);
	if (std::abs(dot(axis, up)) > 0.99f) {
		up = ufo::Vec3f(1.0f, 0.0f, 0.0f);
	}

	ufo::Vec3f right   = normalize(cross(up, axis));
	ufo::Vec3f forward = normalize(cross(axis, right));

	result.vertices.push_back({cylinder.center_1, -direction});
	result.vertices.push_back({cylinder.center_2, direction});

	float angle_step = 2 * M_PI / segments;

	// Generate vertices for the top and bottom circles
	for (int i = 0; i <= segments; ++i) {
		float angle = i * angle_step;
		float x     = cylinder.radius * std::cos(angle);
		float y     = cylinder.radius * std::sin(angle);

		ufo::Vec3f p = x * right + y * forward;

		// Top circle vertex
		ufo::Vec3f top_vertex = cylinder.center_1 + p;
		result.vertices.push_back({top_vertex, p / cylinder.radius});

		// Bottom circle vertex
		ufo::Vec3f bottom_vertex = cylinder.center_2 + p;
		result.vertices.push_back({bottom_vertex, p / cylinder.radius});
	}

	// Indices
	// cap indices
	for (int i = 0; i <= segments; ++i) {
		result.indices.push_back(0);  // Center vertex of the top cap
		result.indices.push_back(2 * i);
		result.indices.push_back(2 * i + 2);

		result.indices.push_back(1);  // Center vertex of the bottom cap
		result.indices.push_back(2 * i + 1);
		result.indices.push_back(2 * i + 3);
	}

	// Side surface indices
	for (int i = 0; i < segments; ++i) {
		int top_index    = 2 * i + 2;
		int bottom_index = top_index + 1;

		result.indices.push_back(top_index);
		result.indices.push_back(bottom_index);
		result.indices.push_back(top_index + 2);

		result.indices.push_back(bottom_index);
		result.indices.push_back(bottom_index + 2);
		result.indices.push_back(top_index + 2);
	}

	return result;
}

template <>
inline TriangleList<ufo::Vec3f> triangulate(ufo::Cylinder3 const& cylinder, int segments)
{
	TriangleList<ufo::Vec3f> result;

	// Vertices
	ufo::Vec3f direction = cylinder.center_2 - cylinder.center_1;
	ufo::Vec3f axis      = normalize(direction);

	// Create a local coordinate system
	ufo::Vec3f up(0.0f, 1.0f, 0.0f);
	if (std::abs(dot(axis, up)) > 0.99f) {
		up = ufo::Vec3f(1.0f, 0.0f, 0.0f);
	}

	ufo::Vec3f right   = normalize(cross(up, axis));
	ufo::Vec3f forward = normalize(cross(axis, right));

	result.vertices.push_back(cylinder.center_1);
	result.vertices.push_back(cylinder.center_2);

	float angle_step = 2 * M_PI / segments;

	// Generate vertices for the top and bottom circles
	for (int i = 0; i <= segments; ++i) {
		float angle = i * angle_step;
		float x     = cylinder.radius * std::cos(angle);
		float y     = cylinder.radius * std::sin(angle);

		ufo::Vec3f p = x * right + y * forward;

		// Top circle vertex
		ufo::Vec3f top_vertex = cylinder.center_1 + p;
		result.vertices.push_back(top_vertex);

		// Bottom circle vertex
		ufo::Vec3f bottom_vertex = cylinder.center_2 + p;
		result.vertices.push_back(bottom_vertex);
	}

	// Indices
	// cap indices
	for (int i = 0; i <= segments; ++i) {
		result.indices.push_back(0);  // Center vertex of the top cap
		result.indices.push_back(2 * i);
		result.indices.push_back(2 * i + 2);

		result.indices.push_back(1);  // Center vertex of the bottom cap
		result.indices.push_back(2 * i + 1);
		result.indices.push_back(2 * i + 3);
	}

	// Side surface indices
	for (int i = 0; i < segments; ++i) {
		int top_index    = 2 * i + 2;
		int bottom_index = top_index + 1;

		result.indices.push_back(top_index);
		result.indices.push_back(bottom_index);
		result.indices.push_back(top_index + 2);

		result.indices.push_back(bottom_index);
		result.indices.push_back(bottom_index + 2);
		result.indices.push_back(top_index + 2);
	}

	return result;
}

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::Cone3 const& cone, int segments);

template <>
inline TriangleList<Vertex> triangulate(ufo::Cone3 const& cone, int segments)
{
	TriangleList<Vertex> result;

	auto dir = normalize(cone.tip - cone.base_center);

	result.vertices.push_back({cone.base_center, -dir});
	result.vertices.push_back({cone.tip, dir});

	// Create a local coordinate system (basis)
	ufo::Vec3f up(0.0f, 1.0f, 0.0f);
	if (std::abs(dot(dir, up)) > 0.99f) {
		up = ufo::Vec3f(1.0f, 0.0f, 0.0f);
	}

	ufo::Vec3f right   = normalize(cross(up, dir));
	ufo::Vec3f forward = normalize(cross(dir, right));

	// Base circle vertices
	float angle_step = 2 * M_PI / segments;

	// Generate vertices for base circle
	for (int i = 0; i <= segments; ++i) {
		float angle = i * angle_step;
		float x     = cone.radius * std::cos(angle);
		float y     = cone.radius * std::sin(angle);

		auto p = x * right + y * forward;
		result.vertices.push_back({cone.base_center + p, p / cone.radius});
	}

	// Generate indices for base circle triangles
	for (int i = 2; i <= segments + 1; ++i) {
		result.indices.push_back(0);  // Base center
		result.indices.push_back(i);
		result.indices.push_back(i + 1);
	}

	// Generate the indices for the side triangles
	for (int i = 2; i <= segments + 1; ++i) {
		result.indices.push_back(1);  // Tip index
		result.indices.push_back(i);
		result.indices.push_back(i + 1);
	}

	return result;
}

template <>
inline TriangleList<ufo::Vec3f> triangulate(ufo::Cone3 const& cone, int segments)
{
	TriangleList<ufo::Vec3f> result;

	auto dir = normalize(cone.tip - cone.base_center);

	result.vertices.push_back(cone.base_center);
	result.vertices.push_back(cone.tip);

	// Create a local coordinate system (basis)
	ufo::Vec3f up(0.0f, 1.0f, 0.0f);
	if (std::abs(dot(dir, up)) > 0.99f) {
		up = ufo::Vec3f(1.0f, 0.0f, 0.0f);
	}

	ufo::Vec3f right   = normalize(cross(up, dir));
	ufo::Vec3f forward = normalize(cross(dir, right));

	// Base circle vertices
	float angle_step = 2 * M_PI / segments;

	// Generate vertices for base circle
	for (int i = 0; i <= segments; ++i) {
		float angle = i * angle_step;
		float x     = cone.radius * std::cos(angle);
		float y     = cone.radius * std::sin(angle);

		auto p = x * right + y * forward;
		result.vertices.push_back(cone.base_center + p);
	}

	// Generate indices for base circle triangles
	for (int i = 2; i <= segments + 1; ++i) {
		result.indices.push_back(0);  // Base center
		result.indices.push_back(i);
		result.indices.push_back(i + 1);
	}

	// Generate the indices for the side triangles
	for (int i = 2; i <= segments + 1; ++i) {
		result.indices.push_back(1);  // Tip index
		result.indices.push_back(i);
		result.indices.push_back(i + 1);
	}

	return result;
}

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::LineSegment3 const& ls, int segments,
                                        float radius)
{
	ufo::Cylinder3 cylinder(ls.start, ls.end, radius);
	return triangulate<Vertex>(cylinder, segments);
}

template <class Vertex>
inline TriangleList<Vertex> triangulateArrow(ufo::Vec3f start, ufo::Vec3f tip_end,
                                             float radius, float tip_height,
                                             int num_slices)
{
	TriangleList<Vertex> result;

	auto dir = normalize(tip_end - start);
	auto b   = tip_end - dir * tip_height;

	auto cylinder = triangulate<Vertex>(ufo::Cylinder(start, b, radius), num_slices);
	auto cone     = triangulate<Vertex>(ufo::Cone(b, tip_end, 2 * radius), num_slices);
	for (std::size_t i{}; i < cone.indices.size(); ++i) {
		cone.indices[i] += cylinder.vertices.size();
	}

	result.vertices.insert(result.vertices.end(), cylinder.vertices.begin(),
	                       cylinder.vertices.end());
	result.indices.insert(result.indices.end(), cylinder.indices.begin(),
	                      cylinder.indices.end());

	result.vertices.insert(result.vertices.end(), cone.vertices.begin(),
	                       cone.vertices.end());
	result.indices.insert(result.indices.end(), cone.indices.begin(), cone.indices.end());

	return result;
}

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::PlanGraph<3, float> const& graph,
                                        float node_radius, float edge_radius,
                                        int segments, bool edge_as_arrow = false,
                                        float arrow_head_length = 0.0f)
{
	// TODO maybe separate Nodes and edges for easier coloring

	TriangleList<Vertex> result;
	for (auto [p, n] : graph) {
		auto center = static_cast<ufo::Vec3f>(*n);

		// make a sphere for each node
		auto sphere =
		    triangulate<Vertex>(ufo::BS3(center, node_radius), segments, segments / 2);
		for (std::size_t i{}; i < sphere.indices.size(); ++i) {
			sphere.indices[i] += result.vertices.size();
		}

		result.vertices.insert(result.vertices.end(), sphere.vertices.begin(),
		                       sphere.vertices.end());
		result.indices.insert(result.indices.end(), sphere.indices.begin(),
		                      sphere.indices.end());

		// make a cylinder for each edge
		// TODO handle bidirectional edges: don't add multiple cylinder per edge pair
		for (auto e : n->edges) {
			auto start = static_cast<ufo::Vec3f>(*(e.from));
			auto end   = static_cast<ufo::Vec3f>(*(e.to));
			auto t     = edge_as_arrow ? triangulateArrow<Vertex>(start, end, edge_radius,
			                                                      arrow_head_length, segments)
			                           : triangulate<Vertex>(
                                   ufo::Cylinder3(start, end, edge_radius), segments);
			for (std::size_t i{}; i < t.indices.size(); ++i) {
				t.indices[i] += result.vertices.size();
			}

			result.vertices.insert(result.vertices.end(), t.vertices.begin(), t.vertices.end());
			result.indices.insert(result.indices.end(), t.indices.begin(), t.indices.end());
		}
	}

	return result;
}

template <class Vertex>
inline TriangleList<Vertex> triangulate(ufo::PlanPath<3, float> const& path, float radius,
                                        int segments, bool use_arrow = true,
                                        float arrow_head_height = 0.5f)
{
	TriangleList<Vertex> result;

	for (std::size_t i{}; i < path.size() - 1; ++i) {
		auto a = static_cast<ufo::Vec3f>(*path[i]);
		auto b = static_cast<ufo::Vec3f>(*path[i + 1]);

		auto t = use_arrow
		             ? triangulateArrow<Vertex>(a, b, radius, arrow_head_height, segments)
		             : triangulate<Vertex>(ufo::Cylinder3(a, b, radius), segments);
		for (std::size_t i{}; i < t.indices.size(); ++i) {
			t.indices[i] += result.vertices.size();
		}

		result.vertices.insert(result.vertices.end(), t.vertices.begin(), t.vertices.end());
		result.indices.insert(result.indices.end(), t.indices.begin(), t.indices.end());
	}

	return result;
}

#endif  // UFO_VIZ_TRIANGULATE_HPP