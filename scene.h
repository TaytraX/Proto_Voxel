#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "mesher.h"
#include <queue>
#include <unordered_map>

namespace scene {
	extern std::queue<uint64_t> chunkQueue;
	extern std::unordered_map<glm::ivec3, std::pair<MeshData, size_t>> chunkMap;
	extern uint32_t chunk[RENDER_DISTANCE * 2 + 1][RENDER_DISTANCE * 2 + 1][CHUNK_AXIS3_SIZE];

	void genScene();
	void updateChunk(glm::ivec2 chunkPos);
	void moveScene(glm::ivec3 direction);
}