#include "mesher.h"
#include <iostream>
#include <array>

// Remappe les paramètres génériques (a, b, c) de l'algorithme — dont le sens
// change selon l'axe balayé — vers de vraies coordonnées (x, y, z), puis
// appelle voxelIndex(). C'est mathématiquement équivalent à l'ancienne
// version codée en dur (b + a*CS_P + c*CS_P2, etc.), mais explicite désormais
// la correspondance avec ta formule au lieu de la laisser implicite.
static inline const size_t getAxisIndex(const int axis, const int a, const int b, const int c) {
    if (axis == 0) return voxelIndex(b, c, a);
    else if (axis == 1) return voxelIndex(b, a, c);
    else return voxelIndex(c, b, a);
}

static inline const void insertQuad(std::vector<uint64_t>& vertices, uint64_t quad, int& vertexI, int& maxVertices) {
    if (vertexI >= maxVertices - 6) {
        vertices.resize(maxVertices * 2, 0);
        maxVertices *= 2;
    }

    vertices[vertexI] = quad;

    vertexI++;
}

static inline const uint64_t getQuad(uint64_t x, uint64_t y, uint64_t z, uint64_t w, uint64_t h, uint64_t type) {
    // data.x (bits 0-31)  : x(7) y(7) z(7) w(7) — 28/32 bits utilisés
    // data.y (bits 32-63) : type(24) h(7)       — 31/32 bits utilisés
    return (h << 56) | (type << 32) | (w << 21) | (z << 14) | (y << 7) | x;
}

constexpr uint64_t P_MASK = UINT64_MAX;

uint64_t* fillOpaqueMask(const std::array<uint32_t, CHUNK_AXIS3_SIZE>& voxels) {
    uint64_t* opaqueMask = new uint64_t[CHUNK_AXIS2_SIZE] { 0 };

    // Le 3e axe (c) est compressé dans les bits d'un même uint64_t.
    // ATTENTION: la permutation exacte ici n'est PAS arbitraire — elle doit
    // correspondre à celle que le greedy meshing utilise pour relire le voxel
    // (voir getAxisIndex). Vérifiée par trace manuelle : opaqueMask[a*CS_P+b]
    // bit c doit correspondre à voxels[voxelIndex(c, a, b)].
    for (int a = 0; a < CHUNK_AXIS1_SIZE; a++) {
        const int aCS_P = a * CHUNK_AXIS1_SIZE;

        for (int b = 0; b < CHUNK_AXIS1_SIZE; b++) {
            uint64_t bits = 0;

            for (int c = 0; c < CHUNK_AXIS1_SIZE; c++) {
                if (voxels[voxelIndex(c, a, b)] != 0) {
                    bits |= (1ull << c);
                }
            }

            opaqueMask[aCS_P + b] = bits;
        }
    }

    return opaqueMask;
}

