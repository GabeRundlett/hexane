#include <daxa/daxa.inl>

#include <hexane/shared.inl>

#if defined(RAYTRACE_VERT) || defineD(RAYTRACE_FRAG)
layout(push_constant, scalar) uniform Push
{
    RaytraceDrawPush push;
};
#elif defined(RAYTRACE_PREPARE)
layout(push_constant, scalar) uniform Push
{
    RaytracePreparePush push;
};
#endif

#if defined(RAYTRACE_PREPARE)

layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;

void prepare() {
    deref(push.indirect).vertex_count = 36;
    deref(push.indirect).instance_count = push.volume.region_count;
}

#elif defined(RAYTRACE_VERT)

void main()
{
    Camera camera = deref(push.perframe).camera;

    daxa_f32 x = daxa_f32(1 - daxa_i32(gl_VertexIndex)) * 0.5;
    daxa_f32 y = daxa_f32(daxa_i32(gl_VertexIndex & 1) * 2 - 1) * 0.5;

    daxa_f32vec4 world_position = daxa_f32vec4(x, y, 0.0, 1.0);

    gl_Position = camera.projection * camera.view * world_position;
}

#elif defined(RAYTRACE_FRAG)

layout(location = 0) out daxa_f32vec4 color;
void main()
{
    color = daxa_f32vec4(1, 1, 1, 1);

}

#endif
