#pragma once

#include <daxa/daxa.inl>
#include <hexane/constants.inl>

//REGION
struct Palette {
    daxa_u32 information;
};

struct Chunk {
    daxa_u32 heap_offset;
    daxa_u32 edge_exposed[6];
    daxa_u32 palette_count;
    Palette palettes[PALETTES_SIZE];
};

struct RegionUniformity {
    daxa_u32 lod_x2[1024];
    daxa_u32 lod_x4[256];
    daxa_u32 lod_x8[64];
    daxa_u32 lod_x16[16];
    daxa_u32 lod_x32[4];
};

struct Region {
    daxa_f32mat4x4 transform;
    daxa_u32 chunk_count;
    RegionUniformity uniformity;
    Chunk chunks[REGION_SIZE];
};

DAXA_ENABLE_BUFFER_PTR(Region)

struct Regions {
    daxa_u32 region_count;
    daxa_BufferPtr(Region) data;
};
     
DAXA_ENABLE_BUFFER_PTR(Regions)

struct VolumeDescriptor {
    daxa_u32 seed;
    daxa_u32vec3 bounds;
};

struct Volume {
    VolumeDescriptor descriptor;
    daxa_u32 region_count;
    daxa_u32 region_indices[VOLUME_MAX];
};

DAXA_ENABLE_BUFFER_PTR(Volume)