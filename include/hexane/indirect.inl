#pragma once

#include <daxa/daxa.inl>

struct DrawIndirect {
    daxa_u32    vertex_count;
    daxa_u32    instance_count;
    daxa_u32    first_vertex;
    daxa_u32    first_instance;
};

struct DispatchIndirect {
    daxa_u32    x;
    daxa_u32    y;
    daxa_u32    z;
};

struct CompressorIndirect {
    DispatchIndirect palettize;
    DispatchIndirect allocate;
    DispatchIndirect write;
};