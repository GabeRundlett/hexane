#include <hexane/shared.inl>
#include <daxa/daxa.inl>

#if defined(RAYTRACE_VERT) || defined(RAYTRACE_FRAG)
layout(push_constant, scalar) uniform Push
{
    RaytraceDrawPush push;
};
#include <hexane/rtx.inl>
#elif defined(RAYTRACE_PREPARE_FRONT) || defined(RAYTRACE_PREPARE_BACK)
layout(push_constant, scalar) uniform Push
{
    RaytracePreparePush push;
};
#endif

#if defined(RAYTRACE_PREPARE_FRONT)

layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;

void main() {
    deref(push.raytrace_specs).volume = push.volume;
    deref(push.indirect).vertex_count = 36;
    deref(push.raytrace_specs).spec_count = 0;

    for(daxa_u32 i = 0; i < 8*8; i++) {
        deref(push.raytrace_specs).spec[deref(push.raytrace_specs).spec_count++].region_index = i;
    }

    deref(push.indirect).instance_count = min(deref(push.raytrace_specs).spec_count, deref(push.volume).region_count);
}

#elif defined(RAYTRACE_PREPARE_BACK)

layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;

void main() {
    daxa_u32 axis_region_in_world = AXIS_WORLD_SIZE / (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE);

    Camera camera = deref(push.perframe).camera;

#define NEAR_PLANE_CHECK_COUNT 4
    daxa_f32vec2 near_plane_check[NEAR_PLANE_CHECK_COUNT] = daxa_f32vec2[](
        daxa_f32vec2(-1.0, -1.0),
        daxa_f32vec2(1.0, -1.0),
        daxa_f32vec2(-1.0, 1.0),
        daxa_f32vec2(1.0, 1.0)
    );

    deref(push.raytrace_specs).spec_count = 0;
    deref(push.raytrace_specs).volume = push.volume;

    for(daxa_u32 i = 0; i < NEAR_PLANE_CHECK_COUNT; i++) {
        //for some reason this works if z is 0.0 and not -1.0 (near plane)
        daxa_f32vec4 near_plane = daxa_f32vec4(near_plane_check[i], 0.0, 1.0);
        daxa_f32vec4 near_plane_view_position = camera.inv_projection * near_plane;
        near_plane_view_position /= near_plane_view_position.w;
        daxa_f32vec3 near_plane_world_position = (camera.transform * near_plane_view_position).xyz;
        
        bool inside = all(greaterThanEqual(near_plane_world_position, daxa_f32vec3(0)))
            && all(lessThan(near_plane_world_position, daxa_f32vec3(axis_region_in_world)));

        if(inside) {
            daxa_u32 region_index = three_d_to_one_d(daxa_u32vec3(near_plane_world_position), daxa_u32vec3(axis_region_in_world));

            bool cont = false;
            
            for(daxa_u32 i = 0; i < deref(push.raytrace_specs).spec_count; i++) {
                if(deref(push.raytrace_specs).spec[i].region_index == region_index) {
                    cont = true;
                    break;
                }
            }

            if(cont) {
                continue;
            }

            deref(push.raytrace_specs).spec[deref(push.raytrace_specs).spec_count++].region_index = region_index;
        }
    }

    deref(push.indirect).vertex_count = 36;
    deref(push.indirect).instance_count = deref(push.raytrace_specs).spec_count;
}

#elif defined(RAYTRACE_VERT)


layout(location = 0) out daxa_f32vec4 local_position;
layout(location = 2) out daxa_f32vec4 world_position;
layout(location = 3) flat out daxa_u32vec4 origin;
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

    daxa_u32 region_index = deref(push.raytrace_specs).spec[gl_InstanceIndex].region_index;
    daxa_u32 axis_region_in_world = AXIS_WORLD_SIZE / (AXIS_REGION_SIZE * AXIS_CHUNK_SIZE);
    origin = daxa_u32vec4(one_d_to_three_d(region_index, daxa_u32vec3(axis_region_in_world)), 1);
    local_position = daxa_f32vec4(offsets[indices[gl_VertexIndex]], 1.0);
    world_position = daxa_f32vec4(local_position.xyz + daxa_f32vec3(origin.xyz), 1.0);

    normal = daxa_i32vec4(normals[gl_VertexIndex / 6], 0);

    Camera camera = deref(push.perframe).camera;

    gl_Position = camera.projection * camera.view * world_position;
}

#elif defined(RAYTRACE_FRAG)

layout(location = 0) in daxa_f32vec4 local_position;
layout(location = 2) in daxa_f32vec4 world_position;
layout(location = 3) flat in daxa_u32vec4 origin;
layout(location = 1) flat in daxa_i32vec4 normal;

layout(location = 0) out daxa_f32vec4 result;

void main()
{
    
    daxa_f32vec3 v = world_position.xyz;
    daxa_f32vec3 o = deref(push.perframe).camera.transform[3].xyz;


    RayDescriptor desc;
    desc.volume = deref(push.raytrace_specs).volume;
    desc.regions = push.regions;
    desc.allocator = push.allocator;
    if(gl_FrontFacing) {
        desc.origin = world_position.xyz * AXIS_CHUNK_SIZE * AXIS_REGION_SIZE + 1e-3 * daxa_f32vec3(normal.xyz);
    } else {
        desc.origin = o * AXIS_CHUNK_SIZE * AXIS_REGION_SIZE;
    }
    desc.direction = normalize(v - o);
    desc.max_dist = 1000;
    desc.minimum = daxa_i32vec3(origin.xyz) * AXIS_CHUNK_SIZE * AXIS_REGION_SIZE;
    desc.maximum = daxa_i32vec3(origin.xyz + 1) * AXIS_CHUNK_SIZE * AXIS_REGION_SIZE;
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

    daxa_f32vec3 neighbor = hit.destination + 0.5 * hit.normal;

    Query q;
    q.position = daxa_i32vec3(neighbor);
    q.volume = deref(push.raytrace_specs).volume;
    q.regions = push.regions;
    q.allocator = push.allocator;

    bool outside = any(lessThan(neighbor, daxa_f32vec3(0)))
		|| any(greaterThanEqual(neighbor, daxa_f32vec3(deref(q.volume).descriptor.bounds *  AXIS_REGION_SIZE * AXIS_CHUNK_SIZE)));

    if(query(q) && q.information != ray.descriptor.medium && !outside) {
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
    
    daxa_f32vec4 clip_space_pos = deref(push.perframe).camera.projection 
        * deref(push.perframe).camera.view 
        * daxa_f32vec4(hit.destination / (AXIS_CHUNK_SIZE * AXIS_REGION_SIZE), 1);
    daxa_f32 ndc_depth = clip_space_pos.z / clip_space_pos.w;
    daxa_f32 depth = (((100-0.1) * ndc_depth) + 0.1 + 100) / 2.0;
    gl_FragDepth = depth;
}
#endif
