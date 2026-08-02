#include <vector>
#include "engine_constants.hpp"
#include <cstdint>

uint16_t* generateOpaqueMask(const uint32_t* chunk)
{
    uint16_t opaqueMask[CHUNK_AXIS2_SIZE]; // un mot 64 bits par colonne (x, y)

    for (int y = 0; y < CHUNK_AXIS1_SIZE; ++y)
    {
        for (int z = 0; z < CHUNK_AXIS1_SIZE; ++z)
        {
            for (int x = 0; x < CHUNK_AXIS1_SIZE; ++x)
            {
                int index = y * (CHUNK_AXIS1_SIZE * CHUNK_AXIS1_SIZE) + (CHUNK_AXIS1_SIZE * z) + x;

                if (chunk[index] != 0) // voxel plein (non-air)
                {
                    opaqueMask[(y * CHUNK_AXIS1_SIZE) + x] |= 1ull << z;
                }
            }
        }
    }

    return opaqueMask;
}

constexpr uint16_t P_MASK = ~(1ull << 15 | 1);
/*
uint64_t* generateChunkMesh(const uint32_t* chunk) {
    auto opaqueMask = generateOpaqueMask(chunk);
    uint16_t* faceMasks = new uint16_t[CHUNK_AXIS2_SIZE]{};

    for (int a = 1; a < CHUNK_AXIS1_SIZE - 1; a++) {
        const int aCS_P = a * CHUNK_AXIS1_SIZE;

        for (int b = 1; b < CHUNK_AXIS1_SIZE - 1; b++) {
            const uint64_t columnBits = opaqueMask[(a * CHUNK_AXIS1_SIZE) + b] & P_MASK;
            const int baIndex = (b - 1) + (a - 1) * CHUNK_AXIS1_SIZE;
            const int abIndex = (a - 1) + (b - 1) * CHUNK_AXIS1_SIZE;

            faceMasks[baIndex + 0 * CHUNK_AXIS2_SIZE] = (columnBits & ~opaqueMask[aCS_P + CHUNK_AXIS1_SIZE + b]) >> 1;
            faceMasks[baIndex + 1 * CHUNK_AXIS2_SIZE] = (columnBits & ~opaqueMask[aCS_P - CHUNK_AXIS1_SIZE + b]) >> 1;

            faceMasks[abIndex + 2 * CHUNK_AXIS2_SIZE] = (columnBits & ~opaqueMask[aCS_P + (b + 1)]) >> 1;
            faceMasks[abIndex + 3 * CHUNK_AXIS2_SIZE] = (columnBits & ~opaqueMask[aCS_P + (b - 1)]) >> 1;

            faceMasks[baIndex + 4 * CHUNK_AXIS2_SIZE] = columnBits & ~(opaqueMask[aCS_P + b] >> 1);
            faceMasks[baIndex + 5 * CHUNK_AXIS2_SIZE] = columnBits & ~(opaqueMask[aCS_P + b] << 1);
        }
    }
}*/