//
// Created by r6awe on 4/6/2026.
//

#ifndef TERRAINGENERATIONEDITOR_SCALARFIELDEDITOR_H
#define TERRAINGENERATIONEDITOR_SCALARFIELDEDITOR_H

#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Events/MouseEvents.h"
#include "Engine/Layer.h"
#include "TerrainGenerator/Terrain.h"
#include "tinyexpr.h"

class ScalarFieldEditor;

class Brush
{
	friend ScalarFieldEditor;

public:
	enum FalloffType { Custom = 0, Constant, Linear, Quadratic, Cubic, Sine };

	enum EasingType { In, Out, Both };

	enum Action { Raise = 0, Lower, Smooth, Erosion };

	Brush(float size,
		  float weight,
		  float maxDist,
		  FalloffType type,
		  EasingType easeType);

	float GetWeightWithFalloff(float dist);

	void ImGuiExposeParameters();

private:
	void CompileCustomExpr()
	{
		int error;
		m_CustomExpr =
			te_compile(m_CustomExprStr.c_str(), m_ExprVars, 2, &error);
		if (error)
			std::cout << "FORMULA COMPILE ERROR: " << error << std::endl;
	}

private:
	float CustomFalloff(float x)
	{
		if (m_CustomExpr == nullptr)
			return ConstantFalloff(x);
		m_CustomVarX = x;
		return te_eval(m_CustomExpr);
	}
	float ConstantFalloff(float x) const { return x < m_Falloff ? 1 : 0; }
	float LinearFalloff(float x) const
	{ return std::max(1 - x / m_BrushSize, 0.f); }
	float QuadraticFalloff(float x) const
	{
		float negx = 1 - x;
		switch (m_EasingType) {
		case In: return x * x;
		case Out: return 1 - negx * negx;
		default:
			return x < .5 ? 2 * x * x : 1 - (-2 * x + 2) * (-2 * x + 2) / 2;
		}
	}
	float CubicFalloff(float x) const
	{
		switch (m_EasingType) {
		case In: return x * x * x;
		case Out: return 1 - x * x * x;
		case Both: return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
		}
		return std::max(1 - x / m_BrushSize, 0.f);
	}
	float SineFalloff(float x) const
	{
		auto pi = glm::pi<float>();
		switch (m_EasingType) {
		case In: return 1 - std::cos(x * pi / 2);
		case Out: return std::sin(x * pi / 2);
		default: return -(std::cos(x * pi) - 1) / 2;
		}
	}


private:
	float m_BrushSize, m_Weight, m_MaxDistance;
	FalloffType m_Falloff;
	EasingType m_EasingType;
	Action m_ChosenAction = Raise;

	float m_CustomVarX;
	std::string m_CustomExprStr;
	te_variable m_ExprVars[2];
	te_expr *m_CustomExpr = nullptr;
};

class ScalarFieldEditorLayer : public Engine::Layer
{
public:
	ScalarFieldEditorLayer(ScalarFieldEditor *owner);
	void OnAttach() override { }
	void OnDetach() override { }
	bool OnEvent(Engine::Event &e) override;

private:
	ScalarFieldEditor *m_ScalarFieldEditor;
};

class ScalarFieldEditor
{
	friend ScalarFieldEditorLayer;

public:
	ScalarFieldEditor();

	void ImGuiExposeParameters();
	void UpdateScalarFieldIfMouseDown(float dt,
									  Terrain &terrain,
									  const glm::mat4 &view,
									  const glm::mat4 &proj);

private:
	void ActionRaiseLower(float dt,
						  const Terrain &terrain,
						  ScalarField *scalarField,
						  glm::vec3 center,
						  const glm::vec3 brushBounds[2]);
	void ActionSmooth(float dt,
					  const Terrain &terrain,
					  const ScalarField *scalarField,
					  glm::vec3 center,
					  const glm::vec3 brushBounds[2]);
	void ActionErode(float dt,
					 Terrain &terrain,
					 ScalarField *scalarField,
					 glm::vec3 center,
					 const glm::vec3 brushBounds[2]);

public:
	ScalarFieldEditorLayer *GetLayer() { return &m_Layer; }

private:
	// Events
	bool OnMouseButtonPressed(Engine::MouseButtonPressedEvent &e);
	bool OnMouseButtonReleased(Engine::MouseButtonReleasedEvent &e);
	bool OnMouseMoved(Engine::MouseMovedEvent &e);
	bool OnKeyPressed(Engine::KeyboardKeyPressedEvent &e);
	bool OnKeyReleased(Engine::KeyboardKeyReleasedEvent &e);


private:
	ScalarFieldEditorLayer m_Layer;
	bool m_IsMouseDown = false;
	glm::vec2 m_MousePos {}, m_WindowSize {};

	Brush m_Brush;
};


#endif	  // TERRAINGENERATIONEDITOR_SCALARFIELDEDITOR_H
