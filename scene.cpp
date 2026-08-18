#include "scene.h"

namespace scene {
	std::queue<uint64_t> chunkQueue;

	std::vector<std::pair<MeshData, glm::ivec3>> genScene(uint32_t voxels[RENDER_DISTANCE * 2 + 1][RENDER_DISTANCE * 2 + 1][CHUNK_AXIS3_SIZE]) {
		std::vector<std::pair<MeshData, glm::ivec3>> sceneChunks;
		MeshData meshData{
			.faceMasks = new uint16_t[CS_2 * 6]{ 0 },
			.opaqueMask = nullptr,
			.forwardMerged = new uint8_t[CS_2]{ 0 },
			.rightMerged = new uint8_t[CS]{ 0 },
			.vertices = new std::vector<uint64_t>(1000),
			.maxVertices = 1000
		};
		
		for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
			for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
				glm::ivec3 chunkPos(x, 0, z);
				meshData.opaqueMask = fillOpaqueMask(voxels[x + RENDER_DISTANCE][z + RENDER_DISTANCE]);
				mesh(voxels[x + RENDER_DISTANCE][z + RENDER_DISTANCE], meshData);
				sceneChunks.push_back({ meshData, chunkPos });
			}
		}
		return sceneChunks;
	}

	void moveScene(glm::ivec3 direction) {

	}
}