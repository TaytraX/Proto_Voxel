#include <array>
#include <glm/glm.hpp>
#include "engine_constants.hpp"
#include "ankerl/unordered_dense.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

void connectToServer(const char* serverIp, int serverPort);
void recvComponent(ankerl::unordered_dense::segmented_map<uint64_t, glm::vec3*>& entities);
void sendVelocity(const glm::vec3& velocity);
void closeConnection();

void integrateToServer(ankerl::unordered_dense::segmented_map<glm::ivec3, std::array<uint32_t, CHUNK_AXIS3_SIZE>>& scene, ankerl::unordered_dense::segmented_map<uint64_t, glm::vec3*>& entities);