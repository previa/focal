#pragma once

#include "byte_buffer.h"

#define HEADER_CHUNK_ID						0x52494646	// RIFF
#define HEADER_FORMAT						0x57415645	// WAVE
#define HEADER_SUBCHUNK_1_ID                    		0x666d7420 	// fmt
#define HEADER_SUBCHUNK_2_ID    		                0x64617461 	// data
#define HEADER_LIST		                                0x4c495354 	// LIST
        
typedef struct {
    uint32_t chunk_size;
    uint32_t subchunk_1_size;
    uint32_t subchunk_2_size;
    uint32_t sample_rate;
    uint32_t byte_rate;

    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t audio_format;
    uint16_t num_of_channels;
} WavHeader;


static inline void wav_header_print(const WavHeader *header) {
    printf("\nWAV Header\n");
    printf("\tChunk size: %d\n", header->chunk_size);
    printf("\tSubchunk1ID: %d\n", header->subchunk_1_size);
    printf("\tAudio format: %d\n", header->audio_format);
    printf("\tChannels: %d\n", header->num_of_channels);
    printf("\tSample rate: %d\n", header->sample_rate);
    printf("\tByte rate : %d\n", header->byte_rate);
    printf("\tBlock align: %d\n", header->block_align);
    printf("\tBits per sample: %d\n", header->bits_per_sample);
    printf("\tSubchunk2Size: %d\n", header->subchunk_2_size);
    printf("\n");
}


static inline int wav_header_get(WavHeader *header, ByteBuffer *buffer) {
    printf("\nReading WAV header...\n");
    if(byte_buffer_read_int32(buffer, BE) != HEADER_CHUNK_ID) {
    	fprintf(stderr, "[WavHeader] Unable to parse header: did not find ChunkID\n");
	return 1;
    }

    header->chunk_size = byte_buffer_read_int32(buffer, LE);

    if(byte_buffer_read_int32(buffer, BE) != HEADER_FORMAT) {
	fprintf(stderr, "[WavHeader] Unable to parse header: did not find Format\n");
	return 1;
    }

    if(byte_buffer_read_int32(buffer, BE) != HEADER_SUBCHUNK_1_ID) {
	fprintf(stderr, "[WavHeader] Unable to parse header: did not find Subchunk1ID\n");
	return 1;
    }

    header->subchunk_1_size = byte_buffer_read_int32(buffer, LE);

    header->audio_format = byte_buffer_read_int16(buffer, LE);
    header->num_of_channels = byte_buffer_read_int16(buffer, LE);

    header->sample_rate = byte_buffer_read_int32(buffer, LE);
    header->byte_rate = byte_buffer_read_int32(buffer, LE);

    header->block_align = byte_buffer_read_int16(buffer, LE);
    header->bits_per_sample = byte_buffer_read_int16(buffer, LE);

    // TODO: Support other WAV specs
    if(byte_buffer_read_int32(buffer, BE) != HEADER_SUBCHUNK_2_ID) {
	fprintf(stderr, "[WavHeader] Unable to parse header: did not find Subchunk2ID\n");
	return 1;
    }

    header->subchunk_2_size = byte_buffer_read_int32(buffer, LE);

    return 0;
}
