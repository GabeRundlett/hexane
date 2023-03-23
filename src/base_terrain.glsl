
#include <hexane/shared.inl>

#include <daxa/daxa.inl>

layout(push_constant, scalar) uniform Push
{
    BrushPush push;
};

daxa_u32 world_gen_base(daxa_i32vec3 position) {
    daxa_f32vec3 world_position = daxa_f32vec3(position);

    daxa_u32 seed = 400;

    daxa_u32 octaves = 10;
	daxa_f32 lacunarity = 2.0;
	daxa_f32 gain = 0.5;
	daxa_f32 amplitude = 1.0;
	daxa_f32 frequency = 0.05;

    daxa_f32 alpha = clamp(fbm(Fbm(
        position,
        seed,
        octaves,
        lacunarity,
        gain,
        amplitude,
        frequency
    )), 0.0, 1.0);

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
    local_size_x = AXIS_CHUNK_SIZE, 
    local_size_y = AXIS_CHUNK_SIZE, 
    local_size_z = AXIS_CHUNK_SIZE
) in;

void main() {
    WORKSPACE_PRELUDE
    
    daxa_i32vec3 position = daxa_i32vec3(workspace_position)
        + daxa_i32vec3(deref(push.specs).spec[workspace_chunk_index].origin);

    imageStore(push.workspace, daxa_i32vec3(workspace_position), daxa_u32vec4(
        world_gen_base(position)
    ));
}