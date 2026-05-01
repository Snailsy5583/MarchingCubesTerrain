//
// Created by r6awe on 3/27/2026.
//

#ifndef PUFFINTEXTEDITOR_TERRAINEDITOR_H
#define PUFFINTEXTEDITOR_TERRAINEDITOR_H

#include "Engine/ImGuiRenderer.h"
#include "FlyCamera.h"
#include "ScalarFieldEditor.h"

#include <Engine/Application.h>
#include <Engine/Renderer.h>

#include "TerrainGenerator/TerrainGenerator.h"

class TerrainEditor : public Engine::Application, Engine::IImGuiRender
{
public:
	TerrainEditor();

	void PollEvents() override;

	void Update(float dt) override;

	void Render(float dt);

	void ImGuiRender(float dt) override;
	void OnEvent(Engine::Event &e) override;

private:
	TerrainGenerator m_TerrainGenerator;
	ScalarFieldEditor m_ScalarFieldEditor;
	FlyCamera m_Camera;
	Engine::Shader shader;
	Engine::Mesh test;
	bool m_DebugNormals = false;
};


#endif	  // PUFFINTEXTEDITOR_TERRAINEDITOR_H
