#pragma once

#define WORKSPACE_PRELUDE \
    daxa_u32vec3 workspace_position = gl_GlobalInvocationID; \
    daxa_u32vec3 workspace_chunk_position = workspace_position / AXIS_CHUNK_SIZE; \
    daxa_u32 workspace_chunk_index = three_d_to_one_d(workspace_chunk_position, WORKSPACE_MAXIMUM); \
    daxa_u32vec3 workspace_local_position = workspace_position % AXIS_CHUNK_SIZE; \
    daxa_u32 workspace_local_index = three_d_to_one_d(workspace_local_position, CHUNK_MAXIMUM); \
    daxa_u32 spec_region_index = deref(push.specs).spec[workspace_chunk_index].region_index; \
    daxa_u32 region_index = deref(deref(push.specs).volume).region_indices[spec_region_index]; \
    daxa_u32 chunk_index = deref(push.specs).spec[workspace_chunk_index].chunk_index;

#define ALLOCATOR_PRELUDE \
    daxa_u32vec3 workspace_chunk_position = gl_GlobalInvocationID; \
    daxa_u32 workspace_chunk_index = three_d_to_one_d(workspace_chunk_position, WORKSPACE_MAXIMUM); \
    daxa_u32 spec_region_index = deref(push.specs).spec[workspace_chunk_index].region_index; \
    daxa_u32 region_index = deref(deref(push.specs).volume).region_indices[spec_region_index]; \
    daxa_u32 chunk_index = deref(push.specs).spec[workspace_chunk_index].chunk_index;

#define INDICES(v, p) \
    daxa_u32 local_index = three_d_to_one_d(p % AXIS_CHUNK_SIZE, CHUNK_MAXIMUM); \
	daxa_u32 chunk_index = three_d_to_one_d((p / AXIS_CHUNK_SIZE) % AXIS_REGION_SIZE, REGION_MAXIMUM); \
	daxa_u32 region_index = deref(v).region_indices[three_d_to_one_d((p / (AXIS_CHUNK_SIZE * AXIS_REGION_SIZE)) % AXIS_WORLD_SIZE, deref(v).descriptor.bounds)];
