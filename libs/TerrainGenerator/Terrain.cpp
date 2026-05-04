//
// Created by r6awe on 3/30/2026.
//

#include "Terrain.h"

#include "Engine/Renderer.h"
#include "TerrainGenerator.h"
#include "glm/gtx/norm.inl"
#include "imgui.h"

#include <algorithm>
#include <limits>
#include <map>
#include <mutex>
#include <thread>

/// edgeToVertices[i] = {a, b} => edge i joins vertices a and b
int edgeToVertices[12][2] = {{0, 1},
							 {1, 2},
							 {2, 3},
							 {0, 3},
							 {4, 5},
							 {5, 6},
							 {6, 7},
							 {4, 7},
							 {0, 4},
							 {1, 5},
							 {2, 6},
							 {3, 7}};

/// edgeTable[i] is a 12 bit number; i is a cubeIndex
/// edgeTable[i][j] = 1 if isosurface intersects edge j for cubeIndex i
const int edgeTable[256] = {
	0x0,   0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f,
	0xb06, 0xc0a, 0xd03, 0xe09, 0xf00, 0x190, 0x99,	 0x393, 0x29a, 0x596, 0x49f,
	0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 0x230,
	0x339, 0x33,  0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936,
	0xe3a, 0xf33, 0xc39, 0xd30, 0x3a0, 0x2a9, 0x1a3, 0xaa,	0x7a6, 0x6af, 0x5a5,
	0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 0x460, 0x569,
	0x663, 0x76a, 0x66,	 0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a,
	0x963, 0xa69, 0xb60, 0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff,  0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 0x650, 0x759, 0x453,
	0x55a, 0x256, 0x35f, 0x55,	0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53,
	0x859, 0x950, 0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,  0xfcc,
	0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 0x8c0, 0x9c9, 0xac3, 0xbca,
	0xcc6, 0xdcf, 0xec5, 0xfcc, 0xcc,  0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9,
	0x7c0, 0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x55,
	0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650, 0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6,
	0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0xff,  0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f,
	0x66,  0x76a, 0x663, 0x569, 0x460, 0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af,
	0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa,	 0x1a3, 0x2a9, 0x3a0, 0xd30,
	0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636,
	0x13a, 0x33,  0x339, 0x230, 0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895,
	0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99,	0x190, 0xf00, 0xe09,
	0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a,
	0x203, 0x109, 0x0};

