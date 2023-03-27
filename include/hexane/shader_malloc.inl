daxa_u32 shader_malloc(daxa_u32 size) {
    daxa_u32 result_address = atomicAdd(deref(push.allocator).heap_offset, size + 2);
    //SIZE as the beginning
    deref(deref(push.allocator).heap[result_address]) = size + 2;
    //DEADBEEF at the end
    deref(deref(push.allocator).heap[result_address + size + 1])  = 3735928559;

    return result_address + 1;
}