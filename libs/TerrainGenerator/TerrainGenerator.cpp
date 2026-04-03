//
// Created by r6awe on 3/27/2026.
//

#include "TerrainGenerator.h"

#include "glm/gtc/noise.hpp"
TerrainGenerator::TerrainGenerator(glm::vec3 size,
								   int resolution,
								   float threshold)
	: m_Terrain(size, resolution, m_ScalarField, threshold)
{ InitScalarField(); }

ScalarFieldPoint TerrainGenerator::GetScalarFieldPoint(glm::vec3 position)
{ return {position, glm::simplex(position), 0}; }

void TerrainGenerator::InitScalarField()
{
	glm::vec3 incr {m_Terrain.GetSize().x / m_Terrain.GetResolution(),
					m_Terrain.GetSize().y / m_Terrain.GetResolution(),
					m_Terrain.GetSize().z / m_Terrain.GetResolution()};
	for (int x = 0; x < m_Terrain.GetResolution(); x++) {
		for (int y = 0; y < m_Terrain.GetResolution(); y++) {
			for (int z = 0; z < m_Terrain.GetResolution(); z++) {
				glm::vec3 position = m_Terrain.GetBounds()[0] + incr.x * x +
									 incr.y * y + incr.z * z;
				auto point = GetScalarFieldPoint(position);
				m_ScalarField.push_back(point);
				std::cout << point.scalar << ", ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}