#pragma once
#include <glm/glm.hpp>

glm::ivec2 getChunkIndex(glm::vec3 position);
glm::ivec3 addBlock(glm::vec3& pos, glm::vec3& forward, uint32_t blockID);
glm::ivec3 removeBlock(glm::vec3& pos, glm::vec3& forward);