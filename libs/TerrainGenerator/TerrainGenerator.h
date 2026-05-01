//
// Created by r6awe on 3/27/2026.
//

#ifndef PUFFINTEXTEDITOR_TERRAINGENERATOR_H
#define PUFFINTEXTEDITOR_TERRAINGENERATOR_H

#include "Engine/ImGuiRenderer.h"
#include "Terrain.h"

struct ScalarFieldPoint {
	float scalar;
	glm::vec3 gradient;
};

class TerrainGenerator : public Engine::IImGuiRender
{
public:
	TerrainGenerator(glm::vec3 size,
					 int resolution,
					 float threshold,
					 Engine::Shader *shader);

public:
	Terrain &GetTerrain() { return m_Terrain; }

	void ImGuiRender(float dt) override;

private:
	void GenerateTerrain();

private:
	ScalarField m_ScalarField {};
	Terrain m_Terrain;

	// generation parameters
	int m_Octaves = 4;
	float m_Lacunarity = 1.8, m_Gain = .35f;
	glm::vec3 m_Translate {0}, m_Scale {10.f};
};


#endif	  // PUFFINTEXTEDITOR_TERRAINGENERATOR_H
