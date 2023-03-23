#pragma once

#include <daxa/daxa.inl>
#include <hexane/volume.inl>
#include <hexane/heap.inl>
#include <hexane/perframe.inl>
#include <hexane/specs.inl>
#include <hexane/indirect.inl>

DAXA_ENABLE_BUFFER_PTR(Volume)
DAXA_ENABLE_BUFFER_PTR(Perframe)
DAXA_ENABLE_BUFFER_PTR(Specs)
DAXA_ENABLE_BUFFER_PTR(DrawIndirect)

//PUSH CONSTANTS
struct QueuePush {
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Specs) specs;
};

struct BrushPush {
     daxa_RWImage3Du32 workspace;
     daxa_BufferPtr(Specs) specs;
};

struct CompressorPush {
     daxa_RWImage3Du32 workspace;
     daxa_BufferPtr(Specs) specs;
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Heap) heap;
};

struct RaytraceDrawPush {
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Perframe) perframe;
};

struct RaytracePreparePush {
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(DrawIndirect) indirect;
};