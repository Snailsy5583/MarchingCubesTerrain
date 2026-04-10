//
// Created by r6awe on 3/27/2026.
//

#include "TerrainEditor.h"

#include "../../vendor/OBJLoader/OBJ_Loader.h"
#include "Engine/Renderer.h"

#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "glm/gtc/type_ptr.hpp"

TerrainEditor::TerrainEditor()
	: Application(1280, 720, "Terrain Editor"),
	  m_TerrainGenerator(glm::vec3 {100, 100, 100}, 100, 0.5),
	  m_Camera((float) m_MainWindow->GetWindowSize().x /
			   (float) m_MainWindow->GetWindowSize().y),
	  shader(Engine::Shader::Compile("shaders/demo.vert", "shaders/demo.frag"))

{
	// ImGui setup
	const float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(
		glfwGetPrimaryMonitor());	 // Valid on GLFW 3.3+ only
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void) io;
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard |	// Enable Keyboard Controls
		ImGuiConfigFlags_NavEnableGamepad |		// Enable Gamepad Controls
		ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingAlwaysTabBar = true;
	io.ConfigDockingNoDockingOver = true;

	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_MainWindow->GetGLFWWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	test = Engine::Mesh::ImportFromOBJ("assets/spot_triangulated_good.obj",
									   &shader);

	m_TerrainGenerator.GetTerrain().mesh.renderObj.shader = &shader;

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
	shader.Unbind();
	m_TerrainGenerator.GetTerrain().Render();
	Engine::Renderer::SubmitObject(m_Camera, m_TerrainGenerator.GetTerrain().mesh);
	//for (auto vertex : m_TerrainGenerator.GetTerrain().mesh.vertices) std::cout << vertex.x << ", " << vertex.y << ", " << vertex.z   << std::endl;
	ImGuiRender(dt);
}

void TerrainEditor::ImGuiRender(float dt)
{
	ImGui::DockSpaceOverViewport(
		0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
	{	 // Inspector Window
		ImGui::Begin("Editor");

		m_Camera.ImGuiExposeParameters();

		m_ScalarFieldEditor.ImGuiExposeParameters();

		ImGui::SeparatorText("Misc");
		ImGui::Text("%f", (1 / dt));

		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void TerrainEditor::OnEvent(Engine::Event &e)
{
	if (!ImGui::GetCurrentContext())
		return;
	if (const ImGuiIO &io = ImGui::GetIO(); !io.WantCaptureMouse)
		Application::OnEvent(e);
}
