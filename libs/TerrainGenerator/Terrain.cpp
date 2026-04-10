//
// Created by r6awe on 3/30/2026.
//

#include "Terrain.h"

#include "TerrainGenerator.h"
#include "glm/gtx/norm.inl"

#include <map>

Terrain::Terrain(glm::vec3 size,
				 int resolution,
				 ScalarField &scalarField,
				 float threshold)
	: GameObject(Engine::Mesh(), {0, 0, 0}, {1, 0, 0, 0}),
	  m_Resolution(resolution - 1), m_Bounds {-size / 2.f, size / 2.f},
	  m_Size(size), m_Threshold(threshold), m_ScalarField {scalarField}
{
	mesh.usage = Engine::Mesh::Dynamic;
}

int Terrain::calculate_cube_index(std::vector<int> &cell, float isovalue)
{
    int cubeIndex = 0;
    for (int i = 0; i < 8; i++)
        if (m_ScalarField[cell[i]].scalar < isovalue) cubeIndex |= (1 << i);
    return cubeIndex;
}


std::vector<glm::vec3> Terrain::get_intersection_points(std::vector<int> &cell,
								  float isovalue)
{
    std::vector<glm::vec3> intersections (12);

    int cubeIndex = calculate_cube_index(cell, isovalue);
    int intersectionsKey = edgeTable[cubeIndex];

    int idx = 0;
    while (intersectionsKey)
    {
        if (intersectionsKey&1)
        {


        	int v1 = edgeToVertices[idx].first, v2 = edgeToVertices[idx].second;
        	ScalarFieldPoint p1 = m_ScalarField[cell[v1]], p2 = m_ScalarField[cell[v2]];

        	glm::vec3 intersectionPoint = interpolate(p1.position, p1.scalar,
													p2.position, p2.scalar, isovalue);
        	intersections[idx] = intersectionPoint;
        }
        idx++;
        intersectionsKey >>= 1;
    }


    return intersections;
}

glm::vec3 Terrain::interpolate(glm::vec3& v1, float val1, glm::vec3& v2, float val2, float isovalue)
{
    glm::vec3 interpolated;
    float mu = (isovalue - val1) / (val2 - val1);

    interpolated.x = mu*(v2.x - v1.x) + v1.x;
    interpolated.y = mu*(v2.y - v1.y) + v1.y;
    interpolated.z = mu*(v2.z - v1.z) + v1.z;

	auto vertex = v1;
	/*std::cout << vertex.x << ", " << vertex.y << ", " << vertex.z   << std::endl;
	 vertex = v2;
	std::cout << vertex.x << ", " << vertex.y << ", " << vertex.z   << std::endl;
	std::cout << val1 << ", " << val2 << ", " << isovalue << " ||| " << mu << std::endl;
	vertex = interpolated;
	std::cout << vertex.x << ", " << vertex.y << ", " << vertex.z   << std::endl;
	std::cout << std::endl<< std::endl<< std::endl << std::endl;*/
    return interpolated;
}

void Terrain::MarchingCubes()
{
	glm::vec3 max = glm::vec3(m_Resolution);
	std::vector<unsigned int> indices;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;

	for (int i = 0; i + 1 < max.x; i++) {
		for (int j = 0; j + 1 < max.y; j++)
		{
			for (int k = 0; k + 1 < max.z; k++)
			{
				float x = i, y = j, z = k;
				// cell ordered according to convention in referenced website
				int index = GetIndex(x,y,z);
				std::vector<int> cell = {
					index, index + m_Resolution * m_Resolution,
					index + m_Resolution * m_Resolution + 1, index + 1,
					index + m_Resolution, index + m_Resolution * m_Resolution + m_Resolution,
					index + m_Resolution * m_Resolution + m_Resolution + 1, index + m_Resolution + 1
				};

				// Get the indices in the vertices vector of the vertices we've already created from previous cells


				int cubeIndex = calculate_cube_index(cell, m_Threshold);
				std::vector<glm::vec3> intersections = get_intersection_points(
					cell, m_Threshold);

				std::vector<int> intersectionIndices = {-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1};

				for (int idx = 0; triangleTable[cubeIndex][idx] != -1; idx++) {
					int vertexId = triangleTable[cubeIndex][idx];
					if (intersectionIndices[vertexId] == -1) {
						vertices.push_back(intersections[vertexId]);
						normals.emplace_back(0,0,1);
						texCoords.emplace_back(0,0);
						intersectionIndices[vertexId] = vertices.size()-1;
					}
					indices.push_back(intersectionIndices[vertexId]);

				}

			}
		}
	}

	mesh.vertices = vertices;
	//for (auto vertex : vertices) std::cout << vertex.x << ", " << vertex.y << ", " << vertex.z   << std::endl;
	mesh.normals = normals;
	mesh.texCoords = texCoords;
	mesh.indices = indices;
	mesh.UpdateMesh();
}

