#pragma once

#include <daxa/daxa.inl>

struct Heap {
    daxa_u32 offset;
    daxa_u32 data[1000000];
};

DAXA_ENABLE_BUFFER_PTR(Heap)
