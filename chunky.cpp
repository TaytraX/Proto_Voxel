#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include "chunky.h"
#include <algorithm>
#include "engine_constants.hpp"
#include <stdexcept>

namespace {
    // Convention : voxels[i] == 0 -> voxel vide.
    // voxels[i] != 0 -> voxel plein, et voxels[i] - 1 donne le materialID (1 ou 2).
    constexpr uint32_t kMaterialCount = 2;

    inline uint32_t materialOf(uint32_t voxel) {
        return voxel;
    }
} // namespace

enum FaceDirection {
    TOP = 5,
    BOTTOM = 2,
    RIGHT = 6,
    LEFT = 1,
    FRONT = 3,
    BACK = 4
};

void addSpecialFace(ChunkMesh& mesh, int x, int y, int z, FaceDirection face, glm::vec2 scale = glm::vec2(1.0, 1.0)) {
    uint32_t quad = 0;

    quad |= uint32_t(scale.y) << 20;
    quad |= uint32_t(scale.x) << 16;
    quad |= uint32_t(x) << 12;
    quad |= uint32_t(y) << 8;
    quad |= uint32_t(z) << 4;
    quad |= uint32_t(face) << 0;

    mesh.vertices.push_back(quad);
}

namespace {
    bool theTopIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (y >= Ty - 1) return true;
        return voxels[(y + 1) * (Tx * Tz) + z * Tx + x] == 0;
    }

    bool theBottomIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (y <= 0) return true;
        return voxels[(y - 1) * (Tx * Tz) + z * Tx + x] == 0;
    }

    bool theRightIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (x >= Tx - 1) return true;
        return voxels[(y * (Tx * Tz) + z * Tx + (x + 1))] == 0;
    }

    bool theLeftIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (x <= 0) return true;
        return voxels[(y * (Tx * Tz) + z * Tx + (x - 1))] == 0;
    }

    bool theFrontIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (z >= Tz - 1) return true;
        return voxels[(y * (Tx * Tz) + (z + 1) * Tx + x)] == 0;
    }

    bool theBackIsVoid(const uint32_t* voxels, int x, int y, int z) {
        if (z <= 0) return true;
        return voxels[(y * (Tx * Tz) + (z - 1) * Tx + x)] == 0;
    }

    void addFace(ChunkMesh& mesh, int x, int y, int z, FaceDirection face) {
        uint32_t val = 0;

        val |= uint32_t(1) << 20;
        val |= uint32_t(1) << 16;
        val |= uint32_t(x) << 12;
        val |= uint32_t(y) << 8;
        val |= uint32_t(z) << 4;
        val |= uint32_t(face) << 0;

        mesh.vertices.push_back(val);
    }
}

ChunkMesh generateChunkMesh(const uint32_t* voxels)
{
    ChunkMesh mesh;

    // Pire cas : chaque voxel plein expose ses 6 faces -> 6 vertices (non-indexés) par face
    const size_t maxVoxels = static_cast<size_t>(Tx) * Ty * Tz;
    mesh.vertices.reserve(maxVoxels * 6 * 6);
    mesh.subMeshs.reserve(kMaterialCount);

    // On parcourt une fois par matériel afin que les vertices d'un même
    // matériel restent contigus dans mesh.vertices, ce qui permet de décrire
    // chaque sous-maillage par une simple plage [firstIndex, firstIndex+indexCount[
    // (ici comptée en vertices, à consommer via un draw non-indexé).

    bool already = false;

    for (uint32_t materialID = 1; materialID <= kMaterialCount; ++materialID)
    {
        const uint32_t firstIndex = static_cast<uint32_t>(mesh.vertices.size());

        for (uint32_t i = 0; i < Tx * Ty * Tz; ++i)
        {
            if (voxels[i] == 0)
                continue; // voxel vide -> pas de géométrie

            if (voxels[i] != materialID)
                continue; // ce voxel appartient à un autre matériel, traité à une autre itération

            int x = static_cast<int>(i % Tx);
            int z = static_cast<int>((i / Tx) % Tz);
            int y = static_cast<int>(i / (Tx * Tz));

            if (!already) {
                addSpecialFace(mesh, 8, 2, 8, BOTTOM);
                addSpecialFace(mesh, 8, 2, 8, RIGHT, glm::vec2(1.0, 3.0));
                addSpecialFace(mesh, 8, 2, 8, FRONT, glm::vec2(1.0, 3.0));
                already = true;
            }

            if (theRightIsVoid(voxels, x, y, z))  addFace(mesh, x, y, z, RIGHT);
            if (theLeftIsVoid(voxels, x, y, z))   addFace(mesh, x, y, z, LEFT);
            if (theTopIsVoid(voxels, x, y, z))    addFace(mesh, x, y, z, TOP);
            if (theBottomIsVoid(voxels, x, y, z)) addFace(mesh, x, y, z, BOTTOM);
            if (theBackIsVoid(voxels, x, y, z))   addFace(mesh, x, y, z, BACK);
            if (theFrontIsVoid(voxels, x, y, z))  addFace(mesh, x, y, z, FRONT);
        }

        const uint32_t indexCount = static_cast<uint32_t>(mesh.vertices.size()) - firstIndex;
        if (indexCount == 0)
            continue; // aucun voxel de ce matériel dans le chunk -> pas de sous-maillage

        SubMesh sub;
        sub.firstQuad = firstIndex;
        sub.quadCount = indexCount;
        sub.materialID = static_cast<uint16_t>(materialID);
        mesh.subMeshs.push_back(sub);
    }

    return mesh;
}