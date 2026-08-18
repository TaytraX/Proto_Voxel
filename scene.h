#pragma once
#include <queue>
#include <glm/glm.hpp>
#include "mesher.h"
#include "engine_constants.hpp"

namespace scene {
	extern std::queue<uint64_t> chunkQueue;

	std::vector<std::pair<MeshData, glm::ivec3>> genScene(uint32_t voxels[RENDER_DISTANCE * 2 + 1][RENDER_DISTANCE * 2 + 1][CHUNK_AXIS3_SIZE]);
	void updateChunk(glm::ivec2 chunkPos);
	void moveScene(glm::ivec3 direction);
}