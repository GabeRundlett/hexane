#pragma once

#include <daxa/daxa.inl>
#include <hexane/volume.inl>
#include <hexane/allocator.inl>
#include <hexane/perframe.inl>

DAXA_ENABLE_BUFFER_PTR(Volume)
DAXA_ENABLE_BUFFER_PTR(Heap)
DAXA_ENABLE_BUFFER_PTR(Perframe)

//PUSH CONSTANTS
struct CompressorPush {
     daxa_Image3Du32 workspace;
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Heap) heap;
};

struct RaytracePush {
     daxa_BufferPtr(Perframe) perframe;
};