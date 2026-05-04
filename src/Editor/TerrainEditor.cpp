//
// Created by r6awe on 3/27/2026.
//

#include "TerrainEditor.h"

#include "../../vendor/OBJLoader/OBJ_Loader.h"
#include "Engine/Renderer.h"

#include "Engine/ImGuiRenderer.h"
#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "glm/gtc/type_ptr.hpp"

TerrainEditor::TerrainEditor()
	: Application(1280, 720, "Terrain Editor"),
	  m_Camera((float) m_MainWindow->GetWindowSize().x /
			   (float) m_MainWindow->GetWindowSize().y),
	  shader(Engine::Shader::Compile("shaders/demo.vert", "shaders/demo.frag")),
	  m_TerrainGenerator(glm::vec3 {10, 10, 10}, {30, 30, 30}, 0.0, &shader),
	  test("assets/spot_triangulated_good.obj", &shader, Engine::Mesh::Static)

{
	Engine::ImGuiRenderer::ImGuiInit(m_MainWindow->GetGLFWWindow());

	// Setup layer stack
	m_LayerStack.Push(m_Camera.GetLayer());
	m_LayerStack.Push(m_ScalarFieldEditor.GetLayer());

	glfwMaximizeWindow(m_MainWindow->GetGLFWWindow());
}
void TerrainEditor::PollEvents()
{
	Application::PollEvents();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void TerrainEditor::Update(float dt)
{
	Engine::glCheckError();
	m_ScalarFieldEditor.UpdateScalarFieldIfMouseDown(
		dt,
		m_TerrainGenerator.GetTerrain(),
		m_Camera.GetViewMatrix(),
		m_Camera.GetProjectionMatrix());
	m_Camera.Update(dt);

	Render(dt);
}

void TerrainEditor::Render(float dt)
{
	shader.Bind();
	shader.SetUniformVec("lightPos", m_Camera.GetPosition());
	shader.SetUniformVec("viewPos", m_Camera.GetPosition());
	shader.SetUniformVec("lightColor", {1, 1, 1});
	shader.SetUniform("debugNormals", m_DebugNormals);
	shader.Unbind();

	m_TerrainGenerator.GetTerrain().Render();

	// Engine::ImGuiRenderer::RenderAll("Inspector", dt);
	ImGuiRender(dt);
}

void TerrainEditor::ImGuiRender(float dt)
{
	ImGui::DockSpaceOverViewport(
		0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
	{	 // Inspector Window
		ImGui::Begin("Inspector");
		m_TerrainGenerator.ImGuiRender(dt);

		m_Camera.ImGuiRender(dt);
		m_ScalarFieldEditor.ImGuiRender(dt);

		ImGui::SeparatorText("Misc");
		ImGui::Checkbox("Debug Normals", &m_DebugNormals);
		ImGui::Text("%f", (1 / dt));

		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void TerrainEditor::OnEvent(Engine::Event &e)
{
	if (!ImGui::GetCurrentContext())
		Application::OnEvent(e);
	if (const ImGuiIO &io = ImGui::GetIO(); !io.WantCaptureMouse)
		Application::OnEvent(e);
}
