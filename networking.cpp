#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "networking.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "engine_constants.hpp"

uint16_t entityId = 0;

#pragma comment(lib, "Ws2_32.lib")
SOCKET ConnectSocket = INVALID_SOCKET;
bool getChunk(std::array<uint32_t, CHUNK_AXIS3_SIZE>& outBuffer);

void connectToServer(const char* serverIp, int serverPort) {
	WSADATA wsaData;
	struct sockaddr_in clientService;
	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cerr << "WSAStartup failed: " << iResult << std::endl;
		return;
	}
	// Create a socket for connecting to the server
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		throw("Error at socket():");
		WSACleanup();
		return;
	}
	// Set up the sockaddr_in structure
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(serverIp);
	clientService.sin_port = htons(serverPort);
	// Connect to the server
	iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
	if (iResult == SOCKET_ERROR) {
		printf("WSA error = %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return;
	}
}

void integrateToServer(ankerl::unordered_dense::segmented_map<glm::ivec3, std::array<uint32_t, CHUNK_AXIS3_SIZE>>& scene, ankerl::unordered_dense::segmented_map<uint64_t, glm::vec3*>& entities) {
	for (int X = -RENDER_DISTANCE; X <= RENDER_DISTANCE; X++) {
		for (int Z = -RENDER_DISTANCE; Z <= RENDER_DISTANCE; Z++) {
			if (X * X + Z * Z <= (RENDER_DISTANCE + 1) * (RENDER_DISTANCE + 1)) {
				if (!getChunk(scene[glm::ivec3(X, 0, Z)])) {
					std::cerr << "Failed to get chunk for position (" << X << ", 0, " << Z << ")" << std::endl;
				}
			}
		}
	}

	while (true) {
		uint64_t id = 0;

		if (recv(ConnectSocket, (char*)&id, sizeof(uint64_t), MSG_WAITALL) == SOCKET_ERROR) {
			std::cerr << "Failed to receive entity ID" << std::endl;
			break;
		}

		if (id == UINT64_MAX) {
			break;
		}

		entities[id] = new glm::vec3();

		if (recv(ConnectSocket, (char*)entities[id], sizeof(glm::vec3), MSG_WAITALL) == SOCKET_ERROR) {
			std::cerr << "Failed to receive entity position" << std::endl;
			break;
		}
	}
}

bool getChunk(std::array<uint32_t, CHUNK_AXIS3_SIZE>& outBuffer) {
	return recv(ConnectSocket, (char*)outBuffer.data(), CHUNK_MEM_SIZE, MSG_WAITALL) != SOCKET_ERROR;
}

void sendVelocity(const glm::vec3& velocity) {
	static_assert(sizeof(glm::vec3) == 3 * sizeof(float), "vec3 padding inattendu");
	send(ConnectSocket, (const char*)&velocity, sizeof(glm::vec3), 0);
}

void recvComponent(ankerl::unordered_dense::segmented_map<uint64_t, glm::vec3*>& entities) {
	uint64_t id = 0;
	while (true) {
		recv(ConnectSocket, (char*)&id, sizeof(uint64_t), MSG_WAITALL);
		if (id == UINT64_MAX) break;

		if (!entities.contains(id)) {
			glm::vec3* last = entities.values().back().second;
			entities[id] = last + 1;
		}

		recv(ConnectSocket, (char*)entities[id], sizeof(glm::vec3), MSG_WAITALL);
	
	}
}

void closeConnection() {
	closesocket(ConnectSocket);
	WSACleanup();
}