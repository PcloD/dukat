#pragma once

#include <memory>
#include <vector>

namespace dukat
{
	class Mesh;
	class Vector3;
	struct Color;

	// Utility class the generate 3D meshes. Unless otherwise noted, this class
	// will generate unit-sized meshes (i.e., sphere with radius = 1, quad with edge length = 1)
	class MeshBuilder3
	{
	public:
		MeshBuilder3(void) { }
		~MeshBuilder3(void) { }

		// Creates a new coordinate axis mesh.
		std::unique_ptr<Mesh> build_axis(void);
		// Creates a new mesh for a set of points.
		std::unique_ptr<Mesh> build_points(const std::vector<Vector3>& points, const Color& color);
		// Creates a new cube mesh.
		std::unique_ptr<Mesh> build_cube(void);
		// Creates a new sphere mesh.
		std::unique_ptr<Mesh> build_sphere(int slices, int stacks, bool invert = false);
	};
}