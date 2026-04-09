//
// Created by r6awe on 4/8/2026.
//

#ifndef TERRAINGENERATIONEDITOR_EROSION_H
#define TERRAINGENERATIONEDITOR_EROSION_H
#include "glm/vec3.hpp"

struct ErodeProps {
	glm::vec3 start;
	float startRadius;
	float maxDist;
	float weight;

	int iter;
};

class Erosion
{
public:
	static void ErodeWhole(int iter, float weight);

	static void Erode(ErodeProps props);

	static void DoOneIteration(ErodeProps props);

private:
};


#endif	  // TERRAINGENERATIONEDITOR_EROSION_H
