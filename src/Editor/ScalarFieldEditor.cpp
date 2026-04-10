//
// Created by r6awe on 4/6/2026.
//

#include "ScalarFieldEditor.h"

#include "TerrainGenerator/TerrainGenerator.h"

#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Events/MouseEvents.h"
#include "TerrainGenerator/ErosionSimulationSystem.h"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

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

void Brush::ImGuiExposeParameters()
{
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
}

/////////////////////////// Scalar Field Editor ////////////////////////////////

ScalarFieldEditor::ScalarFieldEditor()
	: m_Layer(this), m_Brush(1, .1, 10, Brush::Linear, Brush::Both)
{
}

void ScalarFieldEditor::UpdateScalarFieldIfMouseDown(const float dt,
													 Terrain &terrain,
													 const glm::mat4 &view,
													 const glm::mat4 &proj)
{
	ScalarField *scalarField = terrain.GetScalarFieldPtr();
	if (m_ErodeWhole) {
		auto props =
			ErosionSimulationSystem::GetInstance()->GetErosionSettings();
		props.iterations = m_ErosionIterations;
		props.weight = m_ErosionWeight;
		ErosionSimulationSystem::GetInstance()->ErodeWhole(scalarField);
		return;
	}
	if (!m_IsMouseDown)
		return;

	const int nearestSFPIndex = terrain.GetNearestScalarFieldPointIndex(
		m_MousePos, m_WindowSize, view, proj, m_Brush.m_MaxDistance);
	if (nearestSFPIndex == -1) {
		std::cout << "not near anything" << std::endl;
		return;
	}

	const ScalarFieldPoint center = (*scalarField)[nearestSFPIndex];
	const glm::vec3 brushBounds[2] = {
		terrain.GetNearestGridPoint(center.position -
									glm::vec3(m_Brush.m_BrushSize / 2)),
		terrain.GetNearestGridPoint(center.position +
									glm::vec3(m_Brush.m_BrushSize / 2))};
	switch (m_Brush.m_ChosenAction) {
	case Brush::Raise:
	case Brush::Lower:
		ActionRaiseLower(
			dt, terrain, scalarField, center.position, brushBounds);
		break;
	case Brush::Smooth:
		ActionSmooth(dt, terrain, scalarField, center.position, brushBounds);
		break;
	case Brush::Erosion:
		ActionErode(dt, terrain, scalarField, center.position, brushBounds);
		break;
	default: break;
	}
}

void ScalarFieldEditor::ActionRaiseLower(const float dt,
										 Terrain &terrain,
										 ScalarField *scalarField,
										 const glm::vec3 center,
										 const glm::vec3 brushBounds[2])
{
	for (int x = brushBounds[0].x; x <= brushBounds[1].x; x++) {
		for (int y = brushBounds[0].y; y <= brushBounds[1].y; y++) {
			for (int z = brushBounds[0].z; z <= brushBounds[1].z; z++) {
				auto &point = (*scalarField)[terrain.GetIndex(x, y, z)];
				const float dist = glm::distance2(center, point.position);

				if (dist > m_Brush.m_BrushSize * m_Brush.m_BrushSize)
					continue;

				const float incr =
					-m_Brush.GetWeightWithFalloff(dist) * dt *
					(m_Brush.m_ChosenAction == Brush::Raise ? 1 : -1);

				point.scalar = std::clamp(point.scalar + incr, 0.0f, 1.0f);
			}
		}
	}
	terrain.MarchingCubes();
}

void ScalarFieldEditor::ActionSmooth(float dt,
									 Terrain &terrain,
									 const ScalarField *scalarField,
									 const glm::vec3 center,
									 const glm::vec3 brushBounds[2])
{
	float avgScalar = 0;
	int numPoints = 0;
	for (int x = brushBounds[0].x; x <= brushBounds[1].x; x++) {
		for (int y = brushBounds[0].y; y <= brushBounds[1].y; y++) {
			for (int z = brushBounds[0].z; z <= brushBounds[1].z; z++) {
				const auto p = (*scalarField)[terrain.GetIndex(x, y, z)];
				avgScalar += p.scalar;
				numPoints++;
			}
		}
	}
	avgScalar /= numPoints;

	// lerp points to avgScalar by brush weight with falloff
	for (int x = brushBounds[0].x; x <= brushBounds[1].x; x++) {
		for (int y = brushBounds[0].y; y <= brushBounds[1].y; y++) {
			for (int z = brushBounds[0].z; z <= brushBounds[1].z; z++) {
				auto p = (*scalarField)[terrain.GetIndex(x, y, z)];
				const auto w = m_Brush.GetWeightWithFalloff(
					glm::distance(center, p.position));
				p.scalar = p.scalar * (1 - w) + avgScalar * w;
			}
		}
	}
	terrain.MarchingCubes();
}

void ScalarFieldEditor::ActionErode(float dt,
									Terrain &terrain,
									ScalarField *scalarField,
									glm::vec3 center,
									const glm::vec3 brushBounds[2])
{
	ErodeProps props;
	props.maxDist = m_Brush.m_BrushSize;
	props.waterOriginBounds[0] = brushBounds[0];
	props.waterOriginBounds[1] = brushBounds[1];
	props.weight = m_ErosionWeight;
	props.iter = m_ErosionIterations;

	ErosionSimulationSystem::GetInstance()->Erode(scalarField, props);
}

void ScalarFieldEditor::ImGuiExposeParameters()
{
	if (ImGui::TreeNode("Terrain Editor Settings")) {
		// ImGui::TreePush("Terrain Editor Settings");
		if (ImGui::TreeNode("Brush Settings")) {
			m_Brush.ImGuiExposeParameters();
			ImGui::TreePop();
		}

		if (m_Brush.m_ChosenAction == Brush::Erosion) {
			if (ImGui::TreeNode("Erosion Settings")) {
				ImGui::DragScalar(
					"Iterations", ImGuiDataType_S64, &m_ErosionIterations, 1);
				ImGui::DragFloat("Weight", &m_ErosionWeight, 0.001, 0, 1);
				m_ErodeWhole = ImGui::Button("Erode Whole");
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

bool ScalarFieldEditor::OnMouseButtonPressed(Engine::MouseButtonPressedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_1)
		return false;
	m_IsMouseDown = true;
	int x, y;
	e.GetMousePixelPos(x, y);
	m_MousePos = glm::fvec2(x, y);

	m_WindowSize = e.GetWindow().GetWindowSize();
	return true;
}

bool
ScalarFieldEditor::OnMouseButtonReleased(Engine::MouseButtonReleasedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_1)
		return false;
	m_IsMouseDown = false;
	return true;
}

bool ScalarFieldEditor::OnMouseMoved(Engine::MouseMovedEvent &e)
{
	if (!m_IsMouseDown)
		return false;
	int x, y;
	e.GetMousePixelPos(x, y);
	m_MousePos = glm::fvec2(x, y);

	m_WindowSize = e.GetWindow().GetWindowSize();
	return true;
}

bool ScalarFieldEditor::OnKeyPressed(Engine::KeyboardKeyPressedEvent &e)
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

bool ScalarFieldEditor::OnKeyReleased(Engine::KeyboardKeyReleasedEvent &e)
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