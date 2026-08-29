#include "scene.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
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
    static std::mutex mapMutex;
	std::unordered_map<glm::ivec3, std::pair<MeshData, size_t>> chunkMeshMap;
	std::unordered_map<glm::ivec3, uint32_t[CHUNK_AXIS3_SIZE]> chunkMap;

    std::queue<glm::ivec3> chunkQueue;
    std::mutex queueMutex;

    static std::thread worker;
    static std::atomic<bool> running{ false };
    static std::mutex cvMutex;
    static std::condition_variable cv;
    static bool moveRequested = false;

    static std::mutex readyMutex;

    static bool outsideWindow(glm::ivec3 pos) {
        return floor(std::sqrt(pow((centralChunk.x - pos.x), 2) + pow((centralChunk.z - pos.z), 2))) > RENDER_DISTANCE;
    }

	void genScene() {
		size_t index = 0;
		for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
			for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
				if (floor(std::sqrt(x * x + z * z)) > RENDER_DISTANCE) continue;
				glm::ivec3 chunkPos(x, 0, z);
				MeshData meshData{
					.faceMasks = new uint64_t[CHUNK_AXIS2_SIZE * 6]{ 0 },
					.opaqueMask = fillOpaqueMask(chunkMap[chunkPos]),
					.forwardMerged = new uint8_t[CHUNK_AXIS2_SIZE]{ 0 },
					.rightMerged = new uint8_t[CHUNK_AXIS1_SIZE + 2]{ 0 },
					.vertices = new std::vector<uint64_t>(1000),
					.maxVertices = 1000
				};

				//std::cout << "chunk [" << index << "]" << std::endl;
				mesh(chunkMap[chunkPos], meshData);
				chunkMeshMap[chunkPos] = { meshData, index };
				index++;
			}
		}

		chunkMap.reserve(chunkMap.size());
        generateFlatChunk();
	}

    void extendScene() {
        auto start = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mapMutex);
        // direction = (±1,0,0) ou (0,0,±1) : un seul axe à la fois
        std::vector<glm::ivec3> outOfWindowChunkPos;
        for (int X = -CHUNK_AXIS1_SIZE; X <= CHUNK_AXIS1_SIZE; X++) {
            for (int Z = -CHUNK_AXIS1_SIZE; Z <= CHUNK_AXIS1_SIZE; Z++) {
                if (chunkMap.contains({ X, 0, Z }) && outsideWindow({ X, 0, Z })) {
                    outOfWindowChunkPos.push_back({ X, 0, Z });
                   // if (outOfWindowChunkPos.size() == CHUNK_AXIS1_SIZE * 2 + 1) break;
                }
            }
        }

        std::cout << "Num chuk " << outOfWindowChunkPos.size() << std::endl;

        for (glm::ivec3 key : outOfWindowChunkPos) {
            MeshData& meshData = chunkMeshMap[key].first;

            // Installe les nouvelles données de voxels
            for (int i = 0; i < CHUNK_AXIS3_SIZE; i++) {
                chunkMap[key][i] = newChunk[i];
            }

            // Régénère uniquement opaqueMask (le seul élément qui doit l'être)
            delete[] meshData.opaqueMask;
            meshData.opaqueMask = fillOpaqueMask(chunkMap[key]);

            mesh(chunkMap[key], meshData);

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                chunkQueue.push(key);
            }
        }
        auto end = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        );

        std::cout << "Temps écoulés : " << duration.count() << " ms\n";
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

    //////////////////////////////////////TREADING/////////////////////////////////////////////////////////////
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