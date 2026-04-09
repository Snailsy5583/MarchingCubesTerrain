//
// Created by r6awe on 3/27/2026.
//

#include "TerrainGenerator.h"

#include "glm/gtc/noise.hpp"
#include <thread>

TerrainGenerator::TerrainGenerator(glm::vec3 size,
								   int resolution,
								   float threshold)
	: m_Terrain(size, resolution, m_ScalarField, threshold)
// {
// }
{ InitScalarField(); }

float TerrainGenerator::GetScalarFieldValue(glm::vec3 position)
{ return glm::simplex(position); }

void TerrainGenerator::InitScalarField()
{
	const glm::vec3 incr = m_Terrain.GetVoxelSize();
	const auto r = m_Terrain.GetResolution() + 1;
	m_ScalarField.resize(r * r * r);
	std::vector<std::thread> threads;

	auto f = [&](int x) {
		for (int y = 0; y < r; y++) {
			for (int z = 0; z < r; z++) {
				glm::vec3 position =
					m_Terrain.GetBounds()[0] +
					glm::vec3 {incr.x * x, incr.y * y, incr.z * z};
				float scalar = GetScalarFieldValue(position);
				m_ScalarField[m_Terrain.GetIndex(x, y, z)] = {
					position, scalar, 0};
			}
			// std::cout << scalar << ", ";
		}
		// std::cout << std::endl;
	};
	// std::cout << std::endl;

	for (int x = 0; x < r; x++) {
		threads.emplace_back(f, x);
	}

	for (int i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
}