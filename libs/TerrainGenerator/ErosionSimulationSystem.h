//
// Created by r6awe on 4/8/2026.
//

#ifndef TERRAINGENERATIONEDITOR_EROSION_H
#define TERRAINGENERATIONEDITOR_EROSION_H
#include "Terrain.h"
#include "glm/vec3.hpp"

struct ErodeProps {
	glm::vec3 waterOriginBounds[2];
	float maxDist;
	float weight;

	long long drops;
	long long iter;
};

struct ErosionSettings {
	float weight;
	long long drops;
	long long iterations;
};

class ErosionSimulationSystem
{
public:
	static ErosionSimulationSystem *GetInstance() { return &m_Instance; }

	ErosionSettings GetErosionSettings() const { return m_Settings; }
	void SetErosionSettings(const ErosionSettings settings)
	{ m_Settings = settings; };

public:
	void ErodeWhole(ScalarField *scalarField);

	void Erode(ScalarField *scalarField, ErodeProps props);

	void DoOneIteration(ScalarField *scalarField, ErodeProps props);

private:
	static ErosionSimulationSystem m_Instance;

	ErosionSettings m_Settings {};
};


#endif	  // TERRAINGENERATIONEDITOR_EROSION_H
