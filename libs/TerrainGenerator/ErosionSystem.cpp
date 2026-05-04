//
// Created by r6awe on 4/8/2026.
//

#include "ErosionSystem.h"

#include "TerrainGenerator.h"
#include "glm/gtx/norm.hpp"

#include <glm/gtc/random.hpp>
#include <numeric>

ErosionSystem ErosionSystem::m_Instance {};

static bool IsSurfaceAdjacent(const Terrain *terrain, const glm::ivec3 p)
{
	if (terrain->GetScalarFieldPtr()->at(terrain->ScalarIndexFromGridCoord(p)).scalar <=
		terrain->GetThreshold())
		return true;

	const glm::ivec3 offsets[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
								  {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	const glm::ivec3 resolution = terrain->GetResolution();
	for (const glm::ivec3 offset : offsets) {
		const glm::ivec3 sample = p + offset;
		if (sample.x < 0 || sample.y < 0 || sample.z < 0 ||
			sample.x >= resolution.x || sample.y >= resolution.y ||
			sample.z >= resolution.z)
			continue;

		if (terrain->GetScalarFieldPtr()
				->at(terrain->ScalarIndexFromGridCoord(sample))
				.scalar <= terrain->GetThreshold())
			return true;
	}
	return false;
}

void ErosionSystem::ErodeWhole(Terrain *terrain)
{
	ErodeProps props {};
	const glm::ivec3 interiorMin(0);
	const glm::ivec3 interiorMax(terrain->GetResolution() - 1);
	auto y = terrain->GetBounds()[1].y - terrain->GetVoxelSize().y;
	props.dropletOriginBounds[0] =
		terrain->WorldPositionFromGridCoord(interiorMin);
	props.dropletOriginBounds[1] =
		terrain->WorldPositionFromGridCoord(interiorMax);
	props.dropletOriginBounds[0].y = y;
	props.dropletOriginBounds[1].y = y;

	for (long long i = 0; i < p_Settings.drops; i++)
		OneRainDrop(terrain, props);

	terrain->RecalculateGradients();
	terrain->MarchingCubes();
}

void ErosionSystem::Erode(Terrain *terrain, ErodeProps props)
{
	for (long long i = 0; i < p_Settings.drops; i++)
		OneRainDrop(terrain, props);
	terrain->RecalculateGradients();
	terrain->MarchingCubes();
}

void ErosionSystem::OneRainDrop(Terrain *terrain, ErodeProps props)
{
	ScalarField &scalarField = *terrain->GetScalarFieldPtr();

	const glm::vec3 origin {glm::linearRand(props.dropletOriginBounds[0],
											props.dropletOriginBounds[1])

	};

	auto *sfp =
		&scalarField[terrain->ScalarIndexFromGridCoord(terrain->NearestGridCoordFromWorldPosition(origin))];
	auto *prevSfp = sfp;

	// droplet properties
	glm::vec3 curDropPos = origin;
	float velocity = 0, water = 1, soil = 0;
	glm::vec3 delta {};

	for (int i = 0; i < p_Settings.maxDropIter; i++) {
		int idx = terrain->ScalarIndexFromGridCoord(terrain->NearestGridCoordFromWorldPosition(curDropPos));
		// const bool pastMaxDist =
		// glm::distance2(origin, curDropPos) > props.maxDist * props.maxDist;

		if (idx < 0 || idx >= terrain->GetScalarFieldPtr()->size())
			break;

		auto gradient = -GetInterpolatedGradient(terrain, curDropPos);
		// if (glm::dot(gradient, {0, 1, 0}) > props.slopeFlatThresh)
		// break;

		bool isInsideTerrain = terrain->IsWorldPositionSolid(curDropPos);
		// detach drop from bottom of overhangs
		bool isOverhang =
			glm::dot(gradient, {0, -1, 0}) > p_Settings.slopeOverhangThresh;

		// dropGradient = gradient X (gradient X {0,1,0})
		auto dropGradient =
			glm::cross(gradient, glm::cross(gradient, {0, 1, 0}));
		delta = !isInsideTerrain || isOverhang
				  ? glm::vec3 {0, -terrain->GetVoxelSize().y, 0}
				  : (delta - dropGradient) * p_Settings.dropletInertia +
						dropGradient;
		auto nextDropPos = curDropPos + delta;

		prevSfp = sfp;
		sfp = &scalarField[terrain->ScalarIndexFromGridCoord(
			terrain->NearestGridCoordFromWorldPosition(nextDropPos))];

		// dont allow erosion/deposition if not touching terrain
		if (!isInsideTerrain) {
			curDropPos = nextDropPos;
			continue;
		}

		// more soil in next point
		if (sfp->scalar > prevSfp->scalar) {
			float soilDelta = (sfp->scalar - prevSfp->scalar) + 0.001f;
			soilDelta = std::min(soilDelta, soil);

			ChangeScalarFieldAtPos(terrain, curDropPos, soilDelta);
			soil -= soilDelta;
			velocity = 0;

			// not enough soil in water drop
			if (soil <= 0)
				break;
		}

		// compute transport capacity
		float ds = prevSfp->scalar - sfp->scalar;
		float capacity = std::max(ds, p_Settings.slopeFlatThresh) * velocity *
						 water * p_Settings.dropletCarryCapacityFactor;

		// erode or deposit
		float soilDelta = soil - capacity;
		if (soilDelta >= 0) {
			soilDelta *= p_Settings.dropletDepositionWeight;
			ChangeScalarFieldAtPos(terrain, curDropPos, soilDelta);
			soil -= soilDelta;
		} else {
			soilDelta *= -p_Settings.dropletErosionWeight;
			// soilDelta = std::min(soilDelta, ds * .99f);
			// soilDelta = ds * .99f;

			ChangeScalarFieldAtPos(terrain, curDropPos, -soilDelta);
			ds -= soilDelta;
			soil += soilDelta;
		}

		velocity =
			sqrt(std::max(0.f, velocity * velocity + p_Settings.gravity * ds));
		water *= 1 - p_Settings.dropletEvaporationSpeed;

		curDropPos += delta;
	}
}

glm::vec3 ErosionSystem::GetInterpolatedGradient(const Terrain *terrain,
												 glm::vec3 point)
{
	point = (point - terrain->GetBounds()[0]) / terrain->GetVoxelSize();
	glm::vec3 result {};
	const glm::ivec3 lo = glm::clamp(
		glm::ivec3(glm::floor(point)), glm::ivec3(0),
		terrain->GetResolution() - glm::ivec3(1));
	const glm::ivec3 hi = glm::clamp(
		lo + glm::ivec3(1), glm::ivec3(0),
		terrain->GetResolution() - glm::ivec3(1));
	const glm::vec3 t = point - glm::vec3(lo);

	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			for (int z = 0; z <= 1; z++) {
				const glm::ivec3 p(x ? hi.x : lo.x,
								   y ? hi.y : lo.y,
								   z ? hi.z : lo.z);
				const float weight = (x ? t.x : 1 - t.x) *
									 (y ? t.y : 1 - t.y) *
									 (z ? t.z : 1 - t.z);
				result += terrain->GetScalarFieldPtr()
							  ->at(terrain->ScalarIndexFromGridCoord(p))
							  .gradient *
						  weight;
			}
		}
	}
	return glm::length2(result) > 0 ? glm::normalize(result) : glm::vec3(0);
}

