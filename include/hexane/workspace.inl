#pragma once

#define WORKSPACE_PRELUDE \
    daxa_u32vec3 workspace_position = gl_GlobalInvocationID; \
    daxa_u32vec3 workspace_chunk_position = workspace_position / CHUNK_SIZE; \
    daxa_u32 workspace_chunk_index = three_d_to_one_d(workspace_chunk_position, WORKSPACE_MAXIMUM); \
    daxa_u32vec3 workspace_local_position = workspace_position % CHUNK_SIZE; \
    daxa_u32 workspace_local_index = three_d_to_one_d(workspace_local_position, CHUNK_MAXIMUM); \
    daxa_u32vec3 region_chunk_position = deref(push.specs).spec[workspace_chunk_index].origin; \
    daxa_u32 region_chunk_index = three_d_to_one_d(region_chunk_position, REGION_MAXIMUM);

#define ALLOCATOR_PRELUDE \
    daxa_u32vec3 workspace_chunk_position = gl_GlobalInvocationID; \
    daxa_u32 workspace_chunk_index = three_d_to_one_d(workspace_chunk_position, WORKSPACE_MAXIMUM); \
    daxa_u32vec3 region_chunk_position = deref(push.specs).spec[workspace_chunk_index].origin; \
    daxa_u32 region_chunk_index = three_d_to_one_d(region_chunk_position, REGION_MAXIMUM);