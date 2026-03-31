//
// Created by r6awe on 3/30/2026.
//

#include "Terrain.h"

Terrain::Terrain(glm::vec3 size,
				 int resolution,
				 const std::vector<std::pair<glm::vec3, float>> &scalarField,
				 float threshold)
	: GameObject(Engine::Mesh(), {0, 0, 0}, {1, 0, 0, 0}),
	  m_Resolution(resolution), m_Bounds {-size / 2.f, size / 2.f},
	  m_Size(size), m_Threshold(threshold), m_ScalarField {scalarField}
{
}

void Terrain::MarchingCubes() {}