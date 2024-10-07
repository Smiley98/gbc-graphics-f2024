#define PAR_SHAPES_IMPLEMENTATION
#include "Mesh.h"
#include <cassert>

void CreateMesh(Mesh* mesh, ShapeType shape)
{
	// 1. Generate par_shapes_mesh
	par_shapes_mesh* par = nullptr;
	par = par_shapes_create_plane(1, 1);
	par_shapes_compute_normals(par);

	// 2. Convert par_shapes_mesh to our Mesh representation
	int count = par->ntriangles * 3;	// 3 points per triangle
	mesh->indices.resize(count);
	memcpy(mesh->indices.data(), par->triangles, count * sizeof(uint16_t));
	mesh->positions.resize(par->npoints);
	memcpy(mesh->positions.data(), par->points, par->npoints * sizeof(Vector3));
	mesh->normals.resize(par->npoints);
	memcpy(mesh->normals.data(), par->normals, par->npoints * sizeof(Vector3));
	if (par->tcoords != nullptr)
	{
		mesh->tcoords.resize(par->npoints);
		memcpy(mesh->tcoords.data(), par->tcoords, par->npoints * sizeof(Vector2));
	}

	// 3. Upload Mesh to GPU
	assert(false);
}

void DestroyMesh(Mesh* mesh)
{

}
