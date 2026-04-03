//
// Created by r6awe on 3/30/2026.
//

#include "Terrain.h"

#include "TerrainGenerator.h"

Terrain::Terrain(glm::vec3 size,
				 int resolution,
				 const std::vector<ScalarFieldPoint> &scalarField,
				 float threshold)
	: GameObject(Engine::Mesh(), {0, 0, 0}, {1, 0, 0, 0}),
	  m_Resolution(resolution), m_Bounds {-size / 2.f, size / 2.f},
	  m_Size(size), m_Threshold(threshold), m_ScalarField {scalarField}
{
}

void Terrain::MarchingCubes() {}