#include <daxa/daxa.inl>
#include <hexane/shared.inl>

layout(push_constant, scalar) uniform Push
{
    CompressorPush push;
};

layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;

void queue() {
    daxa_u32 regions_in_world_axis = AXIS_WORLD_SIZE / (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE);
}