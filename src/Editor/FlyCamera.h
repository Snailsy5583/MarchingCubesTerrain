//
// Created by r6awe on 3/31/2026.
//

#ifndef TERRAINGENERATIONEDITOR_CAMERACONTROLLER_H
#define TERRAINGENERATIONEDITOR_CAMERACONTROLLER_H

#include "Engine/Camera.h"
#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Events/MouseEvents.h"
#include "Engine/Layer.h"

class FlyCamera;

class FlyCameraControllerLayer : public Engine::Layer
{
public:
	FlyCameraControllerLayer(FlyCamera *flyCamera);

	void OnAttach() override {}
	void OnDetach() override {}
	bool OnEvent(Engine::Event &e) override;

private:
	FlyCamera *m_FlyCamera;
};

class FlyCamera : public Engine::Camera
{
	friend class FlyCameraControllerLayer;

public:
	FlyCamera(float aspect, float speed = 5, float sensitivity = 1);

	void Update(float dt);

	void ImGuiExposeParameters();

public:	   // Event Management
	FlyCameraControllerLayer *GetCameraControllerLayer()
	{ return &m_FlyCameraController; }

	bool OnMouseButtonPressed(const Engine::MouseButtonPressedEvent &e);
	bool OnMouseButtonReleased(const Engine::MouseButtonReleasedEvent &e);
	bool OnMouseMoved(const Engine::MouseMovedEvent &e);
	bool OnKeyPressed(const Engine::KeyboardKeyPressedEvent &e);
	bool OnKeyReleased(const Engine::KeyboardKeyReleasedEvent &e);

private:
	glm::vec3 m_EulerAngles {0, 0, 0};
	glm::vec3 m_MoveDelta, m_Acceleration, m_Velocity;

	float m_FlySpeed, m_Sensitivity;

	FlyCameraControllerLayer m_FlyCameraController;

private:
	bool m_IsMouseDown = false;
};


#endif	  // TERRAINGENERATIONEDITOR_CAMERACONTROLLER_H
