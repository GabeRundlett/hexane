#pragma once

#include <daxa/daxa.inl>
#include <hexane/volume.inl>

struct Spec {
    daxa_u32 region_index;
    daxa_u32 chunk_index;
    daxa_i32vec3 origin;
};

struct Specs {
    daxa_BufferPtr(Volume) volume;
    daxa_u32 spec_count;
    Spec spec[WORKSPACE_SIZE];
};

struct UniSpec {
    daxa_u32 region_index;
};

struct UniSpecs {
    daxa_BufferPtr(Volume) volume;
    daxa_u32 spec_count;
    UniSpec spec[WORKSPACE_SIZE];
};

struct RaytraceSpec {
    daxa_u32 region_index;
};

struct RaytraceSpecs {
    daxa_BufferPtr(Volume) volume;
    daxa_u32 spec_count;
    RaytraceSpec spec[VOLUME_MAX];
};

