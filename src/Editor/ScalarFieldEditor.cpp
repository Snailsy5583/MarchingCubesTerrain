//
// Created by r6awe on 4/6/2026.
//

#include "ScalarFieldEditor.h"

#include "TerrainGenerator/TerrainGenerator.h"

#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Events/MouseEvents.h"
#include "Engine/Renderer.h"
#include "TerrainGenerator/ErosionSystem.h"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>

//////////////////////////////////// Brush /////////////////////////////////////

Brush::Brush(const float size,
			 const float weight,
			 const float maxDist,
			 const FalloffType type,
			 const EasingType easeType)
	: m_BrushSize(size), m_Weight(weight), m_MaxDistance(maxDist),
	  m_Falloff(type), m_EasingType(easeType), m_CustomVarX(0),
	  m_ExprVars {{"x", &m_CustomVarX}, {"size", &m_BrushSize}}
{
}

float Brush::GetWeightWithFalloff(const float dist)
{
	const float x = dist / m_BrushSize;
	float f;
	switch (m_Falloff) {
	case Custom: f = CustomFalloff(x); break;
	case Linear: f = LinearFalloff(x); break;
	case Quadratic: f = QuadraticFalloff(x); break;
	case Cubic: f = CubicFalloff(x); break;
	case Sine: f = SineFalloff(x); break;
	default: f = ConstantFalloff(x); break;
	}
	return f * m_Weight;
}

void Brush::ImGuiRender(float dt)
{
	if (ImGui::TreeNode("Brush Settings")) {
		const char *falloff[] = {
			"Custom", "Constant", "Linear", "Quadratic", "Cubic", "Sine"};
		const char *easing[] = {"In", "Out", "Both"};
		const char *actions[] = {"Raise", "Lower", "Smooth", "Erosion"};

		ImGui::DragFloat("Brush Size", &m_BrushSize, 0.1f);
		ImGui::DragFloat("Weight", &m_Weight, 0.001f, -1, 1);
		ImGui::DragFloat("Max Distance", &m_MaxDistance, 0.1f);

		ImGui::Combo("Falloff", (int *) &m_Falloff, falloff, 6);
		if (m_Falloff == Custom) {
			ImGui::InputText("CustomFormula\nvars:\n\tx = distance from "
							 "click origin\n\tsize = brush size",
							 &m_CustomExprStr);

			if (ImGui::Button("Compile")) {
				std::cout << m_CustomExprStr << std::endl;
				CompileCustomExpr();
				ImGui::OpenPopup("CompilePopup");
			}

			if (ImGui::BeginPopup("CompilePopup")) {
				ImGui::Text(m_CustomExpr == nullptr ? "Formula Compile Error."
													: "Compile success!");
				ImGui::EndPopup();
			}
		}

		else if (m_Falloff != Constant && m_Falloff != Linear)
			ImGui::Combo("Easing Type", (int *) &m_EasingType, easing, 3);

		ImGui::Combo("Action", (int *) &m_ChosenAction, actions, 4);
		ImGui::TreePop();
	}
}

/////////////////////////// Scalar Field Editor ////////////////////////////////

ScalarFieldEditor::ScalarFieldEditor()
	: m_Layer(this), m_Brush(1, .5, 1000, Brush::Cubic, Brush::Both)
{
}

void ScalarFieldEditor::UpdateScalarFieldIfMouseDown(const float dt,
													 Terrain &terrain,
													 const glm::mat4 &view,
													 const glm::mat4 &proj)
{
	ScalarField *scalarField = terrain.GetScalarFieldPtr();
	if (m_ErodeWhole) {
		ErosionSystem::GetInstance()->ErodeWhole(&terrain);
		return;
	}
	if (!m_IsMouseDown)
		return;


	const int nearestSFPIndex = terrain.RayCastFromMousePos(
		m_MousePos, view, proj, m_Brush.m_MaxDistance);
	if (nearestSFPIndex == -1)
		return;

	auto centerWorldPosition =
		terrain.WorldPositionFromScalarIndex(nearestSFPIndex);
	glm::vec3 line[2] = {view[3], centerWorldPosition};
	Engine::Renderer::SubmitObject(
		Engine::Renderer::GenLine(line, 1, Engine::Shader::p_ShaderList[0])
			.get());

	const glm::ivec3 brushBounds[2] = {
		terrain.NearestGridCoordFromWorldPosition(
			centerWorldPosition - glm::vec3(m_Brush.m_BrushSize / 2)),
		terrain.NearestGridCoordFromWorldPosition(
			centerWorldPosition + glm::vec3(m_Brush.m_BrushSize / 2))};
	switch (m_Brush.m_ChosenAction) {
	case Brush::Raise:
	case Brush::Lower:
		ActionRaiseLower(
			dt, terrain, scalarField, centerWorldPosition, brushBounds);
		break;
	case Brush::Smooth:
		ActionSmooth(
			dt, terrain, scalarField, centerWorldPosition, brushBounds);
		break;
	case Brush::Erosion:
		ActionErode(dt, terrain, scalarField, centerWorldPosition, brushBounds);
		break;
	default: break;
	}
}

