#pragma once
#include <glm/glm.hpp>

void addBlock(glm::vec3& pos, glm::vec3& forward, uint32_t blockID, uint32_t* voxels);
void removeBlock(glm::vec3& pos, glm::vec3& forward, uint32_t* voxels);