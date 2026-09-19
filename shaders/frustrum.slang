// ============================================================
// Définitions des constantes et structures
// ============================================================
#define CHUNK_AXIS1_SIZE 64     // À adapter selon ton projet (ex: 16.0f ou 32.0f)
#define MAX_FACE 6              // Nombre maximal de faces par chunk

struct IndirectDrawCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
}

struct CameraUniform {
    float4x4 view_proj;
    float3 view_position;
    float _padding;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraUniform> camera;
[[vk::binding(1, 0)]]
RWStructuredBuffer<IndirectDrawCommand> cmds;
[[vk::binding(2, 0)]]
StructuredBuffer<uint2> sceneBuffer;

// ============================================================
// Fonction pour extraire les plans du frustum à partir de view_proj
// ============================================================
void extractFrustumPlanes(float4x4 viewProj, out float4[6] planes) {
    // IMPORTANT : ce moteur utilise la convention "vecteur-colonne" de GLM/Vulkan
    // (clip = viewProj * pos). Dans cette convention, chaque plan du frustum
    // s'obtient en combinant les LIGNES de la matrice (viewProj[i] retourne
    // directement la ligne i en Slang, comme en HLSL), PAS en combinant des
    // composantes prises colonne par colonne comme le faisait l'ancienne version
    // (qui correspondait à la convention "vecteur-ligne" de Direct3D, pos * M).
    float4 row0 = viewProj[0];
    float4 row1 = viewProj[1];
    float4 row2 = viewProj[2];
    float4 row3 = viewProj[3];

    planes[0] = row3 + row0; // Left   (x >= -w)
    planes[1] = row3 - row0; // Right  (x <=  w)
    planes[2] = row3 + row1; // Bottom (y >= -w)
    planes[3] = row3 - row1; // Top    (y <=  w)

    // Plans Near/Far : avec GLM_FORCE_DEPTH_ZERO_TO_ONE, le NDC Vulkan a
    // z dans [0, w] (et non [-w, w] comme en OpenGL). La condition Near
    // devient simplement z >= 0, donc le plan est juste "row2" (pas row3+row2,
    // qui est la formule OpenGL et qui plaçait votre near plane au mauvais endroit).
    planes[4] = row2;        // Near (z >= 0)
    planes[5] = row3 - row2; // Far  (z <= w)
}

// ============================================================
// Fonction pour tester si un AABB est complètement à l'extérieur du frustum
// ============================================================
bool isAABBOutsideFrustum(float3 aabbMin, float3 aabbMax, float4[6] planes) {
    // Tester les 8 sommets de l'AABB contre chaque plan
    float3 aabbVertices[8] = {
        float3(aabbMin.x, aabbMin.y, aabbMin.z), // 0
        float3(aabbMax.x, aabbMin.y, aabbMin.z), // 1
        float3(aabbMin.x, aabbMax.y, aabbMin.z), // 2
        float3(aabbMax.x, aabbMax.y, aabbMin.z), // 3
        float3(aabbMin.x, aabbMin.y, aabbMax.z), // 4
        float3(aabbMax.x, aabbMin.y, aabbMax.z), // 5
        float3(aabbMin.x, aabbMax.y, aabbMax.z), // 6
        float3(aabbMax.x, aabbMax.y, aabbMax.z)  // 7
    };

    for (int i = 0; i < 6; i++) {
        float4 plane = planes[i];
        bool allOutside = true;
        for (int j = 0; j < 8; j++) {
            float distance = dot(plane.xyz, aabbVertices[j]) + plane.w;
            if (distance >= 0) { // Si UN point est à l'intérieur, le chunk n'est pas complètement à l'extérieur
                allOutside = false;
                break;
            }
        }
        if (allOutside) {
            return true; // Tous les sommets sont à l'extérieur de ce plan → chunk complètement à l'extérieur
        }
    }
    return false; // Le chunk est au moins partiellement à l'intérieur
}

// ============================================================
// Shader principal : Frustum Culling
// ============================================================
[numthreads(1, 1, 1)]
void frustum_culling(uint3 dispatchThreadId : SV_DispatchThreadID) {
    // Index du chunk actuel
    uint chunkSlot = dispatchThreadId.x;
    uint cmdBaseIndex = chunkSlot * 6; // Chaque chunk a 6 commandes (1 par face)

    // --- Étape 1 : Extraire la position du chunk à partir de firstVertex ---
    uint32_t* firstVertex = &cmds[cmdBaseIndex].firstVertex;

    // Décoder chunkPos depuis firstVertex (format : [i(3 bits) | x(8 bits) | y(8 bits) | z(8 bits)])
    int3 chunkPos = int3(
        int8_t((*firstVertex >> 16) & 0xFF),
        int8_t((*firstVertex >>  8) & 0xFF),
        int8_t((*firstVertex) & 0xFF)
    );

    // --- Étape 2 : Calculer l'AABB du chunk ---
    int3 chunkMin = chunkPos * CHUNK_AXIS1_SIZE;
    int3 chunkMax = chunkMin + int3(CHUNK_AXIS1_SIZE, CHUNK_AXIS1_SIZE, CHUNK_AXIS1_SIZE);

    // --- Étape 3 : Extraire les plans du frustum ---
    float4 frustumPlanes[6];
    extractFrustumPlanes(camera.view_proj, frustumPlanes);

    // --- Étape 4 : Tester si le chunk est complètement à l'extérieur du frustum ---
    bool isOutside = isAABBOutsideFrustum(chunkMin, chunkMax, frustumPlanes);

    // --- Étape 5 : Mettre à jour les 6 commandes du chunk ---
    if (isOutside) {
        // Le chunk est complètement à l'extérieur : instancesCount = 0 pour toutes ses commandes
        for (int i = 0; i < 6; i++) {
            cmds[cmdBaseIndex + i].instanceCount = 0;
        }
    } else {
        for (int i = 0; i < 5; i++) {
            cmds[cmdBaseIndex + i].instanceCount = cmds[cmdBaseIndex + i + 1].firstInstance - cmds[cmdBaseIndex + i].firstInstance;
        }

        uint32_t* instanceCount = &cmds[cmdBaseIndex + 5].instanceCount;
        while (*instanceCount < 10000) {
            if (sceneBuffer[cmds[cmdBaseIndex + 5].firstInstance + *instanceCount].x == 0 && sceneBuffer[cmds[cmdBaseIndex + 5].firstInstance + *instanceCount].y == 0) break;
            (*instanceCount)++;
        }
    }
}