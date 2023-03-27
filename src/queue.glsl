#include <hexane/shared.inl>

#include <daxa/daxa.inl>

layout(push_constant, scalar) uniform Push
{
    QueuePush push;
};

layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;

void main() {
    deref(push.specs).spec_count = 0;
    deref(push.unispecs).spec_count = 0;

    deref(push.unispecs).volume = push.volume;
    deref(push.specs).volume = push.volume;

    if(deref(push.volume).region_count == 0) {
        //making 0 a void region prevents regions that are unitialized from rendering the first region generated
        atomicAdd(deref(push.regions).region_count, 1);

        deref(push.volume).region_indices[0] = atomicAdd(deref(push.regions).region_count, 1);
        deref(push.volume).region_count = 1;
    }

    daxa_u32 region_index = deref(push.volume).region_count - 1;

    for(daxa_u32 i = 0; i < WORKSPACE_SIZE; i++) {
        daxa_u32 chunk_index;

        daxa_u32 axis_region_in_world = AXIS_WORLD_SIZE / (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE);
        daxa_u32 world_size = axis_region_in_world * axis_region_in_world * axis_region_in_world;
        deref(push.volume).descriptor.bounds = daxa_u32vec3(axis_region_in_world);

        if(deref(push.volume).region_count >= world_size) {
            break;
        }

        if(deref(deref(push.regions).data[deref(push.volume).region_indices[region_index]]).chunk_count >= REGION_SIZE) {
            deref(push.unispecs).spec[deref(push.unispecs).spec_count].region_index = region_index;
            deref(push.unispecs).spec_count++;
            deref(push.volume).region_count += 1;
            region_index = deref(push.volume).region_count - 1;
            deref(push.volume).region_indices[region_index] = atomicAdd(deref(push.regions).region_count, 1);
            chunk_index = 0;
        } else {
            chunk_index = deref(deref(push.regions).data[deref(push.volume).region_indices[region_index]]).chunk_count;
            deref(deref(push.regions).data[deref(push.volume).region_indices[region_index]]).chunk_count++;
        }

        daxa_i32vec3 origin = daxa_i32vec3(one_d_to_three_d(region_index, daxa_u32vec3(axis_region_in_world)));

        deref(push.specs).spec[i] = Spec(
            region_index,
            chunk_index,
            origin
        );
        deref(push.specs).spec_count++;
    }
}