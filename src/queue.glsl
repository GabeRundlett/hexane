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

    deref(push.volume).region_count = 1;

    daxa_u32 region_index = deref(push.volume).region_count - 1;

    for(daxa_u32 i = 0; i < WORKSPACE_SIZE; i++) {
        daxa_u32 chunk_index;

        if(deref(push.volume).regions[region_index].chunk_count >= REGION_SIZE - 1) {
            //TODO do more regions
            break;
        } else {
            chunk_index = deref(push.volume).regions[region_index].chunk_count++;
        }

        daxa_u32vec3 origin = one_d_to_three_d(chunk_index, REGION_MAXIMUM);

        deref(push.specs).spec[i] = Spec(
            origin
        );
        deref(push.specs).spec_count++;
    }
}