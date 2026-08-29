#pragma once
#include <glm/glm.hpp>

glm::ivec3 getChunkPos(glm::vec3 position);
glm::ivec3 addBlock(glm::vec3& pos, glm::vec3& forward, uint32_t blockID);
glm::ivec3 removeBlock(glm::vec3& pos, glm::vec3& forward);