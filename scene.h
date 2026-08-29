#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "mesher.h"
#include <queue>
#include <unordered_map>

namespace scene {
	extern glm::ivec3 centralChunk;
	extern std::unordered_map<glm::ivec3, std::pair<MeshData, size_t>> chunkMeshMap;
	extern std::unordered_map<glm::ivec3, uint32_t[CHUNK_AXIS3_SIZE]> chunkMap;

	void genScene();
	
	void startWorker();
	void stopWorker();
	void notifyMoved();
	bool getReadyChunk(glm::ivec3& outKey);
	bool queueEmpty();
}