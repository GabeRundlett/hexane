#pragma once

#include <daxa/daxa.inl>
#include <hexane/constants.inl>
#include <hexane/util.inl>

#ifdef DAXA_SHADER
struct Query {
	//Input
	daxa_u32 palette_count;
	daxa_u32vec3 chunk_position;
	daxa_u32vec3 local_position;
	//Output
	daxa_u32 information;
};

bool query(inout Query query) {
	query.information = 0;

	daxa_u32 local_position_index = three_d_to_one_d(query.local_position, CHUNK_MAXIMUM);

	daxa_u32 chunk_position_index = three_d_to_one_d(query.chunk_position, REGION_MAXIMUM);

	daxa_u32 u32_bits = 32;
    daxa_u32 index_bits = daxa_u32(ceil(log2(daxa_f32(deref(push.volume).regions[0].chunks[chunk_position_index].palette_count))));

	daxa_u32 palette_id = 0;

	for(daxa_u32 bit = 0; bit < index_bits; bit++) {
		daxa_u32 bit_index = local_position_index * index_bits 
			+ bit
			+ u32_bits * deref(push.volume).regions[0].chunks[chunk_position_index].heap_offset;
		daxa_u32 heap_offset = bit_index / u32_bits;
		daxa_u32 bit_offset = bit_index % u32_bits;
		palette_id |= ((deref(push.heap).data[heap_offset] >> bit_offset) & 1u) << bit;
	}

	query.information = deref(push.volume).regions[0].chunks[chunk_position_index].palettes[palette_id].information;

	return query.information != 0;
}
#endif