#include "block.h"
#include <algorithm>
#include <iostream>
#include "engine_constants.hpp"
#include "mesher.h"
#include "scene.h"

glm::ivec3 getChunkPos(glm::vec3 position) {
	return {
		(int)floor(position.x / CHUNK_AXIS1_SIZE),
		(int)floor(position.y / CHUNK_AXIS1_SIZE),
		(int)floor(position.z / CHUNK_AXIS1_SIZE)
	};
}

uint32_t& getBlock(int x, int y, int z) {
	// Indice du chunk dans le tableau (déjà décalé par RENDER_DISTANCE par getChunkIndex)
	auto chunkPos = getChunkPos(glm::vec3(x, y, z));

	int chunkOriginX = chunkPos.x * CHUNK_AXIS1_SIZE;
	int chunkOriginZ = chunkPos.z * CHUNK_AXIS1_SIZE;

	// Coordonnées locales du bloc, relatives au coin bas-gauche-avant du chunk
	// (toujours positives grâce à la division "floor" faite dans getChunkIndex)
	int localX = x - chunkOriginX;
	int localY = y;
	int localZ = z - chunkOriginZ;

	return scene::chunkMap[chunkPos][voxelIndex(localZ, localY, localX)];
}

glm::ivec3 addBlock(glm::vec3& pos, glm::vec3& forward, uint32_t blockID) {
	std::cout << "Forward vector is (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;

	float xinc = forward.x;
	float yinc = forward.y;
	float zinc = forward.z;

	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	int i = 0;
	while (true) {
		i++;
		x += xinc;
		y += yinc;
		z += zinc;

		if (y < 0 || y > CHUNK_AXIS1_SIZE) {
			std::cout << "Out of bounds: (" << x << ", " << y << ", " << z << ")" << std::endl;
			return glm::ivec3(0);
		}

		if (uint32_t& voxel = getBlock((int)floor(x), (int)floor(y), (int)floor(z)); voxel != 0) {
			std::cout << "voxels = " << voxel << std::endl;
			getBlock((int)floor(x - xinc), (int)floor(y - yinc), (int)floor(z - zinc)) = blockID;

			auto chunkPos = glm::ivec3(
				(int)std::floor((x - xinc) / CHUNK_AXIS1_SIZE), 0,
				(int)std::floor((z - zinc) / CHUNK_AXIS1_SIZE)
			);

			int chunkOriginX = chunkPos.x * CHUNK_AXIS1_SIZE;
			int chunkOriginZ = chunkPos.z * CHUNK_AXIS1_SIZE;
			int localX = (int)floor(x - xinc) - chunkOriginX;
			int localZ = (int)floor(z - zinc) - chunkOriginZ;
			std::cout << "Adding block at chunk (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") local coords (" << localX << ", " << (int)floor(y - yinc) << ", " << localZ << ")" << std::endl;

			scene::chunkMeshMap[chunkPos].opaqueMask[(int)floor(y - yinc) * CHUNK_AXIS1_SIZE + localX] |= (1ull << localZ);
			return { chunkPos };
		}
	}
}

glm::ivec3 removeBlock(glm::vec3& pos, glm::vec3& forward) {
	std::cout << "Forward vector is (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;

	float xinc = forward.x;
	float yinc = forward.y;
	float zinc = forward.z;

	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	int i = 0;
	while (true) {
		i++;
		x += xinc;
		y += yinc;
		z += zinc;

		if (uint32_t& voxel = getBlock((int)floor(x), (int)floor(y), (int)floor(z)); voxel != 0) {
			std::cout << "voxels = " << voxel << std::endl;
			voxel = 0;

			auto chunkPos = glm::ivec3((int)std::floor(x / CHUNK_AXIS1_SIZE), 0, (int)std::floor(z / CHUNK_AXIS1_SIZE));

			// Recalcule les coordonnées locales (0..CHUNK_AXIS1_SIZE-1), comme dans getBlock
			int chunkOriginX = chunkPos.x * CHUNK_AXIS1_SIZE;
			int chunkOriginZ = chunkPos.z * CHUNK_AXIS1_SIZE;
			int localX = (int)floor(x) - chunkOriginX;
			int localZ = (int)floor(z) - chunkOriginZ;

			std::cout << "Removing block at chunk (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") local coords (" << localX << ", " << (int)floor(y) << ", " << localZ << ")" << std::endl;

			scene::chunkMeshMap[chunkPos].opaqueMask[(int)floor(y) * CHUNK_AXIS1_SIZE + localX] &= ~(1ull << localZ);
			return { chunkPos };
		}
	}
}