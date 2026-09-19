#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "networking.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
SOCKET ConnectSocket = INVALID_SOCKET;

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

bool getChunk(std::array<uint32_t, CHUNK_AXIS3_SIZE>& outBuffer) {
	recv(ConnectSocket, (char*)outBuffer.data(), CHUNK_MEM_SIZE, MSG_WAITALL);
	return true;
}

void closeConnection() {
	closesocket(ConnectSocket);
	WSACleanup();
}