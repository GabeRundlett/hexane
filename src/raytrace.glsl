#extension GL_EXT_debug_printf : enable
#include <hexane/shared.inl>
#include <daxa/daxa.inl>

#if defined(RAYTRACE_VERT) || defined(RAYTRACE_FRAG)
layout(push_constant, scalar) uniform Push
{
    RaytraceDrawPush push;
};
#include <hexane/rtx.inl>
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

void main() {
    deref(push.indirect).vertex_count = 36;
    deref(push.indirect).instance_count = deref(push.volume).region_count;
}

#elif defined(RAYTRACE_VERT)

layout(location = 0) out daxa_f32vec4 local_position;
layout(location = 1) flat out daxa_i32vec4 normal;

void main()
{
    daxa_u32 indices[36] = daxa_u32[](
        1, 0, 3, 1, 3, 2,
        4, 5, 6, 4, 6, 7, 
        2, 3, 7, 2, 7, 6, 
        5, 4, 0, 5, 0, 1, 
        6, 5, 1, 6, 1, 2, 
        3, 0, 4, 3, 4, 7
    );

	daxa_f32vec3 offsets[8] = daxa_f32vec3[](
		daxa_f32vec3(0.0, 0.0, 1.0),
        daxa_f32vec3(0.0, 1.0, 1.0),
        daxa_f32vec3(1.0, 1.0, 1.0),
        daxa_f32vec3(1.0, 0.0, 1.0),
        daxa_f32vec3(0.0, 0.0, 0.0),
        daxa_f32vec3(0.0, 1.0, 0.0),
        daxa_f32vec3(1.0, 1.0, 0.0),
		daxa_f32vec3(1.0, 0.0, 0.0)
	);

	daxa_i32vec3 normals[6] = daxa_i32vec3[](
		daxa_i32vec3(0, 0, 1),
		daxa_i32vec3(0, 0, -1),
		daxa_i32vec3(1, 0, 0),
		daxa_i32vec3(-1, 0, 0),
		daxa_i32vec3(0, 1, 0),
		daxa_i32vec3(0, -1, 0)
	);

    local_position = daxa_f32vec4(offsets[indices[gl_VertexIndex]], 1.0);

    normal = daxa_i32vec4(normals[gl_VertexIndex / 6], 0);

    Camera camera = deref(push.perframe).camera;

    gl_Position = camera.projection * camera.view * local_position;
}

#elif defined(RAYTRACE_FRAG)

layout(location = 0) in daxa_f32vec4 local_position;
layout(location = 1) flat in daxa_i32vec4 normal;

layout(location = 0) out daxa_f32vec4 result;

void main()
{
    daxa_f32vec3 v = local_position.xyz;
    daxa_f32vec3 o = deref(push.perframe).camera.transform[3].xyz;


    RayDescriptor desc;
    desc.origin = v * AXIS_CHUNK_SIZE * AXIS_REGION_SIZE + 1e-2 * daxa_f32vec3(normal.xyz);
    desc.direction = normalize(v - o);
    desc.max_dist = 1000;
    desc.minimum = daxa_i32vec3(-1);
    desc.maximum = daxa_i32vec3(AXIS_CHUNK_SIZE * AXIS_REGION_SIZE) + 1;
    desc.medium = BLOCK_ID_AIR;

    Ray ray;

    ray_cast_start(desc, ray);

    while(ray_cast_drive(ray)) {}

    Hit hit;

    ray_cast_complete(ray, hit);

    daxa_f32vec3 color = daxa_f32vec3(1);

    if(hit.ray.state_id == RAY_STATE_OUT_OF_BOUNDS) {
        discard;
    }

    if(hit.block_id == BLOCK_ID_STONE) {
        color = daxa_f32vec3(mod(hit.destination.xyz, AXIS_CHUNK_SIZE) / AXIS_CHUNK_SIZE);
    }

    if(abs(hit.normal.x) == 1) {
        color *= 0.5;
    }

    if(abs(hit.normal.z) == 1) {
        color *= 0.75;
    }

    result = daxa_f32vec4(color, 1);
}
#endif
