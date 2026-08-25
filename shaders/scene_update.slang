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

[shader("compute")]
void move_scene(uint3 dispatchThreadID: SV_DispatchThreadID) {
    uint chunkIndex = dispatchThreadID.x;

    int3 pos = int3(
        int8_t((cmds[chunkIndex].firstVertex >> 18) & 0x1FF),
        int8_t((cmds[chunkIndex].firstVertex >> 9) & 0x1FF),
        int8_t(cmds[chunkIndex].firstVertex & 0x1FF)
    );
    cmds[chunkIndex].firstVertex = 0ull;

    int3 move_dir = get_moveDir();

    pos -= move_dir;

    cmds[chunkIndex].firstVertex = ((chunkIndex % 6) & 0b111) << 27 
    | (pos.x & 0x1FF) << 18 
    | (pos.y & 0x1FF) << 9 
    | (pos.z & 0x1FF);
}