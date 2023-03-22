#pragma once

#include <daxa/daxa.inl>

struct DrawIndirect {
    daxa_u32    vertex_count;
    daxa_u32    instance_count;
    daxa_u32    first_vertex;
    daxa_u32    first_instance;
};