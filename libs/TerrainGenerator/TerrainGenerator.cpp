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
	// const glm::vec3 incr = m_Terrain.GetVoxelSize();
	const auto r = m_Terrain.GetResolution();
	Engine::glCheckError();
	m_ScalarField.clear();
	Engine::glCheckError();
	m_ScalarField.resize(r * r * r, {});
	Engine::glCheckError();

	Engine::ComputeShader sfPopulator = Engine::ComputeShader::Compile(
		glm::uvec3(r), "shaders/scalarFieldGen_FLAT.comp");
	Engine::glCheckError();

	sfPopulator.AttachTexture(std::make_unique<Engine::Texture3D>(
		glm::uvec3(r), 4, GL_FLOAT, GL_RGBA32F, m_ScalarField.data()));
	Engine::glCheckError();

	sfPopulator.Bind();
	Engine::glCheckError();
	sfPopulator.p_Textures[0]->BindImage();
	Engine::glCheckError();

	sfPopulator.SetUniform("octaves", m_Octaves);
	Engine::glCheckError();
	sfPopulator.SetUniform("lacunarity", m_Lacunarity);
	Engine::glCheckError();
	sfPopulator.SetUniform("gain", m_Gain);
	Engine::glCheckError();
	sfPopulator.SetUniformVec("scale", m_Scale / m_Terrain.GetSize());
	Engine::glCheckError();
	sfPopulator.SetUniformVec("translate", m_Translate);
	Engine::glCheckError();
	sfPopulator.SetUniformVec("resolution",
							  glm::vec3((float) m_Terrain.GetResolution()));
	Engine::glCheckError();

	sfPopulator.Dispatch();
	Engine::glCheckError();
	sfPopulator.Join();
	Engine::glCheckError();
	sfPopulator.Unbind();
	Engine::glCheckError();

	sfPopulator.p_Textures[0]->GetImage(m_ScalarField.data());
	Engine::glCheckError();

	// for (const auto &[scalar, gradient] : m_ScalarField) {
	// std::cout << scalar << ", " << glm::to_string(gradient) << std::endl;
	// }

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