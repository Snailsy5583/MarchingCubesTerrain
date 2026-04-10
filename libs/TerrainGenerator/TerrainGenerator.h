//
// Created by r6awe on 3/27/2026.
//

#ifndef PUFFINTEXTEDITOR_TERRAINGENERATOR_H
#define PUFFINTEXTEDITOR_TERRAINGENERATOR_H

#include "Terrain.h"

struct ScalarFieldPoint {
	glm::vec3 position;
	float scalar, hardness = 0;
};

class TerrainGenerator
{
public:
	TerrainGenerator(glm::vec3 size,
					 int resolution,
					 float threshold,
					 Engine::Shader *shader);

public:
	Terrain &GetTerrain() { return m_Terrain; }

private:
	float GetScalarFieldValue(glm::vec3 position);

	void InitScalarField();


private:
	ScalarField m_ScalarField;
	Terrain m_Terrain;
};


#endif	  // PUFFINTEXTEDITOR_TERRAINGENERATOR_H
