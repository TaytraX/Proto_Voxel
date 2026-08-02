#include "mesher.h"

// Dimensions du buffer voxel brut fourni par l'appelant (chunk cubique padded).
// Ta formule i = y*(Tx*Tz) + Tx*z + x devient, avec Tx = Tz = CS_P :
static constexpr int Tx = CS_P;
static constexpr int Tz = CS_P;

// Implémente directement ton système de mapping :
// i = y*(Tx*Tz) + Tx*z + x
static inline const int voxelIndex(const int x, const int y, const int z) {
    return (y * (Tx * Tz)) + (Tx * z) + x;
}

// Remappe les paramètres génériques (a, b, c) de l'algorithme — dont le sens
// change selon l'axe balayé — vers de vraies coordonnées (x, y, z), puis
// appelle voxelIndex(). C'est mathématiquement équivalent à l'ancienne
// version codée en dur (b + a*CS_P + c*CS_P2, etc.), mais explicite désormais
// la correspondance avec ta formule au lieu de la laisser implicite.
static inline const int getAxisIndex(const int axis, const int a, const int b, const int c) {
    if (axis == 0) return voxelIndex(b, c, a);
    else if (axis == 1) return voxelIndex(b, a, c);
    else return voxelIndex(c, b, a);
}

static inline const void insertQuad(std::vector<uint64_t>& vertices, uint32_t quad, int& vertexI, int& maxVertices) {
    if (vertexI >= maxVertices - 6) {
        vertices.resize(maxVertices * 2, 0);
        maxVertices *= 2;
    }

    vertices[vertexI] = quad;

    vertexI++;
}

static inline const uint64_t getQuad(uint64_t x, uint64_t y, uint64_t z, uint64_t w, uint64_t h, uint64_t type) {
    return (type << 32) | (h << 24) | (w << 18) | (z << 12) | (y << 6) | x;
}

constexpr uint16_t P_MASK = ~(1ull << 15 | 1);

uint16_t* fillOpaqueMask(const uint32_t* voxels) {
    uint16_t* opaqueMask = new uint16_t[CS_P2];

    // Reset: chaque uint16_t va être reconstruit bit par bit ci-dessous.
    for (int i = 0; i < CS_P2; i++) {
        opaqueMask[i] = 0;
    }

    // Le 3e axe (c) est compressé dans les bits d'un même uint64_t.
    // ATTENTION: la permutation exacte ici n'est PAS arbitraire — elle doit
    // correspondre à celle que le greedy meshing utilise pour relire le voxel
    // (voir getAxisIndex). Vérifiée par trace manuelle : opaqueMask[a*CS_P+b]
    // bit c doit correspondre à voxels[voxelIndex(c, a, b)].
    for (int a = 0; a < CS_P; a++) {
        const int aCS_P = a * CS_P;

        for (int b = 0; b < CS_P; b++) {
            uint16_t bits = 0;

            for (int c = 0; c < CS_P; c++) {
                if (voxels[voxelIndex(c, a, b)] != 0) {
                    bits |= (1ull << c);
                }
            }

            opaqueMask[aCS_P + b] = bits;
        }
    }

	return opaqueMask;
}

