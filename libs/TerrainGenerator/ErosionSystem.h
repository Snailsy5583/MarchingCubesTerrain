//
// Created by r6awe on 4/8/2026.
//

#ifndef TERRAINGENERATIONEDITOR_EROSION_H
#define TERRAINGENERATIONEDITOR_EROSION_H
#include "Terrain.h"

struct ErodeProps {
	glm::vec3 dropletOriginBounds[2];
};

struct ErosionSettings {
	long long drops = 100;

	float dropletCarryCapacityFactor = 10;
	float dropletEvaporationSpeed = 0.001f;
	float dropletErosionWeight = 0.9f;
	float dropletDepositionWeight = 0.02f;
	float dropletInertia = 0.1f;
	float gravity = 20;

	int maxDropIter = 1000;
	float slopeOverhangThresh = .2f, slopeFlatThresh = .05f;
};

class ErosionSystem
{
public:
	ErosionSettings p_Settings {};

	static ErosionSystem *GetInstance() { return &m_Instance; }

public:
	void ErodeWhole(Terrain *terrain);

	void Erode(Terrain *terrain, ErodeProps props);

	void OneRainDrop(Terrain *terrain, ErodeProps props);

private:
	glm::vec3 GetInterpolatedGradient(const Terrain *terrain, glm::vec3 point);

	void
	ChangeScalarFieldAtPos(Terrain *terrain, glm::vec3 pos, float scalarDelta);

private:
	static ErosionSystem m_Instance;
};


#endif	  // TERRAINGENERATIONEDITOR_EROSION_H
