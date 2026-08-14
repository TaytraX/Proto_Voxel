#include "block.h"
#include <algorithm>
#include <iostream>
#include "engine_constants.hpp"

void addBlock(glm::vec3& pos, glm::vec3& forward, uint32_t blockID, uint32_t* voxels) {
	std::cout << "Forward vector is (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;

	float& xinc = forward.x;
	float& yinc = forward.y;
	float& zinc = forward.z;

	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	int i = 0;
	while(true) {
		i++;
		x += xinc;
		y += yinc;
		z += zinc;

		if (uint32_t& voxel = voxels[(int)floor(y) * CHUNK_AXIS2_SIZE + (CHUNK_AXIS1_SIZE * (int)floor(x)) + (int)floor(z)]; voxel != 0) {
			voxels[(int)floor(y - yinc) * CHUNK_AXIS2_SIZE + (CHUNK_AXIS1_SIZE * (int)floor(x - xinc)) + (int)floor(z - zinc)] = blockID;
			std::cout << "Interation number " << i << "at position (" << x << ", " << y << ", " << z << ")" << std::endl;
			return;
		}
	}
}

void removeBlock(glm::vec3& pos, glm::vec3& forward, uint32_t* voxels) {
	std::cout << "Forward vector is (" << forward.x << ", " << forward.y << ", " << forward.z << ")" << std::endl;

	float& xinc = forward.x;
	float& yinc = forward.y;
	float& zinc = forward.z;

	float x = pos.x;
	float y = pos.y;
	float z = pos.z;

	int i = 0;
	while (true) {
		i++;
		x += xinc;
		y += yinc;
		z += zinc;

		if (uint32_t& voxel = voxels[(int)floor(y) * CHUNK_AXIS2_SIZE + (CHUNK_AXIS1_SIZE * (int)floor(x)) + (int)floor(z)]; voxel != 0) {
			voxel = 0;
			std::cout << "Interation number " << i << "at position (" << x << ", " << y << ", " << z << ")" << std::endl;
			return;
		}
	}
}