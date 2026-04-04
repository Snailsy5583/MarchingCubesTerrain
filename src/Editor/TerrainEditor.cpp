//
// Created by r6awe on 3/27/2026.
//

#include "TerrainEditor.h"

#include "Engine/OBJ_Loader.h"
#include "Engine/Renderer.h"

#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "glm/gtc/type_ptr.hpp"

TerrainEditor::TerrainEditor()
	: Application(1280, 720, "Terrain Editor"),
	  m_TerrainGenerator(glm::vec3 {1, 1, 1}, 5, 0.5),
	  m_Camera((float) m_MainWindow->GetWindowSize().x /
			   m_MainWindow->GetWindowSize().y),
	  shader(Engine::Shader::Compile("shaders/demo.vert", "shaders/demo.frag"))

{
	m_LayerStack.Push(m_Camera.GetCameraControllerLayer());
	glfwMaximizeWindow(m_MainWindow->GetGLFWWindow());

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
}

void TerrainEditor::Update(float dt)
{
	m_Camera.Update(dt);
	shader.Bind();
	shader.SetUniformVec("lightPos", m_Camera.GetPosition());
	shader.SetUniformVec("viewPos", m_Camera.GetPosition());
	shader.SetUniformVec("lightColor", {1, 1, 1});
	Engine::Renderer::SubmitObject(m_Camera, test);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(0,
								 nullptr,
								 ImGuiDockNodeFlags_PassthruCentralNode);
	{	 // Inspector Window
		ImGui::Begin("Terrain Editor");

		ImGui::SeparatorText("Camera");

		m_Camera.ImGuiExposeParameters();

		ImGui::SeparatorText("Misc");
		ImGui::Text(std::to_string(1 / dt).c_str());

		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
