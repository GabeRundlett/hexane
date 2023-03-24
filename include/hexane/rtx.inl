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
	daxa_i32vec3 position;
	daxa_f32 dist;
	daxa_b32vec3 mask;
	daxa_f32vec3 delta_dist;
	daxa_i32vec3 step;
	daxa_f32vec3 side_dist;
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

void ray_cast_body(inout Ray ray) {
	ray.mask = lessThanEqual(ray.side_dist.xyz, min(ray.side_dist.yzx, ray.side_dist.zxy));
    ray.side_dist += vec3(ray.mask) * ray.delta_dist;
    ray.position += ivec3(vec3(ray.mask)) * ray.step;
	ray.dist = length(vec3(ray.mask) * (ray.side_dist - ray.delta_dist)) / length(ray.descriptor.direction);
}


void ray_cast_start(RayDescriptor descriptor, out Ray ray) {    
	descriptor.direction = normalize(descriptor.direction);

	ray.descriptor = descriptor;
	ray.state_id = RAY_STATE_INITIAL;
	ray.position = daxa_i32vec3(floor(ray.descriptor.origin + 0.));
	ray.dist = 0;
	ray.mask = daxa_b32vec3(false);
	ray.delta_dist = 1.0 / abs(ray.descriptor.direction);
	ray.step = daxa_i32vec3(sign(ray.descriptor.direction));
	ray.side_dist = (daxa_f32vec3(ray.step) * (daxa_f32vec3(ray.position) - ray.descriptor.origin) + (daxa_f32vec3(ray.step) * 0.5) + 0.5) * ray.delta_dist;
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
	bool inside = all(greaterThanEqual(ray.position, ray.descriptor.minimum)) && all(lessThan(ray.position, ray.descriptor.maximum));
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
	Query q;
	
	q.chunk_position = ray.position / AXIS_CHUNK_SIZE;
	q.local_position = ray.position % AXIS_CHUNK_SIZE;

	bool voxel_found = query(q);

    bool inside = all(greaterThanEqual(ray.position, daxa_i32vec3(0))) && all(lessThan(ray.position, daxa_i32vec3(CHUNK_SIZE * REGION_SIZE)));
    debugPrintfEXT("%d", q.information);

	if (voxel_found && q.information != ray.descriptor.medium && inside) {
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