void ErosionSystem::ChangeScalarFieldAtPos(Terrain *terrain,
										   glm::vec3 pos,
										   float scalarDelta)
{
	glm::vec3 point = (pos - terrain->GetBounds()[0]) / terrain->GetVoxelSize();
	const glm::ivec3 lo = glm::clamp(
		glm::ivec3(glm::floor(point)), glm::ivec3(0),
		terrain->GetResolution() - glm::ivec3(1));
	const glm::ivec3 hi = glm::clamp(
		lo + glm::ivec3(1), glm::ivec3(0),
		terrain->GetResolution() - glm::ivec3(1));
	const glm::vec3 t = point - glm::vec3(lo);

	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			for (int z = 0; z <= 1; z++) {
				const glm::ivec3 p(x ? hi.x : lo.x,
								   y ? hi.y : lo.y,
								   z ? hi.z : lo.z);
				const float weight = (x ? t.x : 1 - t.x) *
									 (y ? t.y : 1 - t.y) *
									 (z ? t.z : 1 - t.z);
				ScalarFieldPoint &sfp =
					terrain->GetScalarFieldPtr()->at(terrain->ScalarIndexFromGridCoord(p));
				if (scalarDelta < 0 && !IsSurfaceAdjacent(terrain, p))
					continue;
				sfp.scalar = std::clamp(
					sfp.scalar + scalarDelta * weight, -1.f, 1.f);
			}
		}
	}
}