void mesh(const uint32_t* voxels, MeshData& meshData) {
    meshData.vertexCount = 0;
    int vertexI = 0;

    uint16_t* opaqueMask = meshData.opaqueMask;
    uint16_t* faceMasks = meshData.faceMasks;
    uint8_t* forwardMerged = meshData.forwardMerged;
    uint8_t* rightMerged = meshData.rightMerged;

    // Hidden face culling
    for (int a = 1; a < CS_P - 1; a++) {
        const int aCS_P = a * CS_P;

        for (int b = 1; b < CS_P - 1; b++) {
            const uint16_t columnBits = opaqueMask[(a * CS_P) + b] & P_MASK;
            const int baIndex = (b - 1) + (a - 1) * CS;
            const int abIndex = (a - 1) + (b - 1) * CS;

            faceMasks[baIndex + 0 * CS_2] = (columnBits & ~opaqueMask[aCS_P + CS_P + b]) >> 1;
            faceMasks[baIndex + 1 * CS_2] = (columnBits & ~opaqueMask[aCS_P - CS_P + b]) >> 1;

            faceMasks[abIndex + 2 * CS_2] = (columnBits & ~opaqueMask[aCS_P + (b + 1)]) >> 1;
            faceMasks[abIndex + 3 * CS_2] = (columnBits & ~opaqueMask[aCS_P + (b - 1)]) >> 1;

            faceMasks[baIndex + 4 * CS_2] = columnBits & ~(opaqueMask[aCS_P + b] >> 1);
            faceMasks[baIndex + 5 * CS_2] = columnBits & ~(opaqueMask[aCS_P + b] << 1);
        }
    }

    // Greedy meshing faces 0-3
    for (int face = 0; face < 4; face++) {
        const int axis = face / 2;

        const int faceVertexBegin = vertexI;

        for (int layer = 0; layer < CS; layer++) {
            const int bitsLocation = layer * CS + face * CS_2;

            for (int forward = 0; forward < CS; forward++) {
                uint16_t bitsHere = faceMasks[forward + bitsLocation];
                if (bitsHere == 0) continue;

                const uint16_t bitsNext = forward + 1 < CS ? faceMasks[(forward + 1) + bitsLocation] : 0;

                uint8_t rightMerged = 1;
                while (bitsHere) {
                    unsigned long bitPos;
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif

                    const uint32_t type = voxels[getAxisIndex(axis, forward + 1, bitPos + 1, layer + 1)];
                    uint8_t& forwardMergedRef = forwardMerged[bitPos];

                    if ((bitsNext >> bitPos & 1) && type == voxels[getAxisIndex(axis, forward + 2, bitPos + 1, layer + 1)]) {
                        forwardMergedRef++;
                        bitsHere &= ~(1ull << bitPos);
                        continue;
                    }

                    for (int right = bitPos + 1; right < CS; right++) {
                        if (!(bitsHere >> right & 1) || forwardMergedRef != forwardMerged[right] || type != voxels[getAxisIndex(axis, forward + 1, right + 1, layer + 1)]) break;
                        forwardMerged[right] = 0;
                        rightMerged++;
                    }
                    bitsHere &= ~((1ull << (bitPos + rightMerged)) - 1);

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

        for (int forward = 0; forward < CS; forward++) {
            const int bitsLocation = forward * CS + face * CS_2;
            const int bitsForwardLocation = (forward + 1) * CS + face * CS_2;

            for (int right = 0; right < CS; right++) {
                uint16_t bitsHere = faceMasks[right + bitsLocation];
                if (bitsHere == 0) continue;

                const uint16_t bitsForward = forward < CS - 1 ? faceMasks[right + bitsForwardLocation] : 0;
                const uint16_t bitsRight = right < CS - 1 ? faceMasks[right + 1 + bitsLocation] : 0;
                const int rightCS = right * CS;

                while (bitsHere) {
                    unsigned long bitPos;
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif

                    bitsHere &= ~(1ull << bitPos);

                    const uint32_t type = voxels[getAxisIndex(axis, right + 1, forward + 1, bitPos)];
                    uint8_t& forwardMergedRef = forwardMerged[rightCS + (bitPos - 1)];
                    uint8_t& rightMergedRef = rightMerged[bitPos - 1];

                    if (rightMergedRef == 0 && (bitsForward >> bitPos & 1) && type == voxels[getAxisIndex(axis, right + 1, forward + 2, bitPos)]) {
                        forwardMergedRef++;
                        continue;
                    }

                    if ((bitsRight >> bitPos & 1) && forwardMergedRef == forwardMerged[(rightCS + CS) + (bitPos - 1)] && type == voxels[getAxisIndex(axis, right + 2, forward + 1, bitPos)]) {
                        forwardMergedRef = 0;
                        rightMergedRef++;
                        continue;
                    }

                    const uint8_t meshLeft = right - rightMergedRef;
                    const uint8_t meshFront = forward - forwardMergedRef;
                    const uint8_t meshUp = bitPos - 1 + (~face & 1);

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
}