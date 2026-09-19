#include <array>
#include "engine_constants.hpp"

void connectToServer(const char* serverIp, int serverPort);
bool getChunk(std::array<uint32_t, CHUNK_AXIS3_SIZE>& outBuffer);
void closeConnection();