#include "mesher.h"
#include <iostream>
#include <array>

// Remappe les paramètres génériques (a, b, c) de l'algorithme — dont le sens
// change selon l'axe balayé — vers de vraies coordonnées (x, y, z), puis
// appelle voxelIndex(). C'est mathématiquement équivalent à l'ancienne
// version codée en dur (b + a*CS_P + c*CS_P2, etc.), mais explicite désormais
// la correspondance avec ta formule au lieu de la laisser implicite.
static inline const size_t getAxisIndex(const int axis, const int a, const int b, const int c) {
    if (axis == 0)      return voxelIndex(a, c, b);
    else if (axis == 1) return voxelIndex(c, a, b);
    else                return voxelIndex(a, b, c);
}

static inline const void insertQuad(std::vector<uint64_t>& vertices, uint64_t quad, int& vertexI, int& maxVertices) {
    if (vertexI >= maxVertices - 6) {
        vertices.resize(maxVertices * 2, 0);
        maxVertices *= 2;
    }

    vertices[vertexI] = quad;

    vertexI++;
}

static inline const uint64_t getQuad(uint64_t x, uint64_t y, uint64_t z, uint64_t h, uint64_t type) {
    // data.x (bits 0-31)  : x(7) y(7) z(7) w(7) — 28/32 bits utilisés
    // data.y (bits 32-63) : type(24) h(7)       — 31/32 bits utilisés
    return (type << 32) | (h << 21) | (z << 14) | (y << 7) | x;
}

constexpr uint64_t P_MASK = UINT64_MAX;

uint64_t* fillOpaqueMask(const std::array<uint32_t, CHUNK_AXIS3_SIZE>& voxels) {
    uint64_t* opaqueMask = new uint64_t[CHUNK_AXIS2_SIZE] { 0 };

    for (int a = 0; a < CHUNK_AXIS1_SIZE; a++) {
        const int aCS_P = a * CHUNK_AXIS1_SIZE;

        for (int b = 0; b < CHUNK_AXIS1_SIZE; b++) {
            uint64_t bits = 0;

            for (int c = 0; c < CHUNK_AXIS1_SIZE; c++) {
                if (voxels[voxelIndex(b, a, c)] != 0) bits |= (1ull << c);
                
            }

            opaqueMask[aCS_P + b] = bits;
        }
    }

    return opaqueMask;
}

// Parité de la colonne hexagonale q = position du bit (z du mesher)
constexpr uint64_t EVEN_BITS = 0x5555555555555555ull; // q pair
constexpr uint64_t ODD_BITS = 0xAAAAAAAAAAAAAAAAull; // q impair

