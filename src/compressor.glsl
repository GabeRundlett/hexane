
#extension GL_EXT_debug_printf : require

#include <hexane/shared.inl>

#include <daxa/daxa.inl>

layout(push_constant, scalar) uniform Push
{
    CompressorPush push;
};

#if defined(COMPRESSOR_PALETTIZE) || defined(COMPRESSOR_WRITE)
layout(
    local_size_x = AXIS_CHUNK_SIZE, 
    local_size_y = AXIS_CHUNK_SIZE, 
    local_size_z = AXIS_CHUNK_SIZE
) in;
#else
layout(
    local_size_x = 1, 
    local_size_y = 1, 
    local_size_z = 1
) in;
#endif

#define COMPRESSOR_BITS \
    daxa_u32 u32_bits = 32; \
    daxa_u32 index_bits = daxa_u32(ceil(log2(daxa_f32(deref(push.volume).regions[0].chunks[region_chunk_index].palette_count))));

#define COMPRESSOR_LOAD_INFORMATION daxa_u32 information = imageLoad(push.workspace, daxa_i32vec3(workspace_position)).r;

#ifdef COMPRESSOR_PALETTIZE

void main() {
    WORKSPACE_PRELUDE

    COMPRESSOR_LOAD_INFORMATION

    if(information == VOID) {
            return;
    }

    for(daxa_u32 palette_id = 0; palette_id < PALETTES_SIZE; palette_id++) {
        daxa_u32 old_information = atomicCompSwap(
            deref(push.volume)
                .regions[0]
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information,
            VOID,
            information
        );
        
        if(old_information == VOID) {
            atomicAdd(
                deref(push.volume)
                    .regions[0]
                    .chunks[region_chunk_index]
                    .palette_count,
                1
            );
        }

        daxa_u32 current_information = deref(push.volume)
                .regions[0]
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information;

        if(current_information == information) {
            break;
        }
    }    
}
#elif defined(COMPRESSOR_ALLOCATE)

#include <hexane/allocator.inl>

void main() {
    ALLOCATOR_PRELUDE

    COMPRESSOR_BITS

    daxa_u32 blob_size = daxa_u32(
        ceil(
            daxa_f32(CHUNK_SIZE)
            * daxa_f32(index_bits)
            / daxa_f32(u32_bits)  
        )
    );

    deref(push.volume)
        .regions[0]
        .chunks[region_chunk_index]
        .heap_offset = shader_malloc(blob_size);
}
#elif defined(COMPRESSOR_WRITE)
void main() {
    WORKSPACE_PRELUDE

    COMPRESSOR_BITS

    COMPRESSOR_LOAD_INFORMATION

    daxa_u32 palette_count = deref(push.volume)
        .regions[0]
        .chunks[region_chunk_index]
        .palette_count;
    
    if(palette_count == 0) {
        return;
    }

    daxa_u32 palette_id = 0;

    for(; palette_id < palette_count; palette_id++) {
		daxa_u32 palette_information =
            deref(push.volume)
                .regions[0]
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information;

        if(information == palette_information) {
            break;
		}
	}

    for(daxa_u32 bit = 0; bit < index_bits; bit++) {
        daxa_u32 bit_index = workspace_local_index * index_bits
            + bit
            + u32_bits * deref(push.volume).regions[0].chunks[region_chunk_index].heap_offset;
        daxa_u32 heap_offset = bit_index / u32_bits;
        daxa_u32 bit_offset = bit_index % u32_bits;
        atomicOr(deref(push.heap).data[heap_offset], ((palette_id >> bit) & 1) << bit_offset);             
    }
}
#endif