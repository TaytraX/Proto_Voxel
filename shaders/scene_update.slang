struct IndirectDrawCommand {
    uint32_t    vertexCount;
    uint32_t    instanceCount;
    uint32_t    firstVertex;
    uint32_t    firstInstance;
}

struct CameraUniform {
    float4x4 view_proj;
    float3 view_position;
    float _padding;
};

[[vk::binding(1, 0)]]
RWStructuredBuffer<IndirectDrawCommand> cmds;
[[vk::binding(0, 0)]]
ConstantBuffer<CameraUniform> camera;

int3 get_moveDir() {
    return int3(
        (int)floor(camera.view_position.x / 64),
        (int)floor(camera.view_position.y / 64),
        (int)floor(camera.view_position.z / 64)
    );
}

static bool outsideWindow(int3 pos, int3 center) {
    int dx = center.x - pos.x;
    int dz = center.z - pos.z;
    constexpr int R1sq = (16 + 1) * (16 + 1);
    return (dx * dx + dz * dz) >= R1sq;
}

[shader("compute")]
void move_scene(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint chunkIndex = dispatchThreadID.x;

    int3 pos = int3(
        int8_t((cmds[chunkIndex].firstVertex >> 16) & 0xFF),
        int8_t((cmds[chunkIndex].firstVertex >> 8) & 0xFF),
        int8_t(cmds[chunkIndex].firstVertex & 0xFF)
    );
    cmds[chunkIndex].firstVertex = 0ull;

    int3 move_dir = get_moveDir();

    if (outsideWindow(pos, move_dir)) pos = -pos;
    else pos -= move_dir;

    cmds[chunkIndex].firstVertex = ((chunkIndex % 6) & 0b111) << 24 
    | (pos.x & 0xFF) << 16 
    | (pos.y & 0xFF) << 8
    | (pos.z & 0xFF);
}