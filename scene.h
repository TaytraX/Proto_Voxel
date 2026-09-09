#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "mesher.h"
#include <queue>
#include <unordered_dense.h>
#include <array>

namespace scene {
	extern glm::ivec3 centralChunk;
	extern ankerl::unordered_dense::segmented_map<glm::ivec3, MeshData> chunkMeshMap;
	extern ankerl::unordered_dense::segmented_map<glm::ivec3, std::array<uint32_t, CHUNK_AXIS3_SIZE>> chunkMap;

	void genScene();
	
	void startWorker();
	void stopWorker();
	void notifyMoved();
	bool getReadyChunk(glm::ivec3& outKey);
	bool queueEmpty();
}