//
// Created by r6awe on 3/27/2026.
//

#ifndef PUFFINTEXTEDITOR_TERRAINGENERATOR_H
#define PUFFINTEXTEDITOR_TERRAINGENERATOR_H

#include "Terrain.h"

class TerrainGenerator
{
public:
	TerrainGenerator(glm::vec3 size, int resolution, float threshold);

private:
	float GetScalarFieldValue(glm::vec3 position);

	void InitScalarField();

private:
	std::vector<std::pair<glm::vec3, float>> m_ScalarField;
	Terrain m_Terrain;
};


#endif	  // PUFFINTEXTEDITOR_TERRAINGENERATOR_H
