#pragma once

#include <stddef.h>
#include <stdint.h>

#define HEADER_CHUNK_ID 0x52494646       // riff
#define HEADER_FORMAT 0x57415645         // wave
#define HEADER_SUBCHUNK_1_ID 0x666d7420  // fmt
#define HEADER_SUBCHUNK_2_ID 0x64617461  // data
#define HEADER_LIST 0x4c495354           // list

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

// Channel 0: data[0], Channel 1: data[1], etc
typedef struct {
  float* m_data;
} WavSample;

typedef struct {
  size_t nr_of_samples;
  uint8_t nr_of_channels;
  WavSample* m_samples;
} WavData;