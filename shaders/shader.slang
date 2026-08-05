struct CameraUniform {
    float4x4 view_proj;
    float3 view_position;
    float _padding;
};

struct VSOutput {
    float4 clip_position : SV_Position;
    float3 world_position;
    float3 normal;
    float2 uv;

    nointerpolation uint materialID;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraUniform> camera;
[[vk::binding(0, 1)]]
Sampler2D material[];

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
VSOutput vs_main(uint2 data, uint vertexID: SV_VertexID, uint face: SV_StartVertexLocation)
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

    VSOutput output;
    output.world_position = origin;
    output.clip_position = mul(camera.view_proj, float4(origin, 1.0));
    output.normal = normalLookup[faceIdx];
    output.uv = float2(wMod * w, hMod * h);
    output.materialID = data.y - 1;

    return output;
}

[shader("fragment")]
float4 ps_main(VSOutput v) : SV_Target
{
    // NonUniformResourceIndex est OBLIGATOIRE ici : materialID peut varier
    // d'un pixel à l'autre dans le même groupe d'exécution GPU (frontière
    // entre deux matériaux différents). Sans cette annotation, le
    // matériel peut sampler la mauvaise texture pour certains pixels du
    // groupe (artefacts "sel et poivre" à la frontière entre matériaux).
    float3 albedo = material[NonUniformResourceIndex(v.materialID)].Sample(v.uv).rgb;

    return float4(albedo, 1.0);
}