void ScalarFieldEditor::ActionRaiseLower(const float dt,
										 Terrain &terrain,
										 ScalarField *scalarField,
										 const glm::vec3 center,
										 const glm::ivec3 brushBounds[2])
{
	for (int x = brushBounds[0].x; x <= brushBounds[1].x; x++) {
		for (int y = brushBounds[0].y; y <= brushBounds[1].y; y++) {
			for (int z = brushBounds[0].z; z <= brushBounds[1].z; z++) {
				auto i = terrain.ScalarIndexFromGridCoord(x, y, z);
				auto &point = (*scalarField)[i];
				const float dist = glm::distance(
					center, terrain.WorldPositionFromScalarIndex(i));

				if (dist > m_Brush.m_BrushSize)
					continue;

				const float incr =
					m_Brush.GetWeightWithFalloff(dist) * dt *
					(m_Brush.m_ChosenAction == Brush::Raise ? 1.f : -1.f);

				point.scalar = std::clamp(point.scalar + incr, -1.0f, 1.0f);
			}
		}
	}
	terrain.RecalculateGradients(brushBounds[0] - glm::ivec3 {1},
								 brushBounds[1] + glm::ivec3 {1});
	terrain.MarchingCubes(brushBounds[0], brushBounds[1]);
}

void ScalarFieldEditor::ActionSmooth(float dt,
									 Terrain &terrain,
									 ScalarField *scalarField,
									 const glm::vec3 center,
									 const glm::ivec3 brushBounds[2])
{
	std::vector<float> originalScalars(scalarField->size());
	for (size_t i = 0; i < scalarField->size(); ++i)
		originalScalars[i] = (*scalarField)[i].scalar;

	for (int x = brushBounds[0].x; x <= brushBounds[1].x; x++) {
		for (int y = brushBounds[0].y; y <= brushBounds[1].y; y++) {
			for (int z = brushBounds[0].z; z <= brushBounds[1].z; z++) {
				const int i = terrain.ScalarIndexFromGridCoord(x, y, z);
				const float dist = glm::distance(
					center, terrain.WorldPositionFromScalarIndex(i));
				if (dist > m_Brush.m_BrushSize)
					continue;

				float neighborAvg = 0.0f;
				int neighborCount = 0;
				for (int dx = -1; dx <= 1; ++dx) {
					for (int dy = -1; dy <= 1; ++dy) {
						for (int dz = -1; dz <= 1; ++dz) {
							const int sampleX = std::clamp(
								x + dx, 0, terrain.GetResolution().x - 1);
							const int sampleY = std::clamp(
								y + dy, 0, terrain.GetResolution().y - 1);
							const int sampleZ = std::clamp(
								z + dz, 0, terrain.GetResolution().z - 1);
							neighborAvg += originalScalars
								[terrain.ScalarIndexFromGridCoord(
									sampleX, sampleY, sampleZ)];
							neighborCount++;
						}
					}
				}
				neighborAvg /= (float) neighborCount;

				const float currentScalar = originalScalars[i];
				const float brushWeight = m_Brush.GetWeightWithFalloff(dist);
				const float surfaceWeight =
					1.0f -
					std::min(std::abs(currentScalar - terrain.GetThreshold()),
							 1.0f);
				const float blend = std::clamp(
					brushWeight * surfaceWeight * dt * 8.0f, 0.0f, 1.0f);

				(*scalarField)[i].scalar = std::clamp(
					currentScalar + (neighborAvg - currentScalar) * blend,
					-1.0f,
					1.0f);
			}
		}
	}
	terrain.RecalculateGradients(brushBounds[0] - glm::ivec3 {1},
								 brushBounds[1] + glm::ivec3 {1});
	terrain.MarchingCubes(brushBounds[0], brushBounds[1]);
}

void ScalarFieldEditor::ActionErode(float dt,
									Terrain &terrain,
									ScalarField *scalarField,
									glm::vec3 center,
									const glm::ivec3 brushBounds[2])
{
	ErodeProps props {};
	props.dropletOriginBounds[0] =
		terrain.WorldPositionFromGridCoord(brushBounds[0]);
	props.dropletOriginBounds[1] =
		terrain.WorldPositionFromGridCoord(brushBounds[1]);
	props.dropletOriginBounds[0].y = props.dropletOriginBounds[1].y;

	ErosionSystem::GetInstance()->Erode(&terrain, props);
}

