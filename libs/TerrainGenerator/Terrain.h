//
// Created by r6awe on 3/30/2026.
//

#ifndef TERRAINGENERATIONEDITOR_TERRAIN_H
#define TERRAINGENERATIONEDITOR_TERRAIN_H

#include "Engine/GameObject.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "Engine/ImGuiRenderer.h"
#include "glm/gtx/string_cast.hpp"

struct ScalarFieldPoint;
typedef std::vector<ScalarFieldPoint> ScalarField;
class Terrain : public Engine::GameObject, public Engine::IImGuiRender
{
public:
	Terrain(glm::vec3 size,
			int resolution,
			ScalarField &scalarField,
			float threshold,
			Engine::Shader *shader);

	~Terrain() override = default;

	void ImGuiRender(float dt) override;

public:
	[[nodiscard]] glm::vec3 GetSize() const { return m_Size; }
	[[nodiscard]] glm::vec3 GetVoxelSize() const
	{ return m_Size / glm::vec3(m_Resolution); }
	[[nodiscard]] int GetResolution() const { return m_Resolution; }
	[[nodiscard]] float GetThreshold() const { return m_Threshold; }
	[[nodiscard]] const glm::vec3 *GetBounds() const { return m_Bounds; }
	[[nodiscard]] ScalarField *GetScalarFieldPtr() const
	{ return &m_ScalarField; }


	[[nodiscard]] glm::ivec3 GetScalarFieldPointPos(const int i) const
	{
		return {i / m_Resolution / m_Resolution,
				(i / m_Resolution) % m_Resolution,
				i % m_Resolution};
	}
	[[nodiscard]] glm::vec3 Index2Pos(const int i) const
	{
		return m_Bounds[0] +
			   GetVoxelSize() * (glm::vec3) GetScalarFieldPointPos(i);
	}
	[[nodiscard]] int GetIndex(const int x, const int y, const int z) const
	{ return x * m_Resolution * m_Resolution + y * m_Resolution + z; }
	[[nodiscard]] int GetIndex(const glm::ivec3 p) const
	{ return GetIndex(p.x, p.y, p.z); }
	[[nodiscard]] glm::ivec3 GetGridPointFromIndex(const int i) const
	{
		return {i / (m_Resolution * m_Resolution),
				(i / m_Resolution) % m_Resolution,
				i % m_Resolution};
	}
	[[nodiscard]] glm::ivec3 GetNearestGridPoint(const glm::vec3 pos) const
	{
		return glm::clamp(GetNearestGridPointUnclamped(pos),
						  glm::ivec3(0),
						  glm::ivec3(m_Resolution - 1));
	}
	[[nodiscard]] glm::ivec3
	GetNearestGridPointUnclamped(const glm::vec3 pos) const
	{
		const auto p = (pos - m_Bounds[0]) / GetVoxelSize();
		return glm::round(p + glm::vec3(.000001));
	}


	bool IsPosInsideTerrain(const glm::vec3 pos) const;


public:
	int RayCastFromMousePos(glm::vec2 mouseNdc,
							glm::mat4 view,
							glm::mat4 proj,
							float maxDist) const;
	int RayCast(glm::vec3 origin, glm::vec3 dir, float maxDist) const;
	void SetShader(Engine::Shader *shader) const
	{ m_Mesh->GenRendererObj(shader); }

	void RecalculateGradients();
	void MarchingCubes() const;

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
	int calculate_cube_index(const std::vector<int> &cell,
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
	std::vector<glm::vec3> get_intersection_points(const std::vector<int> &cell,
												   float isovalue) const;
	std::vector<glm::vec3>
	get_intersection_normals(const std::vector<int> &cell,
							 float isovalue) const;


	/// Given `cubeIndex`, get the edge table entry and using `intersections`,
	/// make all triangles
	std::vector<std::vector<glm::vec3>>
	get_triangles(std::vector<glm::vec3> &intersections, int cubeIndex);

	/// Get triangles of a single cell
	std::vector<std::vector<glm::vec3>> triangulate_cell(std::vector<int> &cell,
														 float isovalue);

private:
	ScalarField &m_ScalarField;
	int m_Resolution;
	glm::vec3 m_Size;
	glm::vec3 m_Bounds[2];

	float m_Threshold;
};


#endif	  // TERRAINGENERATIONEDITOR_TERRAIN_H
