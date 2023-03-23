#pragma once

#include <daxa/daxa.inl>

struct Spec {
    daxa_u32vec3 origin;
};

struct Specs {
    daxa_u32 spec_count;
    Spec spec[WORKSPACE_SIZE];
};
