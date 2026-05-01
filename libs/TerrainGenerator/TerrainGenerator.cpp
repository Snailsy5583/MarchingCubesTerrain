//
// Created by r6awe on 3/27/2026.
//
#include "glad/glad.h"

#include "TerrainGenerator.h"

#include "glm/gtc/noise.hpp"
#include "glm/gtx/transform.hpp"
#include "imgui.h"

#include <Engine/Shader.h>

TerrainGenerator::TerrainGenerator(const glm::vec3 size,
								   const int resolution,
								   const float threshold,
								   Engine::Shader *shader)
	: m_Terrain(size, resolution, m_ScalarField, threshold, shader)
{ GenerateTerrain(); }

void TerrainGenerator::GenerateTerrain()
{
	const auto r = m_Terrain.GetResolution();
	m_ScalarField.clear();
	m_ScalarField.resize(r * r * r, {});

	Engine::ComputeShader sfPopulator = Engine::ComputeShader::Compile(
		glm::uvec3(r), "shaders/scalarFieldGen_FLAT.comp");

	sfPopulator.AttachTexture(std::make_unique<Engine::Texture3D>(
		glm::uvec3(r), 4, GL_FLOAT, GL_RGBA32F, m_ScalarField.data()));

	sfPopulator.Bind();
	sfPopulator.p_Textures[0]->BindImage();

	sfPopulator.SetUniform("octaves", m_Octaves);
	sfPopulator.SetUniform("lacunarity", m_Lacunarity);
	sfPopulator.SetUniform("gain", m_Gain);
	sfPopulator.SetUniformVec("scale", m_Scale / m_Terrain.GetSize());
	sfPopulator.SetUniformVec("translate", m_Translate);
	sfPopulator.SetUniformVec("resolution",
							  glm::vec3((float) m_Terrain.GetResolution()));

	sfPopulator.Dispatch();
	sfPopulator.Join();
	sfPopulator.Unbind();

	sfPopulator.p_Textures[0]->GetImage(m_ScalarField.data());

	m_Terrain.RecalculateGradients();
	m_Terrain.MarchingCubes();
	Engine::glCheckError();
}

void TerrainGenerator::ImGuiRender(float dt)
{
	if (ImGui::TreeNode("Terrain Generator")) {
		m_Terrain.ImGuiRender(dt);

		ImGui::DragInt("Octaves", &m_Octaves, .25f, 1);
		ImGui::DragFloat("Lacunarity", &m_Lacunarity, .001f);
		ImGui::DragFloat("Gain", &m_Gain, 0.001f);
		ImGui::DragFloat3("Scale", &m_Scale[0], 0.01f);
		ImGui::DragFloat3("Translate", &m_Translate[0]);
		if (ImGui::Button("Regenerate Terrain"))
			GenerateTerrain();
		ImGui::TreePop();
	}
}