enum Map {
    ALBEDO = 0,
    NORMAL = 1,
    DISPLACEMENT = 2
}

uint getMaterial(uint iD, Map usage) {
    if (iD == 1) return (uint)usage;
    else {
        switch (usage) {
        case Map::ALBEDO: return 3;
        case Map::NORMAL: return 4;
        default: return 5;
        }
    }
}

struct CameraUniform {
    float4x4 view_proj;
    float3 view_position;
    float _padding;
};

// Sortie du Vertex Shader = entrée du Hull Shader (control points bruts, pas de displacement ici)
// On transmet position + normal + uv tels quels : ils sont déjà corrects et
// cohérents par face côté mesher (CPU) ; pas besoin de les recalculer ici.
struct HSInput {
    float3 world_position;
    float3 normal;
    float2 uv;

    uint materialID;
};

struct HSPatchConstants {
    float edgeTess[3]  : SV_TessFactor;
    float insideTess   : SV_InsideTessFactor;
};

struct VertexInput {
    float3 position;
    float3 normal;
    float2 uv;
};

struct VertexOutput {
    float4 clip_position  : SV_Position;
    float3 world_position;
    float3 world_normal;
    float2 uv;

    nointerpolation uint materialID;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraUniform> camera;
[[vk::binding(0, 1)]]
Sampler2D material[];

static const float DISPLACEMENT_SCALE = 0.1;
static const float TEXEL_SIZE      = 1.0 / 2048.0;
static const float NORMAL_STRENGTH = 6.0;
static const float TEXTURE_SCALE   = 16.0; // ajuste pour le tiling triplanaire

// Tessellation adaptative selon la distance caméra
static const float TESS_MIN_DIST   = 2.0;
static const float TESS_MAX_DIST   = 25.0;
static const float TESS_MAX_FACTOR = 32.0;

static const float3 normalLookup[6] = {
    float3(0, 1, 0),
    float3(0, -1, 0),
    float3(1, 0, 0),
    float3(-1, 0, 0),
    float3(0, 0, 1),
    float3(0, 0, -1)
};

static const int flipLookup[6] = { 1, -1, -1, 1, -1, 1 };

// ---------------------------------------------------------------------
// 1) VERTEX SHADER
// Passe position, normale et uv tels quels en world space. Ces valeurs
// sont déjà correctes par face (calculées côté mesher) : la normale de
// face est constante par vertex du quad, l'uv va de (0,0) à (1,1).
// Le domain shader ne les recalcule plus, il les interpole seulement.
// ---------------------------------------------------------------------
[shader("vertex")]
HSInput vs_main(uint2 data, uint vertexID: SV_VertexID, uint face: SV_StartVertexLocation)
{
    // Coins du quad (0,0) (1,0) (1,1) (0,1), indexés dans l'ordre du split
    // diagonal 0,1,2,0,2,3 (2 triangles pour couvrir le quad en 6 vertices).
    static const int cornerW[4] = { 0, 1, 1, 0 };
    static const int cornerH[4] = { 0, 0, 1, 1 };
    static const int cornerIdx[6] = { 0, 1, 2, 0, 2, 3 };

    uint faceIdx = face - 1;

    float3 origin = float3(
        (data.x >> 0)  & 0x3F,
        (data.x >> 6)  & 0x3F,
        (data.x >> 12) & 0x3F
    );

    int w = int((data.x >> 18u) & 31u), h = int((data.x >> 24u) & 31u);
    uint wDir = (faceIdx & 2) >> 1, hDir = 2 - (faceIdx >> 2);

    int c = cornerIdx[vertexID];
    int wMod = cornerW[c];
    int hMod = cornerH[c];

    origin[wDir] += (w * wMod * flipLookup[faceIdx]);
    origin[hDir] += (h * hMod);

    HSInput output;
    output.world_position = origin;
    output.normal = normalLookup[faceIdx];
    output.uv = float2(wMod * w, hMod * h);
    output.materialID = data.y;

    return output;
}

// ---------------------------------------------------------------------
// 2) HULL SHADER
// ---------------------------------------------------------------------
float computeTessFactor(float3 worldPos)
{
    float dist = distance(camera.view_position, worldPos);
    float t = saturate((TESS_MAX_DIST - dist) / (TESS_MAX_DIST - TESS_MIN_DIST));
    return lerp(1.0, TESS_MAX_FACTOR, t);
}

[shader("hull")]
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("hs_patch_constants")]
HSInput hs_main(InputPatch<HSInput, 3> patch, uint pointId : SV_OutputControlPointID)
{
    return patch[pointId];
}

