// ============================================================
// entity_shader.slang
// Dessine un pavé vertical (boîte) par entité, géométrie générée
// entièrement dans le vertex shader à partir de SV_VertexID — pas de
// vertex/index buffer de géométrie, seulement un attribut de position
// par instance (voir en bas du fichier).
//
// Draw attendu côté C++ :
//   vkCmdDraw(cmdBuffer, 36, NUM_ENTITY, 0, 0);
//   (36 = 6 faces * 6 sommets, 2 triangles par face, pas d'index buffer)
// ============================================================

struct CameraUniform {
    float4x4 view_proj;
    float3   view_position;
    float    _padding;
};

struct VSOutput {
    float4 clip_position : SV_Position;
    float3 world_position;
    float3 normal;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraUniform> camera;

// ------------------------------------------------------------
// Dimensions du pavé (ajuste selon la taille voulue de tes entités)
// ------------------------------------------------------------
static const float BOX_WIDTH  = 0.6;  // étendue en X et Z
static const float BOX_HEIGHT = 1.8;  // hauteur en Y

// ------------------------------------------------------------
// 8 coins locaux d'un cube unité : base en Y=0 (pour poser l'entité
// au sol à sa position, pas centrée verticalement), centré en XZ.
// ------------------------------------------------------------
static const float3 corners[8] = {
    float3(-0.5, 0.0, -0.5), // 0: bas  arrière gauche
    float3( 0.5, 0.0, -0.5), // 1: bas  arrière droite
    float3( 0.5, 0.0,  0.5), // 2: bas  avant   droite
    float3(-0.5, 0.0,  0.5), // 3: bas  avant   gauche
    float3(-0.5, 1.0, -0.5), // 4: haut arrière gauche
    float3( 0.5, 1.0, -0.5), // 5: haut arrière droite
    float3( 0.5, 1.0,  0.5), // 6: haut avant   droite
    float3(-0.5, 1.0,  0.5), // 7: haut avant   gauche
};

// Les 6 faces, chacune listée par 4 indices de coins, dans un ordre
// anti-horaire vu de l'EXTÉRIEUR (winding correct pour face culling futur).
static const int faceCorners[6][4] = {
    { 1, 0, 4, 5 }, // arrière (-Z)
    { 3, 2, 6, 7 }, // avant   (+Z)
    { 0, 3, 7, 4 }, // gauche  (-X)
    { 2, 1, 5, 6 }, // droite  (+X)
    { 7, 6, 5, 4 }, // dessus  (+Y)
    { 0, 1, 2, 3 }, // dessous (-Y)
};

static const float3 faceNormals[6] = {
    float3( 0.0,  0.0, -1.0), // arrière
    float3( 0.0,  0.0,  1.0), // avant
    float3(-1.0,  0.0,  0.0), // gauche
    float3( 1.0,  0.0,  0.0), // droite
    float3( 0.0,  1.0,  0.0), // dessus
    float3( 0.0, -1.0,  0.0), // dessous
};

// Découpe d'un quad (4 coins) en 2 triangles : (0,1,2) puis (0,2,3),
// même convention que shader.slang pour le terrain.
static const int cornerIdx[6] = { 0, 1, 2, 0, 2, 3 };

[shader("vertex")]
VSOutput vs_main(
    uint   vertexID     : SV_VertexID,
    float3 entityPosition : POSITION
)
{
    uint faceIdx      = vertexID / 6;
    uint triLocalIdx  = vertexID % 6;
    int  localCorner  = faceCorners[faceIdx][cornerIdx[triLocalIdx]];

    float3 local = corners[localCorner];
    local.x *= BOX_WIDTH;
    local.z *= BOX_WIDTH;
    local.y *= BOX_HEIGHT;

    float3 worldPos = local + entityPosition;

    VSOutput output;
    output.world_position = worldPos;
    output.normal         = faceNormals[faceIdx];
    output.clip_position  = mul(camera.view_proj, float4(worldPos, 1.0));

    return output;
}

// ============================================================
// FRAGMENT SHADER
// Ombrage simple basé sur la normale (pas de texture pour l'instant,
// EntityState n'a pas de descriptor set de texture) : une lumière
// directionnelle fixe + un peu d'ambiant, pour distinguer les faces.
// ============================================================
[shader("fragment")]
float4 ps_main(VSOutput v) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}
