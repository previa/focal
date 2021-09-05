#pragma once

#include "byte_buffer.h"
#include "wav_constants.h"

static void wav_decoder_data_free(WavData* data) {
  if (data->m_samples && data->nr_of_samples != 0) {
    for (size_t i = 0; i < data->nr_of_samples; i++) {
      free(data->m_samples[i].m_data);
    }

    free(data->m_samples);
  }

  data->nr_of_samples = 0;
}

static inline int wav_decoder_data_get(WavData* data, ByteBuffer* buffer,
                                       size_t nr_of_samples) {
  data->m_samples = (WavSample*)malloc(sizeof(WavSample) * nr_of_samples);
  if (data->m_samples == NULL) {
    fprintf(stderr, "[WavData] Unable to allocate memory for samples\n");
    return 1;
  }

  for (size_t i = 0; i < nr_of_samples; i++) {
    data->m_samples[i].m_data =
        (float*)malloc(sizeof(float) * data->nr_of_channels);
    if (data->m_samples[i].m_data == NULL) {
      fprintf(stderr,
              "[WavData] Unable to allocate memory for sample channels\n");
      data->nr_of_samples = i;
      wav_decoder_data_free(data);
      break;
    }

    for (uint8_t c = 0; c < data->nr_of_channels; c++) {
      data->m_samples[i].m_data[c] =
          (float)byte_buffer_read_int16(buffer, LE) / INT16_MAX;
    }
  }

  data->nr_of_samples = nr_of_samples;

  return !(data->nr_of_samples == nr_of_samples);
}

static inline int wav_decoder_header_get(WavHeader* header,
                                         ByteBuffer* buffer) {
  printf("\nReading WAV header...\n");
  if (byte_buffer_read_int32(buffer, BE) != HEADER_CHUNK_ID) {
    fprintf(stderr,
            "[WavHeader] Unable to parse header: did not find ChunkID\n");
    return 1;
  }

  header->chunk_size = byte_buffer_read_int32(buffer, LE);

  if (byte_buffer_read_int32(buffer, BE) != HEADER_FORMAT) {
    fprintf(stderr,
            "[WavHeader] Unable to parse header: did not find Format\n");
    return 1;
  }

  if (byte_buffer_read_int32(buffer, BE) != HEADER_SUBCHUNK_1_ID) {
    fprintf(stderr,
            "[WavHeader] Unable to parse header: did not find Subchunk1ID\n");
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
  if (byte_buffer_read_int32(buffer, BE) != HEADER_SUBCHUNK_2_ID) {
    fprintf(stderr,
            "[WavHeader] Unable to parse header: did not find Subchunk2ID\n");
    return 1;
  }

  header->subchunk_2_size = byte_buffer_read_int32(buffer, LE);

  return 0;
}