glm::vec3 rayCast(double xpos,
				  double ypos,
				  glm::mat4 view,
				  glm::mat4 projection,
				  unsigned SCR_WIDTH,
				  unsigned SCR_HEIGHT)
{
	using namespace glm;
	float x = (2.0f * xpos) / SCR_WIDTH - 1.0f;
	float y = 1.0f - (2.0f * ypos) / SCR_HEIGHT;
	float z = 1.0f;
	vec3 ray_nds = vec3(x, y, z);
	// Change this part
	vec4 ray_clip = vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
	vec4 ray_eye = inverse(projection) * ray_clip;
	// And this part
	ray_eye = vec4(ray_eye.x, ray_eye.y, ray_eye.z, 0.0f);
	vec4 inv_ray_wor = (inverse(view) * ray_eye);
	vec3 ray_wor = vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
	ray_wor = normalize(ray_wor);
	return ray_wor;
}

int Terrain::GetNearestScalarFieldPointIndex(glm::vec2 mousePos,
											 glm::vec2 windowSize,
											 glm::mat4 view,
											 glm::mat4 proj,
											 float maxDist) const
{
	const glm::vec3 pos = view[3];
	// glm::unProject(glm::vec3(mousePos.x, mousePos.y, 0.0),
	// view,
	// proj,
	// glm::vec4(0, 0, windowSize.x, windowSize.y));
	const glm::vec3 dir =
		rayCast(mousePos.x, mousePos.y, view, proj, windowSize.x, windowSize.y);

	glm::vec3 vs = GetVoxelSize();
	glm::ivec3 step = glm::sign(dir);
	glm::ivec3 gridPoint = GetNearestGridPoint(pos + dir);

	glm::vec3 gridPos = m_ScalarField[GetIndex(gridPoint)].position;
	// ReSharper disable once CppTooWideScopeInitStatement
	glm::vec3 curPos = pos + glm::distance(pos, (glm::vec3) gridPos) * dir;

	if (glm::distance2(gridPos, curPos) > maxDist * maxDist) {
		std::cout << glm::to_string(gridPos) << glm::to_string(curPos);
		return -1;
	}

	// DDA
	glm::vec3 nextBoundary;
	nextBoundary.x =
		m_Bounds[0].x + (gridPoint.x + (step.x > 0 ? 1.f : 0.f)) * vs.x;
	nextBoundary.y =
		m_Bounds[0].y + (gridPoint.y + (step.y > 0 ? 1.f : 0.f)) * vs.y;
	nextBoundary.z =
		m_Bounds[0].z + (gridPoint.z + (step.z > 0 ? 1.f : 0.f)) * vs.z;
	glm::vec3 tDelta = glm::abs(vs / dir);
	glm::vec3 tMax = (nextBoundary - pos) / dir;

	while (true) {
		int i = GetIndex(gridPoint.x, gridPoint.y, gridPoint.z);
		if (i < 0 || i > m_ScalarField.size())
			break;

		if (auto point = m_ScalarField[i]; point.scalar < m_Threshold)
			return i;

		if (tMax.x < tMax.y && tMax.x < tMax.z) {
			gridPoint.x += step.x;
			tMax.x += tDelta.x;
		} else if (tMax.y < tMax.z) {
			gridPoint.y += step.y;
			tMax.y += tDelta.y;
		} else {
			gridPoint.z += step.z;
			tMax.z += tDelta.z;
		}
	}

	// if there is nothing under the threshold (ground)
	// then return something maxDist in front of the click
	return GetIndex(GetNearestGridPoint(pos + dir * maxDist));
}
