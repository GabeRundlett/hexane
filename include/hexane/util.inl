#pragma once

#include <daxa/daxa.inl>

#if !defined(DAXA_SHADER)
#include <bit>
#endif

#ifdef DAXA_SHADER
float map(daxa_f32 value, daxa_f32 in_min, daxa_f32 in_max, daxa_f32 out_min, daxa_f32 out_max) {
  return out_min + (out_max - out_min) * (value - in_min) / (in_max - in_min);
}

vec2 map(daxa_f32vec2 value, daxa_f32vec2 in_min, daxa_f32vec2 in_max, daxa_f32vec2 out_min, daxa_f32vec2 out_max) {
  return out_min + (out_max - out_min) * (value - in_min) / (in_max - in_min);
}

vec3 map(daxa_f32vec3 value, daxa_f32vec3 in_min, daxa_f32vec3 in_max, daxa_f32vec3 out_min, daxa_f32vec3 out_max) {
  return out_min + (out_max - out_min) * (value - in_min) / (in_max - in_min);
}

vec4 map(daxa_f32vec4 value, daxa_f32vec4 in_min, daxa_f32vec4 in_max, daxa_f32vec4 out_min, daxa_f32vec4 out_max) {
  return out_min + (out_max - out_min) * (value - in_min) / (in_max - in_min);
}
#endif

daxa_u32 three_d_to_one_d(daxa_u32vec3 p, daxa_u32vec3 max) {
    return (p.z * max.x * max.y) + (p.y * max.x) + p.x;
}

daxa_u32vec3 one_d_to_three_d(daxa_u32 idx, daxa_u32vec3 max) {
    daxa_u32 z = idx / (max.x * max.y);
    idx -= (z * max.x * max.y);
    daxa_u32 y = idx / max.x;
    daxa_u32 x = idx % max.x;
    daxa_u32vec3 p;
    p.x = x;
    p.y = y;
    p.z = z;
    return p; 
}

daxa_u32 hash(daxa_u32 a) {
    daxa_u32 x = a;
	x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

daxa_u32 hash(daxa_u32vec2 v) {
    return hash(v.x ^ hash(v.y));
}

daxa_u32 hash(daxa_u32vec3 v) {
    return hash(v.x ^ hash(v.y) ^ hash(v.z));
}

daxa_u32 hash(daxa_u32vec4 v) {
    return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w));
}

daxa_u32 bitcast(daxa_f32 x) {
#ifdef DAXA_SHADER
	return floatBitsToUint(x);
#else
    return std::bit_cast<daxa_u32>(x);
#endif
}

daxa_f32 bitcast(daxa_u32 x) {
#ifdef DAXA_SHADER
	return uintBitsToFloat(x);
#else
    return std::bit_cast<daxa_f32>(x);
#endif
}

daxa_f32 float_construct(daxa_u32 m) {
	daxa_u32 x = m;

	daxa_u32 ieee_mantissa = 0x007FFFFFu;
	daxa_u32 ieee_one = 0x3F800000u;

	x &= ieee_mantissa;
	x |= ieee_one;

    daxa_f32 f = bitcast(x);

	return f - 1.0;
}

daxa_f32 random(daxa_f32 x) {
    return float_construct(hash(bitcast(x)));
}

daxa_f32 random(daxa_f32vec2 v) {
    daxa_u32vec2 a;
    a.x = bitcast(v.x);
    a.y = bitcast(v.y);
    return float_construct(hash(a));
}

daxa_f32 random(daxa_f32vec3 v) {
    daxa_u32vec3 a;
    a.x = bitcast(v.x);
    a.y = bitcast(v.y); 
    a.z = bitcast(v.z); 
    return float_construct(hash(a));
}

daxa_f32 random(daxa_f32vec4 v) {
    daxa_u32vec4 a;
    a.x = bitcast(v.x);
    a.y = bitcast(v.y);
    a.z = bitcast(v.z); 
    a.w = bitcast(v.w);
    return float_construct(hash(a));
}

//TODO implement perlin noise for non-shader code if need be
#ifdef DAXA_SHADER

daxa_f32vec3 random_gradient(daxa_i32vec3 position, daxa_u32 seed) {
    daxa_f32 alpha = random(daxa_f32vec4(daxa_f32vec3(position), daxa_f32(seed)));
	daxa_f32 beta = random(daxa_f32vec4(daxa_f32vec3(position), daxa_f32(seed) + 1.0));

    //normalize not implemented for non shaders
	return normalize(daxa_f32vec3(
		cos(alpha) * cos(beta),
		sin(beta),
		sin(alpha) * cos(beta)
	));
}

daxa_f32 dot_grid_gradient(daxa_i32vec3 i, daxa_f32vec3 p, daxa_u32 seed) {
    return dot(random_gradient(i, seed), p - daxa_f32vec3(i));
}

daxa_f32 perlin(daxa_f32vec3 position, daxa_u32 seed) {
    daxa_i32vec3 m0 = daxa_i32vec3(floor(position));

	daxa_i32vec3 m1 = m0 + 1;

	daxa_f32vec3 s = position - daxa_f32vec3(m0);

	daxa_f32 n0;
	daxa_f32 n1;
	daxa_f32 ix0;
	daxa_f32 ix1;
	daxa_f32 jx0;
	daxa_f32 jx1;
	daxa_f32 k;

	n0 = dot_grid_gradient(daxa_i32vec3(m0.x, m0.y, m0.z), position, seed);
	n1 = dot_grid_gradient(daxa_i32vec3(m1.x, m0.y, m0.z), position, seed);
	ix0 = mix(n0, n1, s.x);

	n0 = dot_grid_gradient(daxa_i32vec3(m0.x, m1.y, m0.z), position, seed);
	n1 = dot_grid_gradient(daxa_i32vec3(m1.x, m1.y, m0.z), position, seed);
	ix1 = mix(n0, n1, s.x);

	jx0 = mix(ix0, ix1, s.y); 
	
	n0 = dot_grid_gradient(daxa_i32vec3(m0.x, m0.y, m1.z), position, seed);
	n1 = dot_grid_gradient(daxa_i32vec3(m1.x, m0.y, m1.z), position, seed);
	ix0 = mix(n0, n1, s.x);

	n0 = dot_grid_gradient(daxa_i32vec3(m0.x, m1.y, m1.z), position, seed);
	n1 = dot_grid_gradient(daxa_i32vec3(m1.x, m1.y, m1.z), position, seed);
	ix1 = mix(n0, n1, s.x);

	jx1 = mix(ix0, ix1, s.y); 

	k = mix(jx0, jx1, s.z);

	return k;
}

struct Fbm {
    daxa_f32vec3 position;
    daxa_u32 seed;
    daxa_u32 octaves;
    daxa_f32 lacunarity;
    daxa_f32 gain;
    daxa_f32 amplitude;
    daxa_f32 frequency;
};

daxa_f32 fbm(Fbm f) {
    daxa_f32 height = 0.0;

    for(daxa_u32 i = 0; i < f.octaves; i++) {
        height += f.amplitude * perlin(f.frequency * f.position, f.seed);
        f.frequency *= f.lacunarity;
        f.amplitude *= f.gain;    
    }

    return height;
}

#endif