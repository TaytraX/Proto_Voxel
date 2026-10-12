#include <array>
#include <glm/glm.hpp>

void connectToServer(const char* serverIp, int serverPort);
void recvComponent(glm::vec3* outVector);
void sendVelocity(const glm::vec3& velocity);
bool getChunk(std::array<uint32_t, CHUNK_AXIS3_SIZE>& outBuffer);
void closeConnection();