/// triangleTable[i] is a list of edges forming triangles for cubeIndex i
const int triangleTable[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

TerrainChunk::TerrainChunk(glm::ivec3 minCell,
						   glm::ivec3 maxCell,
						   ScalarField &scalarField,
						   int id,
						   Engine::Shader *shader)
	: m_Mesh(std::make_unique<Engine::Mesh>(shader, Engine::Mesh::Dynamic)),
	  m_MinCell(minCell), m_MaxCell(maxCell), m_ScalarField(scalarField),
	  m_ID(id)
{
}

void TerrainChunk::Rebuild(const Terrain &terrain)
{
	RebuildGeometry(terrain);
	UpdateMesh();
}

void TerrainChunk::RebuildGeometry(const Terrain &terrain)
{
	m_Mesh->p_Vertices.clear();
	m_Mesh->p_Indices.clear();

	for (int i = m_MinCell.x; i < m_MaxCell.x; i++)
		for (int j = m_MinCell.y; j < m_MaxCell.y; j++)
			for (int k = m_MinCell.z; k < m_MaxCell.z; k++)
				terrain.AppendCellTriangles(
					i, j, k, m_Mesh->p_Vertices, m_Mesh->p_Indices);
}

void TerrainChunk::UpdateMesh()
{
	m_Mesh->UpdateMesh();
}

void TerrainChunk::Render() const
{
	m_Mesh->p_RenderObject.shader->SetUniformVec("pos", glm::vec3(0.0f));
	Engine::Renderer::SubmitObject(m_Mesh.get());
}

void TerrainChunk::SetShader(Engine::Shader *shader)
{ m_Mesh->GenRendererObj(shader); }

Terrain::Terrain(const glm::vec3 size,
				 const glm::ivec3 resolution,
				 ScalarField &scalarField,
				 const float threshold,
				 Engine::Shader *shader)
	: m_ScalarField {scalarField}, m_Resolution(resolution), m_Size(size),
	  m_Bounds {-size / 2.f, size / 2.f}, m_Shader(shader),
	  m_Threshold(threshold)
{
}

void Terrain::ImGuiRender(float dt)
{
	if (ImGui::TreeNode("Terrain Mesh")) {
		ImGui::DragFloat3("Scale", &m_Size[0]);
		ImGui::DragInt3("Resolution", &m_Resolution[0]);
		ImGui::DragInt3("Chunk Size", &m_ChunkCellSize[0]);
		ImGui::DragFloat("Threshold", &m_Threshold, 0.001f, -1.0f, 1.0f);
		ImGui::TreePop();
	}
}

int Terrain::calculate_cube_index(const std::vector<int> &cell,
								  const float isovalue) const
{
	int cubeIndex = 0;
	for (int i = 0; i < 8; i++) {
		if (GetScalarValueForMeshing(cell[i], isovalue) > isovalue)
			cubeIndex |= (1 << i);
	}
	return cubeIndex;
}

bool Terrain::IsBoundaryScalarFieldPoint(const int index) const
{
	const glm::ivec3 p = GridCoordFromScalarIndex(index);
	return p.x == 0 || p.y == 0 || p.z == 0 || p.x == m_Resolution.x - 1 ||
		   p.y == m_Resolution.y - 1 || p.z == m_Resolution.z - 1;
}

float Terrain::GetScalarValueForMeshing(const int index,
										const float isovalue) const
{
	if (IsBoundaryScalarFieldPoint(index))
		return isovalue - 1.0f;
	return m_ScalarField[index].scalar;
}


std::vector<glm::vec3>
Terrain::get_intersection_points(const std::vector<int> &cell,
								 const float isovalue) const
{
	std::vector<glm::vec3> intersections(12);

	const int cubeIndex = calculate_cube_index(cell, isovalue);
	int intersectionsKey = edgeTable[cubeIndex];

	int idx = 0;
	while (intersectionsKey) {
		if (intersectionsKey & 1) {
			const int v1 = edgeToVertices[idx][0], v2 = edgeToVertices[idx][1];
			const float scalar1 = GetScalarValueForMeshing(cell[v1], isovalue);
			const float scalar2 = GetScalarValueForMeshing(cell[v2], isovalue);
			auto worldPosition1 = WorldPositionFromScalarIndex(cell[v1]);
			auto worldPosition2 = WorldPositionFromScalarIndex(cell[v2]);

			const glm::vec3 intersectionPoint = interpolate(
				worldPosition1, scalar1, worldPosition2, scalar2, isovalue);
			intersections[idx] = intersectionPoint;
		}
		idx++;
		intersectionsKey >>= 1;
	}


	return intersections;
}

std::vector<glm::vec3>
Terrain::get_intersection_normals(const std::vector<int> &cell,
								  const float isovalue) const
{
	std::vector<glm::vec3> normals(12, glm::vec3(0.0f));

	const int cubeIndex = calculate_cube_index(cell, isovalue);
	int intersectionsKey = edgeTable[cubeIndex];

	int idx = 0;
	while (intersectionsKey) {
		if (intersectionsKey & 1) {
			const int v1 = edgeToVertices[idx][0], v2 = edgeToVertices[idx][1];
			const float scalar1 = GetScalarValueForMeshing(cell[v1], isovalue);
			const float scalar2 = GetScalarValueForMeshing(cell[v2], isovalue);
			const glm::vec3 gradient1 = m_ScalarField[cell[v1]].gradient;
			const glm::vec3 gradient2 = m_ScalarField[cell[v2]].gradient;

			const glm::vec3 gradient =
				interpolate(gradient1, scalar1, gradient2, scalar2, isovalue);
			if (glm::length2(gradient) > 0.0f)
				normals[idx] = glm::normalize(-gradient);
		}
		idx++;
		intersectionsKey >>= 1;
	}

	return normals;
}

glm::vec3 Terrain::interpolate(const glm::vec3 &v1,
							   const float val1,
							   const glm::vec3 &v2,
							   const float val2,
							   const float isovalue)
{
	glm::vec3 interpolated;
	const float mu = (isovalue - val1) / (val2 - val1);

	interpolated.x = mu * (v2.x - v1.x) + v1.x;
	interpolated.y = mu * (v2.y - v1.y) + v1.y;
	interpolated.z = mu * (v2.z - v1.z) + v1.z;

	return interpolated;
}

void Terrain::MarchingCubes() const
{
	const unsigned int hardwareThreadCount =
		std::thread::hardware_concurrency();
	const size_t threadCount = std::min<size_t>(
		hardwareThreadCount == 0 ? 1 : hardwareThreadCount, m_Chunks.size());

	if (threadCount == 0)
		return;

	std::vector<std::thread> threads;
	threads.reserve(threadCount);
	const size_t chunksPerThread =
		(m_Chunks.size() + threadCount - 1) / threadCount;

	for (size_t i = 0; i < threadCount; i++) {
		const size_t start = i * chunksPerThread;
		const size_t end = std::min(start + chunksPerThread, m_Chunks.size());
		threads.emplace_back([&, start, end] {
			for (size_t c = start; c < end; c++)
				m_Chunks[c]->RebuildGeometry(*this);
		});
	}

	for (auto &thread : threads)
		thread.join();

	for (const auto &chunk : m_Chunks)
		chunk->UpdateMesh();

	Engine::glCheckError();
}

void Terrain::MarchingCubes(glm::ivec3 dirtyMin, glm::ivec3 dirtyMax) const
{
	dirtyMin = glm::clamp(
		dirtyMin - glm::ivec3(1), glm::ivec3(0), m_Resolution - glm::ivec3(2));
	dirtyMax =
		glm::clamp(dirtyMax, glm::ivec3(0), m_Resolution - glm::ivec3(2));

	const glm::ivec3 chunkMin = dirtyMin / m_ChunkCellSize;
	const glm::ivec3 chunkMax = dirtyMax / m_ChunkCellSize;

	for (int x = chunkMin.x; x <= chunkMax.x; x++)
		for (int y = chunkMin.y; y <= chunkMax.y; y++)
			for (int z = chunkMin.z; z <= chunkMax.z; z++)
				m_Chunks[GetChunkIndex({x, y, z})]->Rebuild(*this);

	Engine::glCheckError();
}

void Terrain::AppendCellTriangles(const int i,
								  const int j,
								  const int k,
								  std::vector<Engine::Vertex> &vertices,
								  std::vector<unsigned int> &indices) const
{
	// cell ordered according to convention in referenced website
	const auto cell = {ScalarIndexFromGridCoord(i, j, k),
					   ScalarIndexFromGridCoord(i + 1, j, k),
					   ScalarIndexFromGridCoord(i + 1, j, k + 1),
					   ScalarIndexFromGridCoord(i, j, k + 1),
					   ScalarIndexFromGridCoord(i, j + 1, k),
					   ScalarIndexFromGridCoord(i + 1, j + 1, k),
					   ScalarIndexFromGridCoord(i + 1, j + 1, k + 1),
					   ScalarIndexFromGridCoord(i, j + 1, k + 1)};

	const int cubeIndex = calculate_cube_index(cell, m_Threshold);
	const std::vector<glm::vec3> intersections =
		get_intersection_points(cell, m_Threshold);
	const std::vector<glm::vec3> normals =
		get_intersection_normals(cell, m_Threshold);

	std::vector intersectionIndices = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

	for (int idx = 0; triangleTable[cubeIndex][idx] != -1; idx++) {
		const int vertexId = triangleTable[cubeIndex][idx];
		if (intersectionIndices[vertexId] == -1) {
			vertices.push_back(
				{intersections[vertexId], normals[vertexId], glm::vec2(0, 0)});
			intersectionIndices[vertexId] = vertices.size() - 1;
		}
		indices.push_back(intersectionIndices[vertexId]);
	}
}

glm::ivec3 Terrain::GetChunkCounts() const
{
	const glm::ivec3 cells =
		glm::max(m_Resolution - glm::ivec3(1), glm::ivec3(1));
	return (cells + glm::ivec3(m_ChunkCellSize - 1)) / m_ChunkCellSize;
}

int Terrain::GetChunkIndex(const glm::ivec3 chunk) const
{
	return chunk.x * m_ChunkCounts.y * m_ChunkCounts.z +
		   chunk.y * m_ChunkCounts.z + chunk.z;
}

void Terrain::RecalculateChunks()
{
	glm::ivec3 chunkCounts = GetChunkCounts();
	if (chunkCounts == m_ChunkCounts &&
		m_Chunks.size() ==
			(size_t) (chunkCounts.x * chunkCounts.y * chunkCounts.z))
		return;

	m_ChunkCounts = chunkCounts;
	m_Chunks.clear();
	m_Chunks.reserve(m_ChunkCounts.x * m_ChunkCounts.y * m_ChunkCounts.z);

	int id = 0;
	for (int x = 0; x < m_ChunkCounts.x; x++) {
		for (int y = 0; y < m_ChunkCounts.y; y++) {
			for (int z = 0; z < m_ChunkCounts.z; z++) {
				const glm::ivec3 minCell =
					glm::ivec3(x, y, z) * m_ChunkCellSize;
				const glm::ivec3 maxCell =
					glm::min(minCell + glm::ivec3(m_ChunkCellSize),
							 m_Resolution - glm::ivec3(1));
				m_Chunks.push_back(std::make_unique<TerrainChunk>(
					minCell, maxCell, m_ScalarField, id++, m_Shader));
			}
		}
	}
}

void Terrain::SetShader(Engine::Shader *shader)
{
	m_Shader = shader;
	for (auto &chunk : m_Chunks)
		chunk->SetShader(shader);
}

void Terrain::Render() const
{
	for (const auto &chunk : m_Chunks) {
		if (!chunk->m_Mesh || chunk->m_Mesh->p_Indices.empty())
			continue;
		chunk->Render();
	}
}

void Terrain::RecalculateGradients()
{
	const glm::vec3 voxelSize = GetVoxelSize();

	for (int x = 0; x < m_Resolution.x; ++x) {
		for (int y = 0; y < m_Resolution.y; ++y) {
			for (int z = 0; z < m_Resolution.z; ++z)
				RecalculateGradientAt(x, y, z, voxelSize);
		}
	}
}

void Terrain::RecalculateGradients(glm::ivec3 dirtyMin, glm::ivec3 dirtyMax)
{
	const glm::vec3 voxelSize = GetVoxelSize();

	dirtyMin =
		glm::clamp(dirtyMin - glm::ivec3(1), glm::ivec3(0), m_Resolution - 1);
	dirtyMax =
		glm::clamp(dirtyMax + glm::ivec3(1), glm::ivec3(0), m_Resolution - 1);

	for (int x = dirtyMin.x; x <= dirtyMax.x; ++x) {
		for (int y = dirtyMin.y; y <= dirtyMax.y; ++y) {
			for (int z = dirtyMin.z; z <= dirtyMax.z; ++z)
				RecalculateGradientAt(x, y, z, voxelSize);
		}
	}
}

void Terrain::RecalculateGradientAt(const int x,
									const int y,
									const int z,
									const glm::vec3 voxelSize)
{
	const int index = ScalarIndexFromGridCoord(x, y, z);
	auto &point = m_ScalarField[index];

	const int xPrev = std::max(x - 1, 0);
	const int xNext = std::min(x + 1, m_Resolution.x - 1);
	const int yPrev = std::max(y - 1, 0);
	const int yNext = std::min(y + 1, m_Resolution.y - 1);
	const int zPrev = std::max(z - 1, 0);
	const int zNext = std::min(z + 1, m_Resolution.z - 1);

	const float dx =
		m_ScalarField[ScalarIndexFromGridCoord(xNext, y, z)].scalar -
		m_ScalarField[ScalarIndexFromGridCoord(xPrev, y, z)].scalar;
	const float dy =
		m_ScalarField[ScalarIndexFromGridCoord(x, yNext, z)].scalar -
		m_ScalarField[ScalarIndexFromGridCoord(x, yPrev, z)].scalar;
	const float dz =
		m_ScalarField[ScalarIndexFromGridCoord(x, y, zNext)].scalar -
		m_ScalarField[ScalarIndexFromGridCoord(x, y, zPrev)].scalar;

	const float xSpacing = (xNext - xPrev) * voxelSize.x;
	const float ySpacing = (yNext - yPrev) * voxelSize.y;
	const float zSpacing = (zNext - zPrev) * voxelSize.z;

	point.gradient = {xSpacing > 0.0f ? dx / xSpacing : 0.0f,
					  ySpacing > 0.0f ? dy / ySpacing : 0.0f,
					  zSpacing > 0.0f ? dz / zSpacing : 0.0f};
}

bool Terrain::IsWorldPositionSolid(const glm::vec3 worldPosition) const
{
	const glm::ivec3 gridCoord =
		NearestGridCoordFromWorldPosition(worldPosition);
	return m_ScalarField[ScalarIndexFromGridCoord(gridCoord)].scalar >
		   m_Threshold;
}

glm::vec3 getRayDir(glm::vec2 mouseNdc, glm::mat4 view, glm::mat4 projection)
{
	using namespace glm;
	auto ray_clip = vec4(mouseNdc.x, mouseNdc.y, -1.0f, 1.0f);
	auto ray_eye = inverse(projection) * ray_clip;
	ray_eye = vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
	auto inv_ray_wor = inverse(view) * ray_eye;
	auto ray_wor = vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
	ray_wor = normalize(ray_wor);
	return ray_wor;
}

namespace
{
	bool IntersectRayAabb(const glm::vec3 &origin,
						  const glm::vec3 &dir,
						  const glm::vec3 &boundsMin,
						  const glm::vec3 &boundsMax,
						  float &tEnter,
						  float &tExit)
	{
		tEnter = 0.0f;
		tExit = std::numeric_limits<float>::max();

		for (int axis = 0; axis < 3; ++axis) {
			const float originAxis = origin[axis];
			const float dirAxis = dir[axis];
			const float minAxis = boundsMin[axis];
			const float maxAxis = boundsMax[axis];

			if (glm::epsilonEqual(dirAxis, 0.0f, 1e-6f)) {
				if (originAxis < minAxis || originAxis > maxAxis)
					return false;
				continue;
			}

			float t1 = (minAxis - originAxis) / dirAxis;
			float t2 = (maxAxis - originAxis) / dirAxis;
			if (t1 > t2)
				std::swap(t1, t2);

			tEnter = std::max(tEnter, t1);
			tExit = std::min(tExit, t2);
			if (tEnter > tExit)
				return false;
		}

		return tExit >= 0.0f;
	}
}	 // namespace

int Terrain::RayCastFromMousePos(glm::vec2 mouseNdc,
								 glm::mat4 view,
								 glm::mat4 proj,
								 float maxDist) const
{
	const auto pos = glm::vec3(glm::inverse(view)[3]);
	const auto dir = getRayDir(mouseNdc, view, proj);

	return RayCast(pos, dir, maxDist);
}

int Terrain::RayCast(glm::vec3 origin, glm::vec3 dir, float maxDist) const
{
	float tEnter, tExit;
	if (!IntersectRayAabb(origin, dir, m_Bounds[0], m_Bounds[1], tEnter, tExit))
		return -1;

	if (tEnter > maxDist || tExit < 0.0f)
		return -1;

	const float startT = std::max(tEnter, 0.0f);
	const glm::vec3 startPos = origin + dir * startT;
	const glm::vec3 vs = GetVoxelSize();
	glm::ivec3 step = glm::ivec3(glm::sign(dir));
	glm::ivec3 gridPoint = NearestGridCoordFromWorldPosition(startPos);

	if (glm::distance2(origin, startPos) > maxDist * maxDist)
		return -1;

	// DDA
	glm::vec3 nextBoundary;
	nextBoundary.x =
		m_Bounds[0].x + (gridPoint.x + (step.x > 0 ? 1.f : 0.f)) * vs.x;
	nextBoundary.y =
		m_Bounds[0].y + (gridPoint.y + (step.y > 0 ? 1.f : 0.f)) * vs.y;
	nextBoundary.z =
		m_Bounds[0].z + (gridPoint.z + (step.z > 0 ? 1.f : 0.f)) * vs.z;
	glm::vec3 tDelta = glm::abs(vs / dir);
	glm::vec3 tMax = (nextBoundary - startPos) / dir;

	while (true) {
		if (gridPoint.x < 0 || gridPoint.y < 0 || gridPoint.z < 0 ||
			gridPoint.x >= m_Resolution.x || gridPoint.y >= m_Resolution.y ||
			gridPoint.z >= m_Resolution.z)
			break;

		int i = ScalarIndexFromGridCoord(gridPoint.x, gridPoint.y, gridPoint.z);
		if (i < 0 || i >= m_ScalarField.size())
			break;

		if (m_ScalarField[i].scalar > m_Threshold)
			return i;

		const float traveled =
			startT + std::min(tMax.x, std::min(tMax.y, tMax.z));
		if (traveled > maxDist)
			break;

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

	return -1;
}
