#include <hexane/shared.inl>

daxa_u32 world_gen_base(daxa_i32vec3 position) {
    daxa_f32vec3 world_position = daxa_f32vec3(position);

    daxa_u32 seed = 400;

    daxa_f32 alpha = clamp(fbm(Fbm {
        world_position,
        seed,

    }), 0.0, 1.0);

    daxa_f32 height_offset = 60;
    daxa_f32 squashing_factor = 0.05;

    daxa_f32 density = alpha;

    density -= daxa_f32(position.z - height_offset) / (1.0 / squashing_factor);

    if(density > 0.0) {
        return BLOCK_ID_STONE;
    }

    return BLOCK_ID_AIR;
}

layout(
    local_size_x = AXIS_WORKGROUP_SIZE, 
    local_size_y = AXIS_WORKGROUP_SIZE, 
    local_size_z = AXIS_WORKGROUP_SIZE
) in;

void density_derive_stone() {
    daxa_Image3Du32 workspace = deref(push.volume).workspace;

    daxa_i32vec3 position = daxa_i32vec3(gl_GlobalInvocationID)
        + deref(push.specs).origin
        - textureSize(workspace) / 2;

    imageStore(workspace, daxa_u32vec4(
        world_gen_base(position)
    ))
}