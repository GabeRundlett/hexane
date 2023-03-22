#include <hexane/shader.inl>

layout(push_constant, scalar) uniform Push
{
    CompressorPush push;
};

layout(
    local_size_x = AXIS_WORKGROUP_SIZE, 
    local_size_y = AXIS_WORKGROUP_SIZE, 
    local_size_z = AXIS_WORKGROUP_SIZE
) in;

#define COMPRESSOR_PRELUDE \
    daxa_u32vec3 workspace_position = gl_GlobalInvocationID; \
    daxa_u32vec3 workspace_chunk_position = workspace_position / CHUNK_SIZE; \
    daxa_u32 workspace_chunk_index = three_d_to_one_d(workspace_chunk_position, WORKSPACE_MAXIMUM); \
    daxa_u32vec3 workspace_local_position = workspace_position % CHUNK_SIZE; \
    daxa_u32 workspace_local_index = three_d_to_one_d(workspace_local_position, CHUNK_MAXIMUM); \
    daxa_u32vec3 region_chunk_position = push_constant.workspace.chunk_positions[workspace_chunk_index]; \
    daxa_u32 region_chunk_index = three_d_to_one_d(region_chunk_position, REGION_MAXIMUM);

#define COMPRESSOR_BITS \
    u32 u32_bits = 32; \
    u32 index_bits = ceil_log2(deref(push_constant.region).chunks[region_chunk_index].palette_count);

#define COMPRESSOR_LOAD_INFORMATION u32 information = imageLoad(push_constant.workspace.image, workspace_position);

void palettize() {
    COMPRESSOR_PRELUDE

    COMPRESSOR_LOAD_INFORMATION

    for(u32 palette_id = 0; palette_id < MAX_PALETTES; palette_id++) {
        u32 old_information = atomicCompareExchange(
            deref(push_constant.region)
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information,
            VOID,
            information
        );

        if old_information == VOID {
            atomicAdd(
                deref(push_constant.region)
                    .chunks[region_chunk_index]
                    .palette_count,
                1
            );
        }

        u32 current_information = atomicLoad(
            deref(push_constant.region)
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information
        );

        if current_information == information {
            break;
        }
    }    
}

void allocate() {
    COMPRESSOR_PRELUDE

    COMPRESSOR_BITS

    u32 blob_size = u32(
        ceil(
            f32(CHUNK_SIZE)
            * f32(index_bits)
            / f32(u32_bits)  
        )
    );

    deref(push_constant.region)
        .chunks[region_chunk_index]
        .heap_offset = shader_malloc(blob_size);
}

void write() {
    COMPRESSOR_PRELUDE

    COMPRESSOR_BITS

    COMPRESSOR_LOAD_INFORMATION

    u32 palette_count = deref(push_constant.region)
        .chunks[region_chunk_index]
        .palette_count;
    
    if(palette_count == 0) {
        return;
    }

    u32 palette_id = 0;

    for(; palette_id < palette_count; palette_id++) {
		u32 palette_information =
            deref(push_constant.region)
                .chunks[region_chunk_index]
                .palettes[palette_id]
                .information;

        if(information == palette_information) {
            break;
		}
	}

    for(u32 bit = 0; bit < index_bits; bit++) {
        u32 bit_index = workspace_local_index * index_bits
            + bit
            + u32_bits * deref(push_constant.region).chunks[region_chunk_index].heap_offset;
        u32 heap_offset = bit_index / u32_bits;
        u32 bit_offset = bit_index % u32_bits;
        u32 bit = (palette_id >> bit) & 1;
        atomicOr(deref(push_constant.heap).data[heap_offset], bit << bit_offset);             
    }
}