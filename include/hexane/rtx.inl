#pragma once

#include <daxa/daxa.inl>
#include <hexane/information.inl>

#ifdef DAXA_SHADER

#define MAX_STEP_COUNT 1024
#define RAY_STATE_INITIAL 0
#define RAY_STATE_OUT_OF_BOUNDS 1
#define RAY_STATE_MAX_DIST_REACHED 2
#define RAY_STATE_MAX_STEP_REACHED 3
#define RAY_STATE_VOXEL_FOUND 4

struct RayDescriptor {
    daxa_BufferPtr(Volume) volume;
    daxa_BufferPtr(Regions) regions;
    daxa_BufferPtr(Allocator) allocator;
	daxa_f32vec3 origin;
	daxa_f32vec3 direction;
	daxa_i32vec3 minimum;
	daxa_i32vec3 maximum;
	daxa_f32 max_dist;
	daxa_u32 medium;
};

struct Ray {
	RayDescriptor descriptor;
	daxa_u32 state_id;
	daxa_f32vec3 position;
	daxa_f32 dist;
	daxa_b32vec3 mask;
	daxa_f32vec3 delta_dist;
	daxa_f32vec3 step;
	daxa_f32vec3 step01;
	daxa_u32 block_id;
    daxa_u32 step_count;
};

struct Hit {
	Ray ray;
	daxa_f32 dist;
	daxa_i32vec3 normal;
	daxa_i32vec3 back_step;
	daxa_f32vec3 destination;
	daxa_b32vec3 mask;
	daxa_f32vec2 uv;
	daxa_u32 block_id;
};