void mesh(const std::array<uint32_t, CHUNK_AXIS3_SIZE>& voxels, MeshData& meshData) {
    meshData.vertexCount = 0;
    int vertexI = 0;

    uint64_t* opaqueMask = meshData.opaqueMask;
    uint64_t* faceMasks = meshData.faceMasks;
    uint8_t* forwardMerged = meshData.forwardMerged;
    uint8_t* rightMerged = meshData.rightMerged;

    // Hidden face culling
    for (int a = 0; a < CHUNK_AXIS1_SIZE; a++) {
        const int aCS_P = a * CHUNK_AXIS1_SIZE;

        for (int b = 0; b < CHUNK_AXIS1_SIZE; b++) {
            const uint64_t columnBits = opaqueMask[aCS_P + b];
            const int baIndex = b + a * CHUNK_AXIS1_SIZE;
            const int abIndex = a + b * CHUNK_AXIS1_SIZE;

            const uint64_t negA = (a > 0) ? opaqueMask[aCS_P - CHUNK_AXIS1_SIZE + b] : 0;
            const uint64_t posA = (a < CHUNK_AXIS1_SIZE - 1) ? opaqueMask[aCS_P + CHUNK_AXIS1_SIZE + b] : 0;
            const uint64_t posB = (b < CHUNK_AXIS1_SIZE - 1) ? opaqueMask[aCS_P + (b + 1)] : 0;
            const uint64_t negB = (b > 0) ? opaqueMask[aCS_P + (b - 1)] : 0;

            faceMasks[baIndex + 0 * CHUNK_AXIS2_SIZE] = columnBits & ~posA;
            faceMasks[baIndex + 1 * CHUNK_AXIS2_SIZE] = columnBits & ~negA;
            faceMasks[abIndex + 2 * CHUNK_AXIS2_SIZE] = columnBits & ~posB;
            faceMasks[abIndex + 3 * CHUNK_AXIS2_SIZE] = columnBits & ~negB;

            faceMasks[baIndex + 4 * CHUNK_AXIS2_SIZE] = columnBits & ~(columnBits >> 1);
            faceMasks[baIndex + 5 * CHUNK_AXIS2_SIZE] = columnBits & ~(columnBits << 1);
        }
    }

    // Greedy meshing faces 0-3
    for (int face = 0; face < 4; face++) {
        const int axis = face / 2;

        const int faceVertexBegin = vertexI;

        for (int layer = 0; layer < CHUNK_AXIS1_SIZE; layer++) {
            const int bitsLocation = layer * CHUNK_AXIS1_SIZE + face * CHUNK_AXIS2_SIZE;

            for (int forward = 0; forward < CHUNK_AXIS1_SIZE; forward++) {
                uint64_t bitsHere = faceMasks[forward + bitsLocation];

                const uint64_t bitsNext = forward + 1 < CHUNK_AXIS1_SIZE ? faceMasks[(forward + 1) + bitsLocation] : 0;

                uint8_t rightMerged = 1;
                while (bitsHere) {
                    unsigned long bitPos;
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif

                    const uint32_t type = voxels[getAxisIndex(axis, forward, bitPos, layer)];
                    uint8_t& forwardMergedRef = forwardMerged[bitPos];

                    if ((bitsNext >> bitPos & 1) && type == voxels[getAxisIndex(axis, forward + 1, bitPos, layer)]) {
                        forwardMergedRef++;
                        bitsHere &= ~(1ull << bitPos);
                        continue;
                    }

                    for (int right = bitPos + 1; right < CHUNK_AXIS1_SIZE; right++) {
                        if (!(bitsHere >> right & 1) || forwardMergedRef != forwardMerged[right] || type != voxels[getAxisIndex(axis, forward, right, layer)]) break;
                        forwardMerged[right] = 0;
                        rightMerged++;
                    }
                    const int clearBits = bitPos + rightMerged;
                    const uint64_t clearMask = (clearBits >= 64) ? ~0ull : ((1ull << clearBits) - 1);
                    bitsHere &= ~clearMask;

                    const uint8_t meshFront = forward - forwardMergedRef;
                    const uint8_t meshLeft = bitPos;
                    const uint8_t meshUp = layer + (~face & 1);

                    const uint8_t meshWidth = rightMerged;
                    const uint8_t meshLength = forwardMergedRef + 1;

                    forwardMergedRef = 0;
                    rightMerged = 1;

                    uint64_t quad;
                    switch (face) {
                    case 0:
                    case 1:
                        quad = getQuad(meshFront + (face == 1 ? meshLength : 0), meshUp, meshLeft, meshLength, meshWidth, type);
                        break;
                    case 2:
                    case 3:
                        quad = getQuad(meshUp, meshFront + (face == 2 ? meshLength : 0), meshLeft, meshLength, meshWidth, type);
                        break;
                    }

                    insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
                }
            }
        }

        const int faceVertexLength = vertexI - faceVertexBegin;
        meshData.faceVertexBegin[face] = faceVertexBegin;
        meshData.faceVertexLength[face] = faceVertexLength;
    }

    // Greedy meshing faces 4-5
    for (int face = 4; face < 6; face++) {
        const int axis = face / 2;

        const int faceVertexBegin = vertexI;

        for (int forward = 0; forward < CHUNK_AXIS1_SIZE; forward++) {
            const int bitsLocation = forward * CHUNK_AXIS1_SIZE + face * CHUNK_AXIS2_SIZE;
            const int bitsForwardLocation = (forward + 1) * CHUNK_AXIS1_SIZE + face * CHUNK_AXIS2_SIZE;

            for (int right = 0; right < CHUNK_AXIS1_SIZE; right++) {
                uint64_t bitsHere = faceMasks[right + bitsLocation];
                if (bitsHere == 0) continue;

                const uint64_t bitsForward = forward < CHUNK_AXIS1_SIZE - 1 ? faceMasks[right + bitsForwardLocation] : 0;
                const uint64_t bitsRight = right < CHUNK_AXIS1_SIZE - 1 ? faceMasks[right + 1 + bitsLocation] : 0;
                const int rightCS = right * CHUNK_AXIS1_SIZE;

                for (int i = 0; i < 64; i++) {
					if (bitsHere == 0) break;
                    unsigned long bitPos;
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif

                    bitsHere &= ~(1ull << bitPos);

                    const uint32_t type = voxels[getAxisIndex(axis, right, forward, bitPos)];
                    uint8_t& forwardMergedRef = forwardMerged[rightCS + bitPos];
                    uint8_t& rightMergedRef = rightMerged[bitPos];

                    if (rightMergedRef == 0 && (bitsForward >> bitPos & 1) && type == voxels[getAxisIndex(axis, right, forward + 1, bitPos)]) {
                        forwardMergedRef++;
                        continue;
                    }

                    if ((bitsRight >> bitPos & 1) && forwardMergedRef == forwardMerged[(rightCS + CHUNK_AXIS1_SIZE) + bitPos] && type == voxels[getAxisIndex(axis, right + 1, forward, bitPos)]) {
                        forwardMergedRef = 0;
                        rightMergedRef++;
                        continue;
                    }

                    const uint8_t meshLeft = right - rightMergedRef;
                    const uint8_t meshFront = forward - forwardMergedRef;
                    const uint8_t meshUp = bitPos + (~face & 1);

                    const uint8_t meshWidth = 1 + rightMergedRef;
                    const uint8_t meshLength = 1 + forwardMergedRef;

                    forwardMergedRef = 0;
                    rightMergedRef = 0;

                    const uint64_t quad = getQuad(meshLeft + (face == 4 ? meshWidth : 0), meshFront, meshUp, meshWidth, meshLength, type);

                    insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
                }
            }
        }

        const int faceVertexLength = vertexI - faceVertexBegin;
        meshData.faceVertexBegin[face] = faceVertexBegin;
        meshData.faceVertexLength[face] = faceVertexLength;
    }

    meshData.vertexCount = vertexI;
    std::fill(meshData.vertices->begin() + vertexI, meshData.vertices->end(), 0ull);
}