#pragma once

#include <daxa/daxa.inl>

struct Heap {
    daxa_u32 offset;
    daxa_u32 data[1000000];
};

DAXA_ENABLE_BUFFER_PTR(Heap)

#ifdef DAXA_SHADER
daxa_u32 shader_malloc(daxa_BufferPtr(Heap) heap, daxa_u32 size) {
    daxa_u32 result_address = atomicAdd(deref(heap).offset, size + 2);
    //SIZE as the beginning
    deref(heap).data[result_address] = size + 2;
    //DEADBEEF at the end
    deref(heap).data[result_address + size] = 3735928559;
    return result_address + 1;
}
#endif