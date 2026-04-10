//
// Created by r6awe on 4/8/2026.
//

#include "ErosionSimulationSystem.h"

#include "TerrainGenerator.h"

ErosionSimulationSystem ErosionSimulationSystem::m_Instance {};

void ErosionSimulationSystem::ErodeWhole(ScalarField *scalarField)
{
	ErodeProps props {};
	props.waterOriginBounds[0] = scalarField->begin()->position;
	props.waterOriginBounds[1] = (scalarField->end() - 1)->position;
	props.maxDist =
		glm::distance(props.waterOriginBounds[0], props.waterOriginBounds[1]);
	props.weight = m_Settings.weight;
	props.iter = m_Settings.iterations;

	for (long long i = 0; i < m_Settings.iterations; i++)
		DoOneIteration(scalarField, props);
}

void ErosionSimulationSystem::Erode(ScalarField *scalarField, ErodeProps props)
{
	for (long long i = 0; i < props.iter; i++)
		DoOneIteration(scalarField, props);
}
void ErosionSimulationSystem::DoOneIteration(ScalarField *scalarField,
											 ErodeProps props)
{
	while (true) {
		break;
	}
}