daxa_u32 sample_lod(inout Ray ray) {	
    bool inside = all(greaterThanEqual(daxa_i32vec3(ray.position), ray.descriptor.minimum))
		&& all(lessThan(daxa_i32vec3(ray.position), ray.descriptor.maximum));

	if(!inside) {
		return 0;
	}

    Query q;
    
    q.volume = ray.descriptor.volume;
    q.allocator = ray.descriptor.allocator;
	q.regions = ray.descriptor.regions;
	q.position = daxa_i32vec3(ray.position);

	bool voxel_found = query(q);

  	if(voxel_found && q.information != ray.descriptor.medium)
        return 0;

    INDICES(q.volume, q.position)

    daxa_u32 i;

	daxa_u32vec3 p = q.position % (AXIS_CHUNK_SIZE * AXIS_REGION_SIZE);
  
    i = three_d_to_one_d(p / 32, daxa_u32vec3((AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / 32));
    if(((deref(deref(ray.descriptor.regions).data[region_index]).uniformity.lod_x32[i / 32] >> (i % 32)) & 1) != 0)
        return 5;
    i = three_d_to_one_d(p / 16, daxa_u32vec3((AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / 16));
    if(((deref(deref(ray.descriptor.regions).data[region_index]).uniformity.lod_x16[i / 32] >> (i % 32)) & 1) != 0)
        return 4;
    i = three_d_to_one_d(p / 8, daxa_u32vec3((AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / 8));
    if(((deref(deref(ray.descriptor.regions).data[region_index]).uniformity.lod_x8[i / 32] >> (i % 32)) & 1) != 0)
        return 3;
    i = three_d_to_one_d(p / 4, daxa_u32vec3((AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / 4));
    if(((deref(deref(ray.descriptor.regions).data[region_index]).uniformity.lod_x4[i / 32] >> (i % 32)) & 1) != 0)
        return 2;
    i = three_d_to_one_d(p / 2, daxa_u32vec3((AXIS_REGION_SIZE * AXIS_CHUNK_SIZE) / 2));
    if(((deref(deref(ray.descriptor.regions).data[region_index]).uniformity.lod_x2[i / 32] >> (i % 32)) & 1) != 0)
        return 1;

    return 0;
}

void ray_cast_body(inout Ray ray) {
	daxa_u32 voxel = daxa_u32(pow(2, sample_lod(ray)));

    vec3 t_max = ray.delta_dist * (daxa_f32(voxel) * ray.step01 - mod(ray.position, daxa_f32(voxel)));

	ray.mask = lessThanEqual(t_max.xyz, min(t_max.yzx, t_max.zxy));

	float c_dist = min(min(t_max.x, t_max.y), t_max.z);
	ray.position += c_dist * ray.descriptor.direction;
	ray.dist += c_dist;
		
	ray.position += 4e-4 * ray.step * vec3(ray.mask);
}


void ray_cast_start(RayDescriptor descriptor, out Ray ray) {    
	descriptor.direction = normalize(descriptor.direction);

	ray.descriptor = descriptor;
	ray.state_id = RAY_STATE_INITIAL;
	ray.position = ray.descriptor.origin;
	ray.dist = 0;
	ray.mask = daxa_b32vec3(false);
	ray.delta_dist = 1.0 / ray.descriptor.direction;
	ray.step = daxa_f32vec3(sign(ray.descriptor.direction));
	ray.step01 = daxa_f32vec3(max(sign(ray.descriptor.direction), 0.));
	ray.block_id = 0;
    ray.step_count = 0;
}

bool ray_cast_complete(inout Ray ray, out Hit hit) {
	daxa_f32vec3 destination = ray.descriptor.origin + ray.descriptor.direction * ray.dist;
	daxa_i32vec3 back_step = ivec3(ray.position - ray.step * vec3(ray.mask));
	daxa_f32vec2 uv = mod(vec2(dot(vec3(ray.mask) * destination.yzx, vec3(1.0)), dot(vec3(ray.mask) * destination.zxy, vec3(1.0))), vec2(1.0));
	daxa_i32vec3 normal = ivec3(vec3(ray.mask) * sign(-ray.descriptor.direction));

    hit.ray = ray;
    hit.dist = ray.dist;
    hit.normal = normal;
    hit.back_step = back_step;
    hit.destination = destination;
    hit.mask = ray.mask;
    hit.uv = uv;
    hit.block_id = ray.block_id;

	return ray.state_id == RAY_STATE_VOXEL_FOUND;
}

bool ray_cast_check_over_count(inout Ray ray) {
	if(ray.step_count++ > MAX_STEP_COUNT) {
		ray.state_id = RAY_STATE_MAX_STEP_REACHED;
		return true;
	}
	return false;
}

bool ray_cast_check_out_of_bounds(inout Ray ray) {
	 bool inside = all(greaterThanEqual(daxa_i32vec3(ray.position), ray.descriptor.minimum - 10))
		&& all(lessThan(daxa_i32vec3(ray.position), ray.descriptor.maximum + 10));
	if(!inside) {
		ray.state_id = RAY_STATE_OUT_OF_BOUNDS;
		return true;
	}
	return false;
}

bool ray_cast_check_over_dist(inout Ray ray) {
	if(ray.dist > ray.descriptor.max_dist) {
		ray.state_id = RAY_STATE_MAX_DIST_REACHED;
		return true;	
	}
	return false;
}

bool ray_cast_check_failure(inout Ray ray) {
	return ray_cast_check_over_dist(ray) || ray_cast_check_out_of_bounds(ray) || ray_cast_check_over_count(ray);
}

bool ray_cast_check_success(inout Ray ray) {
 	bool inside = all(greaterThanEqual(daxa_i32vec3(ray.position), ray.descriptor.minimum))
		&& all(lessThan(daxa_i32vec3(ray.position), ray.descriptor.maximum));

	if(!inside) {
		return false;
	}

	Query q;
	
    q.volume = ray.descriptor.volume;
    q.regions = ray.descriptor.regions;
    q.allocator = ray.descriptor.allocator;
	q.position = daxa_i32vec3(ray.position);

	bool voxel_found = query(q);

	if (voxel_found && q.information != ray.descriptor.medium) {
		ray.state_id = RAY_STATE_VOXEL_FOUND;
		ray.block_id = q.information;
		return true;
	}

	return false;
}

bool ray_cast_drive(inout Ray ray) {
	if(ray.state_id != RAY_STATE_INITIAL) {
		return false;
	}
	
	if(ray_cast_check_over_count(ray)) {
		return false;
	}

	if(ray_cast_check_failure(ray)) {
		return false;
	}

	if(ray_cast_check_success(ray)) {
		return false;
	}
	
	ray_cast_body(ray);

	return true;
}

#endif