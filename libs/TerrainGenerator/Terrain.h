//
// Created by r6awe on 3/30/2026.
//

#ifndef TERRAINGENERATIONEDITOR_TERRAIN_H
#define TERRAINGENERATIONEDITOR_TERRAIN_H

#include "Engine/GameObject.h"
// #include "TerrainGenerator.h"
#include <functional>


struct ScalarFieldPoint;
class Terrain : public Engine::GameObject
{
public:
	Terrain(glm::vec3 size,
			int resolution,
			const std::vector<ScalarFieldPoint> &scalarField,
			float threshold);

	~Terrain() override = default;

public:
	inline glm::vec3 GetSize() const { return m_Size; }
	inline int GetResolution() const { return m_Resolution; }
	inline float GetThreshold() const { return m_Threshold; }
	inline const glm::vec3 *GetBounds() const { return m_Bounds; }

private:
	void MarchingCubes();

	inline int GetIndex(int x, int y, int z)
	{ return x * m_Resolution * m_Resolution + y * m_Resolution + z; }

private:
	const std::vector<ScalarFieldPoint> &m_ScalarField;
	int m_Resolution;
	glm::vec3 m_Size;
	glm::vec3 m_Bounds[2];

	float m_Threshold;
};


#endif	  // TERRAINGENERATIONEDITOR_TERRAIN_H
