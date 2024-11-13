#define PAR_SHAPES_IMPLEMENTATION
#define FAST_OBJ_IMPLEMENTATION
#include <par_shapes.h>
#include <fast_obj.h>
#include "Mesh.h"
#include <cassert>
#include <cstdio>

void Upload(Mesh* mesh);

void GenCube(Mesh* mesh, float width, float height, float length);

void CreateMesh(Mesh* mesh, const char* path)
{
	fastObjMesh* obj = fast_obj_read(path);
	int count = obj->index_count;
	mesh->positions.resize(count);
	mesh->normals.resize(count);
	std::vector<fastObjIndex> indices(obj->index_count);
	memcpy(indices.data(), obj->indices, indices.size() * sizeof(fastObjIndex));
	
	assert(obj->position_count > 1);
	for (int i = 0; i < count; i++)
	{
		// Using the obj file's indices, populate the mesh->positions with the object's vertex positions
		fastObjIndex idx = obj->indices[i];
		Vector3 position = ((Vector3*)obj->positions)[idx.p];
		mesh->positions[i] = position;
	}
	
	assert(obj->normal_count > 1);
	for (int i = 0; i < count; i++)
	{
		// Using the obj file's indices, populate the mesh->normals with the object's vertex normals
		fastObjIndex idx = obj->indices[i];
		Vector3 normal = ((Vector3*)obj->normals)[idx.n];
		mesh->normals[i] = normal;
	}
	
	if (obj->texcoord_count > 1)
	{
		mesh->tcoords.resize(count);
		for (int i = 0; i < count; i++)
		{
			// Using the obj file's indices, populate the mesh->tcoords with the object's vertex texture coordinates
			fastObjIndex idx = obj->indices[i];
			Vector2 tcoord = ((Vector2*)obj->texcoords)[idx.t];
			mesh->tcoords[i] = tcoord;
		}
	}
	else
	{
		printf("**Warning: mesh %s loaded without texture coordinates**\n", path);
	}
	fast_obj_destroy(obj);
	mesh->count = count;

	Upload(mesh);
}

void CreateMesh(Mesh* mesh, ShapeType shape)
{
	// 1. Generate par_shapes_mesh
	par_shapes_mesh* par = nullptr;
	switch (shape)
	{
	case PLANE:
		par = par_shapes_create_plane(1, 1);
		break;

		// No longer supported because par platonic solids work differently than par parametrics
	case CUBE:
		//par = par_shapes_create_cube();
		break;

	case SPHERE:
		par = par_shapes_create_parametric_sphere(8, 8);
		break;

	default:
		assert(false, "Invalid shape type");
		break;
	}
	
	if (par != nullptr)
	{
		par_shapes_compute_normals(par);

		// 2. Convert par_shapes_mesh to our Mesh representation
		int count = par->ntriangles * 3;	// 3 points per triangle
		mesh->count = count;
		mesh->indices.resize(count);
		memcpy(mesh->indices.data(), par->triangles, count * sizeof(uint16_t));
		mesh->positions.resize(par->npoints);
		memcpy(mesh->positions.data(), par->points, par->npoints * sizeof(Vector3));
		mesh->normals.resize(par->npoints);
		memcpy(mesh->normals.data(), par->normals, par->npoints * sizeof(Vector3));
		mesh->tcoords.resize(par->npoints);
		memcpy(mesh->tcoords.data(), par->tcoords, par->npoints * sizeof(Vector2));
		par_shapes_free_mesh(par);
	}
	else
	{
		assert(shape == CUBE);
		GenCube(mesh, 1.0f, 1.0f, 1.0f);
	}

	// 3. Upload Mesh to GPU
	Upload(mesh);
}

void DestroyMesh(Mesh* mesh)
{
	glDeleteBuffers(1, &mesh->ebo);
	glDeleteBuffers(1, &mesh->tbo);
	glDeleteBuffers(1, &mesh->nbo);
	glDeleteBuffers(1, &mesh->pbo);
	glDeleteVertexArrays(1, &mesh->vao);

	mesh->vao = mesh->pbo = mesh->nbo = mesh->tbo = mesh->ebo = GL_NONE;
}

void DrawMesh(const Mesh& mesh)
{
	glBindVertexArray(mesh.vao);
	if (mesh.ebo != GL_NONE)
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);
	else
		glDrawArrays(GL_TRIANGLES, 0, mesh.count);
	glBindVertexArray(GL_NONE);
}

void Upload(Mesh* mesh)
{
	GLuint vao, pbo, nbo, tbo, ebo;
	vao = pbo = nbo = tbo = ebo = GL_NONE;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_ARRAY_BUFFER, pbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &nbo);
	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(Vector3), mesh->normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
	glEnableVertexAttribArray(1);

	if (!mesh->tcoords.empty())
	{
		glGenBuffers(1, &tbo);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->tcoords.size() * sizeof(Vector2), mesh->tcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
		glEnableVertexAttribArray(2);
	}
	
	if (!mesh->indices.empty())
	{
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(uint16_t), mesh->indices.data(), GL_STATIC_DRAW);
	}

	glBindVertexArray(GL_NONE);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);

	mesh->vao = vao;
	mesh->pbo = pbo;
	mesh->nbo = nbo;
	mesh->tbo = tbo;
	mesh->ebo = ebo;
}

void GenCube(Mesh* mesh, float width, float height, float length)
{
	float positions[] = {
		-width / 2, -height / 2, length / 2,
		width / 2, -height / 2, length / 2,
		width / 2, height / 2, length / 2,
		-width / 2, height / 2, length / 2,
		-width / 2, -height / 2, -length / 2,
		-width / 2, height / 2, -length / 2,
		width / 2, height / 2, -length / 2,
		width / 2, -height / 2, -length / 2,
		-width / 2, height / 2, -length / 2,
		-width / 2, height / 2, length / 2,
		width / 2, height / 2, length / 2,
		width / 2, height / 2, -length / 2,
		-width / 2, -height / 2, -length / 2,
		width / 2, -height / 2, -length / 2,
		width / 2, -height / 2, length / 2,
		-width / 2, -height / 2, length / 2,
		width / 2, -height / 2, -length / 2,
		width / 2, height / 2, -length / 2,
		width / 2, height / 2, length / 2,
		width / 2, -height / 2, length / 2,
		-width / 2, -height / 2, -length / 2,
		-width / 2, -height / 2, length / 2,
		-width / 2, height / 2, length / 2,
		-width / 2, height / 2, -length / 2
	};

	float tcoords[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	float normals[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 0.0f,-1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		0.0f,-1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f
	};

	mesh->positions.resize(24);
	mesh->normals.resize(24);
	mesh->tcoords.resize(24);
	memcpy(mesh->positions.data(), positions, 24 * sizeof(Vector3));
	memcpy(mesh->normals.data(), normals, 24 * sizeof(Vector3));
	memcpy(mesh->tcoords.data(), tcoords, 24 * sizeof(Vector2));

	int k = 0;
	mesh->indices.resize(36);
	for (int i = 0; i < 36; i += 6)
	{
		mesh->indices[i] = 4 * k;
		mesh->indices[i + 1] = 4 * k + 1;
		mesh->indices[i + 2] = 4 * k + 2;
		mesh->indices[i + 3] = 4 * k;
		mesh->indices[i + 4] = 4 * k + 2;
		mesh->indices[i + 5] = 4 * k + 3;

		k++;
	}

	mesh->count = 36;
}