void ScalarFieldEditor::ImGuiRender(float dt)
{
	if (ImGui::TreeNode("Painter")) {
		m_Brush.ImGuiRender(dt);
		if (m_Brush.m_ChosenAction == Brush::Erosion) {
			if (ImGui::TreeNode("Erosion Settings")) {
				ErosionSettings &settings =
					ErosionSystem::GetInstance()->p_Settings;
				ImGui::DragScalar(
					"Iterations", ImGuiDataType_S64, &settings.drops, 1);
				ImGui::DragFloat("Droplet Carry Capacity Factor",
								 &settings.dropletCarryCapacityFactor,
								 0.1);
				ImGui::DragFloat("Droplet Evaporation Speed",
								 &settings.dropletEvaporationSpeed,
								 0.0001,
								 0,
								 1);
				ImGui::DragFloat("Droplet Erosion Speed",
								 &settings.dropletErosionWeight,
								 0.001);
				ImGui::DragFloat("Droplet Deposition Speed",
								 &settings.dropletDepositionWeight,
								 0.001);
				ImGui::DragFloat(
					"Droplet Inertia", &settings.dropletInertia, 0.001);
				ImGui::DragFloat("Gravity", &settings.gravity, 0.1);
				ImGui::DragInt("Max Drop Iterations", &settings.maxDropIter, 1);
				ImGui::DragFloat("Slope Fall Threshold",
								 &settings.slopeOverhangThresh,
								 .0001);
				ImGui::DragFloat(
					"Flat Slope Thresh", &settings.slopeFlatThresh, .0001);

				m_ErodeWhole = ImGui::Button("Erode Whole");
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

bool ScalarFieldEditor::OnMouseButtonPressed(
	const Engine::MouseButtonPressedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_1)
		return false;
	m_IsMouseDown = true;
	float x, y;
	e.GetMousePos(x, y);
	m_MousePos = glm::fvec2(x, y);
	return true;
}

bool ScalarFieldEditor::OnMouseButtonReleased(
	const Engine::MouseButtonReleasedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_1)
		return false;
	m_IsMouseDown = false;
	return true;
}

bool ScalarFieldEditor::OnMouseMoved(const Engine::MouseMovedEvent &e)
{
	if (!m_IsMouseDown)
		return false;
	m_MousePos = e.GetMousePosition();
	return true;
}

bool ScalarFieldEditor::OnKeyPressed(const Engine::KeyboardKeyPressedEvent &e)
{
	switch (e.GetKey()) {
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL:
		if (m_Brush.m_ChosenAction != Brush::Raise)
			break;
		m_Brush.m_ChosenAction = Brush::Lower;
		return true;
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_SHIFT:
		if (m_Brush.m_ChosenAction != Brush::Raise)
			break;
		m_Brush.m_ChosenAction = Brush::Smooth;
		return true;
	default: break;
	}

	return false;
}

bool ScalarFieldEditor::OnKeyReleased(const Engine::KeyboardKeyReleasedEvent &e)
{
	switch (e.GetKey()) {
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL:
		if (m_Brush.m_ChosenAction != Brush::Lower)
			break;
		m_Brush.m_ChosenAction = Brush::Raise;
		return true;
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_SHIFT:
		if (m_Brush.m_ChosenAction != Brush::Smooth)
			break;
		m_Brush.m_ChosenAction = Brush::Raise;
		return true;
	default: break;
	}

	return false;
}

/////////////////////////////// Layer //////////////////////////////////////////


ScalarFieldEditorLayer::ScalarFieldEditorLayer(ScalarFieldEditor *owner)
	: m_ScalarFieldEditor(owner)
{
}

bool ScalarFieldEditorLayer::OnEvent(Engine::Event &e)
{
	Engine::EventDispatcher dispatcher(e);

	if (dispatcher.Dispatch<Engine::MouseButtonPressedEvent>(BIND_EVENT_FUNC(
			ScalarFieldEditor::OnMouseButtonPressed, m_ScalarFieldEditor)))
		return true;
	if (dispatcher.Dispatch<Engine::MouseButtonReleasedEvent>(BIND_EVENT_FUNC(
			ScalarFieldEditor::OnMouseButtonReleased, m_ScalarFieldEditor)))
		return true;
	if (dispatcher.Dispatch<Engine::MouseMovedEvent>(BIND_EVENT_FUNC(
			ScalarFieldEditor::OnMouseMoved, m_ScalarFieldEditor)))
		return true;
	if (dispatcher.Dispatch<Engine::KeyboardKeyPressedEvent>(BIND_EVENT_FUNC(
			ScalarFieldEditor::OnKeyPressed, m_ScalarFieldEditor)))
		return true;
	if (dispatcher.Dispatch<Engine::KeyboardKeyReleasedEvent>(BIND_EVENT_FUNC(
			ScalarFieldEditor::OnKeyReleased, m_ScalarFieldEditor)))
		return true;
	return false;
}
