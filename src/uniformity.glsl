
#include <hexane/shared.inl>
#include <hexane/information.inl>

#include <daxa/daxa.inl>

layout(push_constant, scalar) uniform Push
{
    UniformityPush push;
};

layout(
    local_size_x = UNIFORMITY_INVOKE_SIZE, 
    local_size_y = UNIFORMITY_INVOKE_SIZE, 
    local_size_z = UNIFORMITY_INVOKE_SIZE
) in;

shared daxa_u32 block_id;
shared daxa_u32 is_uniform;

void main() {
	daxa_u32 region_index = deref(deref(push.unispecs).volume).region_indices[deref(push.unispecs).spec[0].region_index];

    if(deref(push.unispecs).spec_count != 1) {
        return;
    }

    daxa_u32 axis_region_in_world = AXIS_WORLD_SIZE / (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE);
    daxa_u32vec3 origin = one_d_to_three_d(deref(push.unispecs).spec[0].region_index, daxa_u32vec3(axis_region_in_world));

    daxa_u32vec3 block_origin = AXIS_REGION_SIZE * AXIS_CHUNK_SIZE * origin;

    Query q; 
    q.volume = deref(push.unispecs).volume;
    q.allocator = push.allocator;
    q.regions = push.regions;
    q.position = daxa_i32vec3(gl_GlobalInvocationID + block_origin);

    if(all(equal(gl_LocalInvocationID, daxa_u32vec3(0)))) {
        query(q);

        block_id = q.information;
        is_uniform = 1;
    }

    barrier();

    if(block_id == VOID) {
        return;
    }

    daxa_u32 l = UNIFORMITY_SIZE / UNIFORMITY_INVOKE_SIZE;

    for(daxa_u32 x = 0; x < l && bool(is_uniform); x++) {
        for(daxa_u32 y = 0; y < l && bool(is_uniform); y++) {
            for(daxa_u32 z = 0; z < l && bool(is_uniform); z++) {
                q.position = daxa_i32vec3(gl_GlobalInvocationID + block_origin) * l + daxa_i32vec3(daxa_u32vec3(x, y, z));
                query(q);

                if(q.information != block_id) {
                    atomicExchange(is_uniform, 0);
                    break;
                }
            }
        }
    }

    barrier();

    if(all(equal(gl_LocalInvocationID, daxa_u32vec3(0)))) {
        daxa_u32 a = (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / UNIFORMITY_SIZE;
        daxa_u32 i = three_d_to_one_d(gl_GlobalInvocationID / UNIFORMITY_SIZE % a, daxa_u32vec3(a));
        daxa_u32 u32_bits = 32;   
        daxa_u32 j = i / u32_bits;
        daxa_u32 k = i % u32_bits;

        if(UNIFORMITY_SIZE == 2) {
            if(bool(is_uniform)) {
                atomicOr(deref(deref(push.regions).data[region_index]).uniformity.lod_x2[j], 1 << k);
            } else {
                atomicAnd(deref(deref(push.regions).data[region_index]).uniformity.lod_x2[j], ~(1 << k));
            }
        }
        if(UNIFORMITY_SIZE == 4) {
            if(bool(is_uniform)) {
                atomicOr(deref(deref(push.regions).data[region_index]).uniformity.lod_x4[j], 1 << k);
            } else {
                atomicAnd(deref(deref(push.regions).data[region_index]).uniformity.lod_x4[j], ~(1 << k));
            }
        }
        if(UNIFORMITY_SIZE == 8) {
            if(bool(is_uniform)) {
                atomicOr(deref(deref(push.regions).data[region_index]).uniformity.lod_x8[j], 1 << k);
            } else {
                atomicAnd(deref(deref(push.regions).data[region_index]).uniformity.lod_x8[j], ~(1 << k));
            }
        }
        if(UNIFORMITY_SIZE == 16) {
            if(bool(is_uniform)) {
                atomicOr(deref(deref(push.regions).data[region_index]).uniformity.lod_x16[j], 1 << k);
            } else {
                atomicAnd(deref(deref(push.regions).data[region_index]).uniformity.lod_x16[j], ~(1 << k));
            }
        }
        if(UNIFORMITY_SIZE == 32) {
            if(bool(is_uniform)) {
                atomicOr(deref(deref(push.regions).data[region_index]).uniformity.lod_x32[j], 1 << k);
            } else {
                atomicAnd(deref(deref(push.regions).data[region_index]).uniformity.lod_x32[j], ~(1 << k));
            }
        }
    }
}
