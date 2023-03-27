#pragma once

#include <daxa/daxa.inl>

struct Allocator {
    daxa_u32 heap_offset;
    daxa_BufferPtr(daxa_u32) heap;
};

DAXA_ENABLE_BUFFER_PTR(Allocator)
