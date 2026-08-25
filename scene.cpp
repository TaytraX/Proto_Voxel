#include "scene.h"
#include <cmath>
#include <iostream>

namespace scene {
	std::queue<uint64_t> chunkQueue;
	std::unordered_map<glm::ivec3, std::pair<MeshData, size_t>> chunkMap;
	uint32_t chunk[RENDER_DISTANCE * 2 + 1][RENDER_DISTANCE * 2 + 1][CHUNK_AXIS3_SIZE] = { 0 };

	void genScene() {
		size_t index = 0;
		for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
			for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
				if (round(std::sqrt(x * x + z * z)) > RENDER_DISTANCE) continue;
				glm::ivec3 chunkPos(x, 0, z);
				MeshData meshData{
					.faceMasks = new uint64_t[CHUNK_AXIS2_SIZE * 6]{ 0 },
					.opaqueMask = fillOpaqueMask(chunk[x + RENDER_DISTANCE][z + RENDER_DISTANCE]),
					.forwardMerged = new uint8_t[CHUNK_AXIS2_SIZE]{ 0 },
					.rightMerged = new uint8_t[CHUNK_AXIS1_SIZE + 2]{ 0 },
					.vertices = new std::vector<uint64_t>(1000),
					.maxVertices = 1000
				};

				//std::cout << "chunk [" << index << "]" << std::endl;
				mesh(chunk[x + RENDER_DISTANCE][z + RENDER_DISTANCE], meshData);
				chunkMap[chunkPos] = { meshData, index };
				index++;
			}
		}
	}

	void moveScene(glm::ivec3 direction) {

	}
}