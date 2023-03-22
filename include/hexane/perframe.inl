#pragma once

#include <daxa/daxa.inl>

struct Camera {
    daxa_f32mat4x4 projection;
    daxa_f32mat4x4 inv_projection;
    daxa_f32mat4x4 view;
    daxa_f32mat4x4 transform;
};

struct Perframe {
    Camera camera;
};