#pragma once

#include <daxa/daxa.inl>
#include <hexane/constants.inl>
#include <hexane/util.inl>
#include <hexane/workspace.inl>
#include <hexane/volume.inl>

#ifdef DAXA_SHADER
struct Query {
	daxa_BufferPtr(Volume) volume;
	daxa_BufferPtr(Regions) regions;
	daxa_BufferPtr(Allocator) allocator;
	daxa_u32vec3 position;
	//Output
	daxa_u32 information;
};

bool query(inout Query query) {
	query.information = 0;

	INDICES(query.volume, query.position)

	daxa_u32 u32_bits = 32;
    daxa_u32 index_bits = daxa_u32(ceil(log2(daxa_f32(deref(deref(query.regions).data[region_index]).chunks[chunk_index].palette_count))));

	daxa_u32 palette_id = 0;

	for(daxa_u32 bit = 0; bit < index_bits; bit++) {
		daxa_u32 bit_index = local_index * index_bits 
			+ bit
			+ u32_bits * deref(deref(query.regions).data[region_index]).chunks[chunk_index].heap_offset;
		daxa_u32 heap_offset = bit_index / u32_bits;
		daxa_u32 bit_offset = bit_index % u32_bits;
		palette_id |= ((deref(deref(query.allocator).heap[heap_offset]) >> bit_offset) & 1u) << bit;
	}

	query.information = deref(deref(query.regions).data[region_index]).chunks[chunk_index].palettes[palette_id].information;

	return query.information != 0;
}
#endif