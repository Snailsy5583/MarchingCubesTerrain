//
// Created by r6awe on 3/30/2026.
//

#ifndef TERRAINGENERATIONEDITOR_TERRAIN_H
#define TERRAINGENERATIONEDITOR_TERRAIN_H

#include "Engine/Mesh.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "Engine/ImGuiRenderer.h"
#include "glm/gtx/string_cast.hpp"

struct ScalarFieldPoint;
typedef std::vector<ScalarFieldPoint> ScalarField;
class Terrain;

class TerrainChunk
{
	friend class Terrain;

public:
	TerrainChunk(glm::ivec3 minCell,
				 glm::ivec3 maxCell,
				 ScalarField &scalarField,
				 int id,
				 Engine::Shader *shader);

private:
	void Rebuild(const Terrain &terrain);
	void RebuildGeometry(const Terrain &terrain);
	void UpdateMesh();
	void Render() const;
	void SetShader(Engine::Shader *shader);

	std::unique_ptr<Engine::Mesh> m_Mesh;
	glm::ivec3 m_MinCell;
	glm::ivec3 m_MaxCell;
	ScalarField &m_ScalarField;
	int m_ID;
};

class Terrain : public Engine::IImGuiRender
{
	friend class TerrainChunk;

public:
	Terrain(glm::vec3 size,
			glm::ivec3 resolution,
			ScalarField &scalarField,
			float threshold,
			Engine::Shader *shader);

	~Terrain() override = default;

	void ImGuiRender(float dt) override;

public:
	[[nodiscard]] glm::vec3 GetSize() const { return m_Size; }
	[[nodiscard]] glm::vec3 GetVoxelSize() const
	{ return m_Size / glm::vec3(m_Resolution); }
	[[nodiscard]] glm::ivec3 GetResolution() const { return m_Resolution; }
	[[nodiscard]] float GetThreshold() const { return m_Threshold; }
	[[nodiscard]] const glm::vec3 *GetBounds() const { return m_Bounds; }
	[[nodiscard]] ScalarField *GetScalarFieldPtr() const
	{ return &m_ScalarField; }


	[[nodiscard]] glm::ivec3 GridCoordFromScalarIndex(const int i) const
	{
		return {i / (m_Resolution.y * m_Resolution.z),
				(i / m_Resolution.z) % m_Resolution.y,
				i % m_Resolution.z};
	}
	[[nodiscard]] glm::vec3 WorldPositionFromScalarIndex(const int i) const
	{
		return m_Bounds[0] +
			   GetVoxelSize() * (glm::vec3) GridCoordFromScalarIndex(i);
	}
	[[nodiscard]] glm::vec3
	WorldPositionFromGridCoord(glm::ivec3 gridCoord) const
	{ return m_Bounds[0] + GetVoxelSize() * (glm::vec3) gridCoord; }
	[[nodiscard]] int
	ScalarIndexFromGridCoord(const int x, const int y, const int z) const
	{ return x * m_Resolution.y * m_Resolution.z + y * m_Resolution.z + z; }
	[[nodiscard]] int ScalarIndexFromGridCoord(const glm::ivec3 gridCoord) const
	{ return ScalarIndexFromGridCoord(gridCoord.x, gridCoord.y, gridCoord.z); }
	[[nodiscard]] glm::ivec3
	NearestGridCoordFromWorldPosition(const glm::vec3 worldPosition) const
	{
		return glm::clamp(GridCoordFromWorldPositionUnclamped(worldPosition),
						  glm::ivec3(0),
						  m_Resolution - glm::ivec3(1));
	}
	[[nodiscard]] glm::ivec3
	GridCoordFromWorldPositionUnclamped(const glm::vec3 worldPosition) const
	{
		const auto gridPosition =
			(worldPosition - m_Bounds[0]) / GetVoxelSize();
		return glm::round(gridPosition + glm::vec3(.000001));
	}


	[[nodiscard]] bool IsWorldPositionSolid(glm::vec3 worldPosition) const;


public:
	[[nodiscard]] int RayCastFromMousePos(glm::vec2 mouseNdc,
										  glm::mat4 view,
										  glm::mat4 proj,
										  float maxDist) const;
	[[nodiscard]] int
	RayCast(glm::vec3 origin, glm::vec3 dir, float maxDist) const;
	void SetShader(Engine::Shader *shader);
	void Render() const;

	void RecalculateGradients();
	void RecalculateGradients(glm::ivec3 dirtyMin, glm::ivec3 dirtyMax);
	void RecalculateChunks();
	void MarchingCubes() const;
	void MarchingCubes(glm::ivec3 dirtyMin, glm::ivec3 dirtyMax) const;

private:
	[[nodiscard]] bool IsBoundaryScalarFieldPoint(int index) const;
	[[nodiscard]] float GetScalarValueForMeshing(int index,
												 float isovalue) const;

	struct GridCell {
		glm::vec3 vertex[8];
		float value[8];
	};


	/**
	 * Given a `cell`, calculate its cube index
	 * The cube index is an 8-bit encoding. Each bit represents a vertex.
	 * `index[i]` is the ith bit If the value at the ith vertex is < isovalue,
	 * `index[i]` = 1. Else, `index[i]` = 0
	 */
	[[nodiscard]] int calculate_cube_index(const std::vector<int> &cell,
										   float isovalue) const;

	/// Find the point between `v1` and `v2` where the functional value =
	/// `isovalue`
	static glm::vec3 interpolate(const glm::vec3 &v1,
								 float val1,
								 const glm::vec3 &v2,
								 float val2,
								 float isovalue);

	/// Returns all intersection coordinates of a cell with the isosurface
	/// (Calls `interpolate()`)
	[[nodiscard]] std::vector<glm::vec3>
	get_intersection_points(const std::vector<int> &cell, float isovalue) const;
	[[nodiscard]] std::vector<glm::vec3>
	get_intersection_normals(const std::vector<int> &cell,
							 float isovalue) const;


	/// Given `cubeIndex`, get the edge table entry and using `intersections`,
	/// make all triangles
	std::vector<std::vector<glm::vec3>>
	get_triangles(std::vector<glm::vec3> &intersections, int cubeIndex);

	/// Get triangles of a single cell
	std::vector<std::vector<glm::vec3>> triangulate_cell(std::vector<int> &cell,
														 float isovalue);
	void AppendCellTriangles(int i,
							 int j,
							 int k,
							 std::vector<Engine::Vertex> &vertices,
							 std::vector<unsigned int> &indices) const;
	void RecalculateGradientAt(int x, int y, int z, glm::vec3 voxelSize);
	[[nodiscard]] glm::ivec3 GetChunkCounts() const;
	[[nodiscard]] int GetChunkIndex(glm::ivec3 chunk) const;

private:
	ScalarField &m_ScalarField;
	glm::ivec3 m_Resolution;
	glm::vec3 m_Size;
	glm::vec3 m_Bounds[2];
	std::vector<std::unique_ptr<TerrainChunk>> m_Chunks;
	glm::ivec3 m_ChunkCounts {0};
	Engine::Shader *m_Shader;

	float m_Threshold;
	glm::ivec3 m_ChunkCellSize {8};
};


#endif	  // TERRAINGENERATIONEDITOR_TERRAIN_H
