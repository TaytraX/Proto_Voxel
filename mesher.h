/*
MIT License

Copyright (c) 2020 Erik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

 //   Define BM_IMPLEMENTATION in exactly one source file to include the implementation
 //   and include the mesher.h without that define as often as needed
 //
 //   it could look like this
 //
 //   #define BM_IMPLEMENTATION
 //   #include "mesher.h"
 //
 //   There are other defines to control the behaviour of the library.
 //   * Define BM_VECTOR with your own vector implementation - otherwise it will use std::vector

#ifndef MESHER_H
#define MESHER_H
#include <vector>

#include <stdint.h>

// CS = chunk size (max 14)
static constexpr int CS = 14;

// Padded chunk size
static constexpr int CS_P = CS + 2;
static constexpr int CS_2 = CS * CS;
static constexpr int CS_P2 = CS_P * CS_P;
static constexpr int CS_P3 = CS_P * CS_P * CS_P;



// Dimensions du buffer voxel brut fourni par l'appelant (chunk cubique padded).
// Ta formule i = y*(Tx*Tz) + Tx*z + x devient, avec Tx = Tz = CS_P :
static constexpr int Tx = CS_P;
static constexpr int Tz = CS_P;

static const int voxelIndex(const int x, const int y, const int z) {
	return (y * (Tx * Tz)) + (Tx * z) + x;
}

struct MeshData {
	uint16_t* faceMasks = nullptr; // CS_2 * 6
	uint16_t* opaqueMask = nullptr; //CS_P2
	uint8_t* forwardMerged = nullptr; // CS_2
	uint8_t* rightMerged = nullptr; // CS
	std::vector<uint64_t>* vertices = nullptr;
	int vertexCount = 0;
	int maxVertices = 0;
	int faceVertexBegin[6] = { 0 };
	int faceVertexLength[6] = { 0 };
};

// @param[in] voxels: The input data includes duplicate edge data from neighboring chunks which is used
// for visibility culling. For optimal performance, your world data should already be structured
// this way so that you can feed the data straight into this algorithm.
// Input data is ordered in ZXY and is 64^3 which results in a 62^3 mesh.
//
// @param[out] meshData The allocated vertices in MeshData with a length of meshData.vertexCount.
void mesh(const uint32_t* voxels, MeshData& meshData);

// @brief Fills meshData.opaqueMask from the raw voxel buffer. MUST be called
// before mesh() on every call, since mesh() only reads opaqueMask/faceMasks,
// never voxels directly for visibility testing.
// @param[in] voxels: Same CS_P^3 padded buffer that will be passed to mesh().
// A voxel is considered opaque if its value is non-zero (id 0 = air/empty).
// @param[out] meshData: meshData.opaqueMask must already be allocated with
// CS_P2 elements. This function overwrites it completely (no need to
// zero-initialize beforehand).
uint16_t* fillOpaqueMask(const uint32_t* voxels);

#endif // MESHER_H