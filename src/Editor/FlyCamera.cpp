//
// Created by r6awe on 3/31/2026.
//

#include "FlyCamera.h"

#include "Engine/Events/Events.h"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/norm.inl"
#include "imgui.h"


FlyCamera::FlyCamera(float aspect, float speed /* = 5.f*/, float sensitivity
					 /* = 1*/)
	: Camera(Perspective, {0, 0, 10}, {1, 0, 0, 0}, aspect, .01, 10000, 90),
	  m_FlySpeed(speed), m_Sensitivity(sensitivity), m_FlyCameraController(this)
{
}

void FlyCamera::Update(float dt)
{
	// update position
	if (glm::length2(m_MoveDelta) < 0.01f) {
		m_MoveDelta = {0, 0, 0};
		m_Velocity = {0, 0, 0};
	} else {
		m_MoveDelta = glm::normalize(m_MoveDelta);
		m_Velocity = m_Rotation * m_MoveDelta * m_FlySpeed;
	}

	m_Velocity += m_Acceleration * m_FlySpeed * dt;
	m_Position += m_Velocity * dt;

	// update rotation
	auto yaw =
		glm::angleAxis(glm::radians(m_EulerAngles.y), glm::vec3(0, 1, 0));
	auto pitch =
		glm::angleAxis(glm::radians(m_EulerAngles.x), glm::vec3(1, 0, 0));
	auto roll =
		glm::angleAxis(glm::radians(m_EulerAngles.z), glm::vec3(0, 0, 1));

	m_Rotation = yaw * pitch * roll;
}

void FlyCamera::ImGuiExposeParameters()
{
	if (ImGui::TreeNode("Camera Settings")) {
		ImGui::DragFloat3("Position", &m_Position[0]);
		ImGui::DragFloat3("Euler Angles", &m_EulerAngles[0], 0.1f);
		ImGui::DragFloat("Camera Speed", &m_FlySpeed, 0.01f, 0.01f, 100.0f);
		ImGui::DragFloat(
			"Camera Sensitivity", &m_Sensitivity, 0.01f, 0.01f, 100.0f);
		ImGui::TreePop();
	}
}

//////////////////////////////////// Event Handler /////////////////////////////

bool FlyCamera::OnMouseButtonPressed(const Engine::MouseButtonPressedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_2)
		return false;
	glfwSetInputMode(
		e.GetWindow().GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	m_IsMouseDown = true;
	return true;
}
bool FlyCamera::OnMouseButtonReleased(const Engine::MouseButtonReleasedEvent &e)
{
	if (e.GetMouseButton() != GLFW_MOUSE_BUTTON_2)
		return false;
	glfwSetInputMode(
		e.GetWindow().GetGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	m_IsMouseDown = false;
	return true;
}
bool FlyCamera::OnMouseMoved(const Engine::MouseMovedEvent &e)
{
	if (!m_IsMouseDown)
		return false;

	const glm::vec2 delta = e.GetDelta();
	m_EulerAngles.x += glm::degrees(delta.y);
	m_EulerAngles.y -= glm::degrees(delta.x);
	m_EulerAngles.z = glm::degrees(0.0);

	return true;
}

bool FlyCamera::OnKeyPressed(const Engine::KeyboardKeyPressedEvent &e)
{
	if (!m_IsMouseDown) {
		m_MoveDelta = {0, 0, 0};
		m_Velocity = {0, 0, 0};
		return false;
	}
	switch (e.GetKey()) {
	case GLFW_KEY_W: m_MoveDelta.z -= 1; break;
	case GLFW_KEY_S: m_MoveDelta.z += 1; break;
	case GLFW_KEY_A: m_MoveDelta.x -= 1; break;
	case GLFW_KEY_D: m_MoveDelta.x += 1; break;
	case GLFW_KEY_Q: m_MoveDelta.y -= 1; break;
	case GLFW_KEY_E: m_MoveDelta.y += 1; break;
	default: return false;
	}
	return true;
}

bool FlyCamera::OnKeyReleased(const Engine::KeyboardKeyReleasedEvent &e)
{
	if (!m_IsMouseDown) {
		m_MoveDelta = {0, 0, 0};
		m_Velocity = {0, 0, 0};
		return false;
	}
	switch (e.GetKey()) {
	case GLFW_KEY_W: m_MoveDelta.z = 0; break;
	case GLFW_KEY_S: m_MoveDelta.z = 0; break;
	case GLFW_KEY_A: m_MoveDelta.x = 0; break;
	case GLFW_KEY_D: m_MoveDelta.x = 0; break;
	case GLFW_KEY_Q: m_MoveDelta.y = 0; break;
	case GLFW_KEY_E: m_MoveDelta.y = 0; break;
	default: return false;
	}
	return true;
}

////////////////////////// Layer ///////////////////////////////////////////////

FlyCameraControllerLayer::FlyCameraControllerLayer(FlyCamera *flyCamera)
	: m_FlyCamera(flyCamera)
{
}

bool FlyCameraControllerLayer::OnEvent(Engine::Event &e)
{
	Engine::EventDispatcher dispatcher(e);
	if (dispatcher.Dispatch<Engine::MouseButtonPressedEvent>(
			BIND_EVENT_FUNC(FlyCamera::OnMouseButtonPressed, m_FlyCamera)))
		return true;
	if (dispatcher.Dispatch<Engine::MouseButtonReleasedEvent>(
			BIND_EVENT_FUNC(FlyCamera::OnMouseButtonReleased, m_FlyCamera)))
		return true;
	if (dispatcher.Dispatch<Engine::MouseMovedEvent>(
			BIND_EVENT_FUNC(FlyCamera::OnMouseMoved, m_FlyCamera)))
		return true;
	if (dispatcher.Dispatch<Engine::KeyboardKeyPressedEvent>(
			BIND_EVENT_FUNC(FlyCamera::OnKeyPressed, m_FlyCamera)))
		return true;
	if (dispatcher.Dispatch<Engine::KeyboardKeyReleasedEvent>(
			BIND_EVENT_FUNC(FlyCamera::OnKeyReleased, m_FlyCamera)))
		return true;
	return false;
}
