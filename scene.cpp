#include "scene.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include "util.hpp"
#include <chrono>

uint32_t newChunk[CHUNK_AXIS3_SIZE]{};

void generateFlatChunk() {
    for (int x = 0; x < CHUNK_AXIS1_SIZE; ++x) {
        for (int y = 1; y < 2; ++y) {
            for (int z = 0; z < CHUNK_AXIS1_SIZE; ++z) {
                newChunk[voxelIndex(x, y, z)] = 1;
                newChunk[voxelIndex(1, 2, 1)] = 2;
            }
        }
    }
}

namespace scene {
	glm::ivec3 centralChunk(0);
	ankerl::unordered_dense::segmented_map<glm::ivec3, MeshData> chunkMeshMap;
	ankerl::unordered_dense::segmented_map<glm::ivec3, std::array<uint32_t, CHUNK_AXIS3_SIZE>> chunkMap;

    std::queue<glm::ivec3> chunkQueue;
    std::mutex queueMutex;

    static std::thread worker;
    static std::atomic<bool> running{ false };
    static std::mutex cvMutex;
    static std::condition_variable cv;
    static bool moveRequested = false;

    static bool outsideWindow(glm::ivec3 pos) {
        int dx = centralChunk.x - pos.x;
        int dz = centralChunk.z - pos.z;
        constexpr int R1sq = (RENDER_DISTANCE + 1) * (RENDER_DISTANCE + 1);
        return (dx * dx + dz * dz) >= R1sq;
    }

	void genScene() {
		for (const auto& [chunkPos, voxels] : chunkMap) {

			std::cout << "Index of chunk at position (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") is " << index_of(chunkMap, chunkPos) << std::endl;
            MeshData meshData{
                .faceMasks = new uint64_t[CHUNK_AXIS2_SIZE * 6]{ 0 },
                .opaqueMask = fillOpaqueMask(chunkMap[chunkPos]),
                .forwardMerged = new uint8_t[CHUNK_AXIS2_SIZE]{ 0 },
                .rightMerged = new uint8_t[CHUNK_AXIS1_SIZE]{ 0 },
                .vertices = new std::vector<uint64_t>(MAX_FACE),
                .maxVertices = MAX_FACE
            };
            
            mesh(voxels, meshData);
            chunkMeshMap[chunkPos] = meshData;

			std::cout << "Chunk at position (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") meshed with " << meshData.vertices->size() << " faces." << std::endl;
		}
		std::cout << "Scene generated with " << chunkMap.size() << " chunks." << std::endl;
        generateFlatChunk();
	}

    void extendScene() {
        std::vector<glm::ivec3> outOfWindowChunkPos;
        outOfWindowChunkPos.reserve(RENDER_DISTANCE * 2 + 1);

        for (const auto& [key, voxels] : chunkMap) {
            if (outsideWindow(key)) {
                outOfWindowChunkPos.push_back(key);
            }
        }

        std::cout << "Num chunks out of window: " << outOfWindowChunkPos.size() << std::endl;
        
        for (glm::ivec3 key : outOfWindowChunkPos) {
            MeshData& meshData = chunkMeshMap[key];

            // Installe les nouvelles données de voxels
            std::memcpy(
                chunkMap[key].data(),
                newChunk,
                CHUNK_MEM_SIZE
            );

            // Régénère uniquement opaqueMask (le seul élément qui doit l'être)
            delete[] meshData.opaqueMask;
            meshData.opaqueMask = fillOpaqueMask(chunkMap[key]);

            auto newKey = 2 * centralChunk - key;

            mesh(chunkMap[key], meshData);
            chunkMap.replace_key(chunkMap.find(key), newKey);
            chunkMeshMap.replace_key(chunkMeshMap.find(key), newKey);

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                chunkQueue.push(newKey);
            }
        }
    }

    static void workerLoop() {
        while (running) {
            {
                std::unique_lock<std::mutex> lock(cvMutex);
                cv.wait(lock, [] { return moveRequested || !running; });
                if (!running) return;
                moveRequested = !chunkQueue.empty();
            }

            extendScene();
        }
    }

    ///////////////////////////////////////////////////////////TREADING/////////////////////////////////////////////////////////////
    void startWorker() { running = true; worker = std::thread(workerLoop); }

    void stopWorker() {
        running = false;
        { std::lock_guard<std::mutex> lock(cvMutex); moveRequested = true; }
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    void notifyMoved() {
        { std::lock_guard<std::mutex> lock(cvMutex); moveRequested = true; }
        cv.notify_one();
    }

    bool getReadyChunk(glm::ivec3& outKey) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (chunkQueue.empty()) return false;
        outKey = chunkQueue.front();
        chunkQueue.pop();
        return true;
    }
}