HSPatchConstants hs_patch_constants(InputPatch<HSInput, 3> patch)
{
    HSPatchConstants pc;

    float tessA = computeTessFactor(patch[0].world_position);
    float tessB = computeTessFactor(patch[1].world_position);
    float tessC = computeTessFactor(patch[2].world_position);

    pc.edgeTess[0] = (tessB + tessC) * 0.5;
    pc.edgeTess[1] = (tessC + tessA) * 0.5;
    pc.edgeTess[2] = (tessA + tessB) * 0.5;
    pc.insideTess  = (tessA + tessB + tessC) / 3.0;

    return pc;
}

// ---------------------------------------------------------------------
// 3) DOMAIN SHADER
// La normale vient directement du mesh (transmis depuis le vertex shader).
// textureUV (triplanaire, basé sur la position monde) sert à échantillonner
// les textures. Deux faces adjacentes partagent le même axe de projection
// le long de leur arête commune, donc la texture continue naturellement
// d'une face à l'autre du même voxel au lieu de se répéter identique sur
// chaque face.
// ---------------------------------------------------------------------
float2 computeTriplanarUV(float3 worldPos, float3 faceNormal)
{
    float3 absN = abs(faceNormal);

    // Sélectionne le plan de projection selon l'axe dominant de la normale
    if (absN.x >= absN.y && absN.x >= absN.z)
        return worldPos.yz * TEXTURE_SCALE;
    else if (absN.y >= absN.x && absN.y >= absN.z)
        return worldPos.xz * TEXTURE_SCALE;
    else
        return worldPos.xy * TEXTURE_SCALE;
}

[shader("domain")]
[domain("tri")]
[partitioning("fractional_odd")]
VertexOutput ds_main(
    HSPatchConstants patchConstants,
    float3 barycentric: SV_DomainLocation,
    const OutputPatch<HSInput, 3> patch)
{
    VertexOutput output;

    // Position interpolée
    float3 worldPos =
        patch[0].world_position * barycentric.x
        + patch[1].world_position * barycentric.y
        + patch[2].world_position * barycentric.z;

    // Normale de face (flat shading)
    float3 faceNormal = patch[0].normal;

    // UV interpolés
    float2 uv =
        patch[0].uv * barycentric.x
        + patch[1].uv * barycentric.y
        + patch[2].uv * barycentric.z;

    uint materialID = uint(
        patch[0].materialID * barycentric.x
        + patch[1].materialID * barycentric.y
        + patch[2].materialID * barycentric.z);

    // Height map
    float height = material[getMaterial(materialID, Map::DISPLACEMENT)]
                       .SampleLevel(uv, 0).r;

    // Déplacement
    worldPos += faceNormal * (height * DISPLACEMENT_SCALE);

    output.world_position = worldPos;
    output.world_normal = faceNormal;
    output.uv = uv;
    output.clip_position = mul(camera.view_proj, float4(worldPos, 1.0));
    output.materialID = materialID;

    return output;
}

// ---------------------------------------------------------------------
// 4) PIXEL SHADER
// Combine la normale géométrique (issue de la tessellation) avec une
// normale de détail haute fréquence dérivée du height map, pour capter
// aussi le micro-relief que même une tessellation élevée ne peut pas
// entièrement représenter.
// ---------------------------------------------------------------------
float3 computeHeightMapNormal(float2 uv)
{
    float hL = material[2].SampleLevel(uv - float2(TEXEL_SIZE, 0), 0).r;
    float hR = material[2].SampleLevel(uv + float2(TEXEL_SIZE, 0), 0).r;
    float hD = material[2].SampleLevel(uv - float2(0, TEXEL_SIZE), 0).r;
    float hU = material[2].SampleLevel(uv + float2(0, TEXEL_SIZE), 0).r;

    float3 n;
    n.x = (hL - hR) * NORMAL_STRENGTH;
    n.y = (hD - hU) * NORMAL_STRENGTH;
    n.z = 1.0;
    return normalize(n);
}

[shader("fragment")]
float4 ps_main(VertexOutput v) : SV_Target
{
    float3 albedo = material[getMaterial(v.materialID, Map::ALBEDO)].Sample(v.uv).rgb;

    float3 detailN = computeHeightMapNormal(v.uv);
    float3 N = normalize(v.world_normal + detailN * 0.7);

    float3 lightDir = normalize(float3(0.4, 1.0, 0.3));
    float NdotL = saturate(dot(N, lightDir));

    float3 color = albedo * (0.15 + NdotL * 0.85);

    return float4(color, 1.0);
}