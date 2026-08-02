#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "vertex.h"

struct SubMesh {
    uint32_t firstQuad;
    uint32_t quadCount;
    uint16_t materialID;
};

struct ChunkMesh {
	std::vector<uint32_t> vertices;
    std::vector<SubMesh>  subMeshs;
};

constexpr uint32_t chunk_size = 16;
constexpr uint32_t Tx = chunk_size;
constexpr uint32_t Ty = chunk_size;
constexpr uint32_t Tz = chunk_size;

// Grille de coins : un cube (x,y,z) occupe l'espace [x, x+1] x [y, y+1] x [z, z+1]
constexpr uint32_t Vx = Tx + 1;
constexpr uint32_t Vy = Ty + 1;
constexpr uint32_t Vz = Tz + 1;

ChunkMesh generateChunkMesh(const uint32_t* voxels);