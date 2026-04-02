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
	  m_TerrainGenerator(glm::vec3 {1, 1, 1}, 20, 0.5),
	  m_Camera((float) m_MainWindow->GetWindowSize().x /
			   m_MainWindow->GetWindowSize().y),
	  shader(Engine::Shader::Compile("C:"
									 "\\Dev\\College\\4361\\FinalProject\\March"
									 "ingCubesTerrain\\src\\Editor"
									 "\\demo.vert",
									 "C:"
									 "\\Dev\\College\\4361\\FinalProject\\March"
									 "ingCubesTerrain\\src\\Editor"
									 "\\demo.frag"))

{
	m_LayerStack.Push(m_Camera.GetCameraControllerLayer());
	glfwMaximizeWindow(m_MainWindow->GetGLFWWindow());
	const float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(
		glfwGetPrimaryMonitor());	 // Valid on GLFW 3.3+ only
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void) io;
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard;	   // Enable Keyboard Controls
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(
		main_scale);	// Bake a fixed style scale. (until we have a solution
						// for dynamic style scaling, changing this requires
						// resetting Style + calling this again)
	style.FontScaleDpi =
		main_scale;	   // Set initial font scale. (in docking branch: using
					   // io.ConfigDpiScaleFonts=true automatically overrides
					   // this for every window depending on the current
					   // monitor)

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_MainWindow->GetGLFWWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 330");

	shader.Bind();
	shader.SetUniformVec("lightPos", m_Camera.GetPosition());
	shader.SetUniformVec("viewPos", m_Camera.GetPosition());
	shader.SetUniformVec("lightColor", {1, 1, 1});
	shader.UnBind();

	test = Engine::Mesh::ImportFromOBJ(
		"C:\\Users\\r6awe\\Desktop\\blender\\chicken.obj",
		&shader);
}

void TerrainEditor::Update(float dt)
{
	m_Camera.Update(dt);
	shader.Bind();
	shader.SetUniformVec("lightPos", m_Camera.GetPosition());
	shader.SetUniformVec("viewPos", m_Camera.GetPosition());
	Engine::Renderer::SubmitObject(m_Camera, test);

	{	 // Inspector Window
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Terrain Editor");

		ImGui::Separator();

		m_Camera.ImGuiExposeParameters();

		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}
