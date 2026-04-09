//
// Created by r6awe on 3/30/2026.
//

#ifndef TERRAINGENERATIONEDITOR_TERRAIN_H
#define TERRAINGENERATIONEDITOR_TERRAIN_H

#include "Engine/GameObject.h"
// #include "TerrainGenerator.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

struct ScalarFieldPoint;
typedef std::vector<ScalarFieldPoint> ScalarField;
class Terrain : public Engine::GameObject
{
public:
	Terrain(glm::vec3 size,
			int resolution,
			ScalarField &scalarField,
			float threshold);

	~Terrain() override = default;

public:
	[[nodiscard]] glm::vec3 GetSize() const { return m_Size; }
	[[nodiscard]] glm::vec3 GetVoxelSize() const
	{ return m_Size / glm::vec3(m_Resolution); }
	[[nodiscard]] int GetResolution() const { return m_Resolution; }
	[[nodiscard]] float GetThreshold() const { return m_Threshold; }
	[[nodiscard]] const glm::vec3 *GetBounds() const { return m_Bounds; }
	ScalarField *GetScalarFieldPtr() const { return &m_ScalarField; }

	[[nodiscard]] int GetIndex(const int x, const int y, const int z) const
	{ return x * m_Resolution * m_Resolution + y * m_Resolution + z; }
	[[nodiscard]] int GetIndex(const glm::ivec3 p) const
	{ return GetIndex(p.x, p.y, p.z); }
	[[nodiscard]] glm::ivec3 GetGridPointFromIndex(int i) const
	{
		glm::ivec3 gridPoint;
		gridPoint.x = i % m_Resolution;
		i /= m_Resolution;
		gridPoint.y = i % m_Resolution;
		i /= m_Resolution;
		gridPoint.z = i % m_Resolution;
		return gridPoint;
	}
	[[nodiscard]] glm::ivec3 GetNearestGridPoint(const glm::vec3 pos) const
	{
		return glm::clamp(GetNearestGridPointUnclamped(pos),
						  glm::ivec3(0),
						  (glm::ivec3) m_Size);
	}
	[[nodiscard]] glm::ivec3
	GetNearestGridPointUnclamped(const glm::vec3 pos) const
	{
		const auto p = (pos - m_Bounds[0]) / GetVoxelSize();
		return glm::round(p + glm::vec3(.000001));
	}

public:
	int GetNearestScalarFieldPointIndex(glm::vec2 mousePos,
										glm::vec2 windowSize,
										glm::mat4 view,
										glm::mat4 proj,
										float maxDist) const;

private:
	void MarchingCubes();

private:
	ScalarField &m_ScalarField;
	int m_Resolution;
	glm::vec3 m_Size;
	glm::vec3 m_Bounds[2];

	float m_Threshold;
};


#endif	  // TERRAINGENERATIONEDITOR_TERRAIN_H
