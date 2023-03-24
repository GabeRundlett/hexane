#pragma once

#include <daxa/daxa.inl>
#include <hexane/constants.inl>

//REGION
struct Palette {
    daxa_u32 information;
};

struct Chunk {
    daxa_u32 heap_offset;
    daxa_u32 palette_count;
    Palette palettes[PALETTES_SIZE];
};

struct Region {
    daxa_f32mat4x4 transform;
    daxa_u32 chunk_count;
    Chunk chunks[REGION_SIZE];
};

struct VolumeDescriptor {
    daxa_u32 seed;
};

struct Volume {
    VolumeDescriptor descriptor;
    daxa_u32 region_count;
    Region regions[VOLUME_MAX];
};