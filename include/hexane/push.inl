#pragma once

#include <daxa/daxa.inl>
#include <hexane/volume.inl>
#include <hexane/perframe.inl>
#include <hexane/allocator.inl>
#include <hexane/specs.inl>
#include <hexane/indirect.inl>

DAXA_ENABLE_BUFFER_PTR(Perframe)
DAXA_ENABLE_BUFFER_PTR(Specs)
DAXA_ENABLE_BUFFER_PTR(UniSpecs)
DAXA_ENABLE_BUFFER_PTR(RaytraceSpecs)
DAXA_ENABLE_BUFFER_PTR(DrawIndirect)

//PUSH CONSTANTS
struct QueuePush {
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Regions) regions;
     daxa_BufferPtr(Specs) specs;
     daxa_BufferPtr(UniSpecs) unispecs;
};

struct BrushPush {
     daxa_RWImage3Du32 workspace;
     daxa_BufferPtr(Specs) specs;
};

struct CompressorPush {
     daxa_RWImage3Du32 workspace;
     daxa_BufferPtr(Specs) specs;
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Regions) regions;
     daxa_BufferPtr(Allocator) allocator;
};

struct RaytraceDrawPush {
     daxa_BufferPtr(RaytraceSpecs) raytrace_specs;
     daxa_BufferPtr(Regions) regions;
     daxa_BufferPtr(Perframe) perframe;
     daxa_BufferPtr(Allocator) allocator;
};

struct RaytracePreparePush {
     daxa_BufferPtr(Volume) volume;
     daxa_BufferPtr(Regions) regions;
     daxa_BufferPtr(Perframe) perframe;
     daxa_BufferPtr(DrawIndirect) indirect;
     daxa_BufferPtr(RaytraceSpecs) raytrace_specs;
};

struct UniformityPush {
     daxa_BufferPtr(UniSpecs) unispecs;
     daxa_BufferPtr(Regions) regions;
     daxa_BufferPtr(Allocator) allocator;
};