void mesh(const std::array<uint32_t, CHUNK_AXIS3_SIZE>& voxels, MeshData& meshData) {
    meshData.vertexCount = 0;
    int vertexI = 0;

    uint64_t* opaqueMask = meshData.opaqueMask;
    uint64_t* faceMasks = meshData.faceMasks;
    uint8_t* forwardMerged = meshData.forwardMerged;
    uint8_t* rightMerged = meshData.rightMerged;

    // Hidden face culling : prisme hexagonal (8 faces)
    for (int a = 0; a < CHUNK_AXIS1_SIZE; a++) {          // a = y
        const int aCS_P = a * CHUNK_AXIS1_SIZE;

        for (int b = 0; b < CHUNK_AXIS1_SIZE; b++) {      // b = r (x du mesher)
            const uint64_t cur = opaqueMask[aCS_P + b];
            const int baIndex = b + a * CHUNK_AXIS1_SIZE;

            // Voisins verticaux
            const uint64_t negA = (a > 0) ? opaqueMask[aCS_P - CHUNK_AXIS1_SIZE + b] : 0;
            const uint64_t posA = (a < CHUNK_AXIS1_SIZE - 1) ? opaqueMask[aCS_P + CHUNK_AXIS1_SIZE + b] : 0;

            // Colonnes de la rangée r+1 et r-1 (même q)
            const uint64_t posB = (b < CHUNK_AXIS1_SIZE - 1) ? opaqueMask[aCS_P + (b + 1)] : 0;
            const uint64_t negB = (b > 0) ? opaqueMask[aCS_P + (b - 1)] : 0;

            // Voisins diagonaux (colonnes q+1 / q-1), disposition "odd-q" :
            // les colonnes impaires sont décalées de +0.5 sur z.
            //   q pair   : haut = rangée r,   bas = rangée r-1
            //   q impair : haut = rangée r+1, bas = rangée r
            // >> 1 ramène le bit q+1 en position q, << 1 ramène le bit q-1.
            const uint64_t nextUp = ((cur >> 1) & EVEN_BITS) | ((posB >> 1) & ODD_BITS);
            const uint64_t nextDown = ((negB >> 1) & EVEN_BITS) | ((cur >> 1) & ODD_BITS);
            const uint64_t prevUp = ((cur << 1) & EVEN_BITS) | ((posB << 1) & ODD_BITS);
            const uint64_t prevDown = ((negB << 1) & EVEN_BITS) | ((cur << 1) & ODD_BITS);

            faceMasks[baIndex + 0 * CHUNK_AXIS2_SIZE] = cur & ~posA;      // 0 : dessus   (+Y)
            faceMasks[baIndex + 1 * CHUNK_AXIS2_SIZE] = cur & ~negA;      // 1 : dessous  (-Y)
            faceMasks[baIndex + 2 * CHUNK_AXIS2_SIZE] = cur & ~posB;      // 2 : arête plate +z
            faceMasks[baIndex + 3 * CHUNK_AXIS2_SIZE] = cur & ~negB;      // 3 : arête plate -z
            faceMasks[baIndex + 4 * CHUNK_AXIS2_SIZE] = cur & ~nextUp;    // 4 : +x, +z
            faceMasks[baIndex + 5 * CHUNK_AXIS2_SIZE] = cur & ~nextDown;  // 5 : +x, -z
            faceMasks[baIndex + 6 * CHUNK_AXIS2_SIZE] = cur & ~prevUp;    // 6 : -x, +z
            faceMasks[baIndex + 7 * CHUNK_AXIS2_SIZE] = cur & ~prevDown;  // 7 : -x, -z
        }
    }

    for (int face = 0; face < 2; face++) {
        const int faceVertexBegin = vertexI;

        for (int layer = 0; layer < CHUNK_AXIS1_SIZE; layer++) {          // layer   = y
            const int bitsLocation = layer * CHUNK_AXIS1_SIZE + face * CHUNK_AXIS2_SIZE;

            for (int forward = 0; forward < CHUNK_AXIS1_SIZE; forward++) { // forward = x
                uint64_t bitsHere = faceMasks[forward + bitsLocation];

                while (bitsHere) {
                    unsigned long bitPos;                                  // bitPos  = z
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif
                    bitsHere &= bitsHere - 1; // efface le bit de poids faible

                    const uint32_t type = voxels[voxelIndex(forward, layer, bitPos)];

                    // Même convention que l'ancien code :
                    //  face 0 (+Y) : y = layer + 1
                    //  face 1 (-Y) : y = layer, x = forward + 1 (longueur = 1)
                    const uint64_t quad = getQuad(
                        forward + (face == 1 ? 1 : 0),  // x
                        layer + (face == 0 ? 1 : 0),  // y
                        bitPos,                       // z
                        1,                            // h
                        type);

                    insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
                }
            }
        }

        meshData.faceVertexBegin[face] = faceVertexBegin;
        meshData.faceVertexLength[face] = vertexI - faceVertexBegin;
    }

    for (int face = 2; face < 8; face++) {
        const int faceVertexBegin = vertexI;
        const int faceBase = face * CHUNK_AXIS2_SIZE;

        for (int b = 0; b < CHUNK_AXIS1_SIZE; b++) {                // b = x du mesher
            for (int y = 0; y < CHUNK_AXIS1_SIZE; y++) {
                uint64_t bitsHere = faceMasks[faceBase + y * CHUNK_AXIS1_SIZE + b];
                if (bitsHere == 0) continue;

                const uint64_t bitsNext = (y + 1 < CHUNK_AXIS1_SIZE)
                    ? faceMasks[faceBase + (y + 1) * CHUNK_AXIS1_SIZE + b]
                    : 0;

                while (bitsHere) {
                    unsigned long bitPos;                           // bitPos = z du mesher
#ifdef _MSC_VER
                    _BitScanForward64(&bitPos, bitsHere);
#else
                    bitPos = __builtin_ctzll(bitsHere);
#endif
                    bitsHere &= bitsHere - 1;

                    const uint32_t type = voxels[voxelIndex(b, y, bitPos)];
                    uint8_t& run = forwardMerged[bitPos];           // longueur déjà fusionnée sous y

                    // La face continue au-dessus avec le même type : on prolonge
                    if ((bitsNext >> bitPos & 1) && type == voxels[voxelIndex(b, y + 1, bitPos)]) {
                        run++;
                        continue;
                    }

                    // Fin de la colonne : on émet un seul quad
                    const uint8_t meshY = y - run;                // base de la colonne
                    const uint8_t height = run + 1;                // nombre de blocs empilés
                    run = 0;

                    const uint64_t quad = getQuad(b, meshY, bitPos, height, type);
                    insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
                }
            }
        }

        meshData.faceVertexBegin[face] = faceVertexBegin;
        meshData.faceVertexLength[face] = vertexI - faceVertexBegin;
    }

    meshData.vertexCount = vertexI;
    std::fill(meshData.vertices->begin() + vertexI, meshData.vertices->end(), 0ull);
}