#ifdef DAXA_SHADER

#include <hexane/heap.inl>

daxa_u32 shader_malloc(daxa_u32 size) {
    daxa_u32 result_address = atomicAdd(deref(push.heap).offset, size + 2);
    //SIZE as the beginning
    deref(push.heap).data[result_address] = size + 2;
    //DEADBEEF at the end
    deref(push.heap).data[result_address + size + 1] = 3735928559;

    return result_address + 1;
}

#endif