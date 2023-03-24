#pragma once

#include <daxa/daxa.inl>

#define CHUNKS_PER_FRAME 10

#define AXIS_CHUNK_SIZE 8
#define CHUNK_SIZE 512
#define CHUNK_MAXIMUM daxa_u32vec3(AXIS_CHUNK_SIZE)

#define AXIS_REGION_SIZE 8
#define REGION_SIZE 512
#define REGION_MAXIMUM daxa_u32vec3(AXIS_REGION_SIZE)

#define PALETTES_SIZE 64

#define AXIS_WORKSPACE_SIZE 4
#define WORKSPACE_SIZE 64
#define WORKSPACE_MAXIMUM daxa_u32vec3(AXIS_WORKSPACE_SIZE)

#undef VOID
#define VOID 0

#define VOLUME_MAX 10000

#define AXIS_WORLD_SIZE 512