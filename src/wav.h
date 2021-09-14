#pragma once

#include <stddef.h>
#include <stdint.h>

#include "byte_buffer.h"

// Header constants
#define HEADER_CHUNK_ID 0x52494646       // riff
#define HEADER_FORMAT 0x57415645         // wave
#define HEADER_SUBCHUNK_1_ID 0x666d7420  // fmt
#define HEADER_SUBCHUNK_2_ID 0x64617461  // data
#define HEADER_LIST 0x4c495354           // list
#define HEADER_LENGTH_1 44               // TODO: support multiple header lengths

// Bits per sample
#define BITS_PER_SAMPLE_16 16
#define BITS_PER_SAMPLE_8 8

// Audio format
#define AUDIO_FORMAT_PCM 1

// Number of channels
#define MONO 1
#define STEREO 2

typedef struct {
    // These values can be calulated
    uint32_t chunk_size;
    uint32_t subchunk_1_size;
    uint32_t subchunk_2_size;
    uint32_t byte_rate;
    uint16_t block_align;

    // These values can not
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t audio_format;
    uint16_t num_of_channels;
} WavHeader;

// Channel 0: data[0], Channel 1: data[1], etc
typedef struct {
    float* m_data;
} WavSample;

typedef struct {
    size_t nr_of_samples;
    uint8_t nr_of_channels;
    WavSample* m_samples;
} WavData;

static inline void wav_print_header(WavHeader* header) {
    printf("\nWAV Header\n");
    printf("\tChunk size: %d bytes\n", header->chunk_size);
    printf("\tSubchunk1Size: %d bytes\n", header->subchunk_1_size);
    printf("\tAudio format: %d\n", header->audio_format);
    printf("\tChannels: %d channels\n", header->num_of_channels);
    printf("\tSample rate: %d Hz\n", header->sample_rate);
    printf("\tByte rate : %d byte(s) per second\n", header->byte_rate);
    printf("\tBlock align: %d byte(s) per sample\n", header->block_align);
    printf("\tBits per sample: %d bits per sample for one channel\n", header->bits_per_sample);
    printf("\tSubchunk2Size: %d bytes\n", header->subchunk_2_size);
    printf("\n");
}