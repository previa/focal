#include "byte_buffer.h"
#include "wav_constants.h"
#include "wav_decoder.h"

#define SAMPLE_PROCESS_SIZE 1024

static WavSample* wav_upsample(WavData* data, size_t nr_of_samples,
                               uint8_t upsample_scale, uint8_t nr_of_channels) {
  size_t new_nr_of_samples = (nr_of_samples * upsample_scale);

  WavSample* upsample_samples =
      (WavSample*)malloc(sizeof(WavSample) * new_nr_of_samples);
  if (data->m_samples == NULL) {
    fprintf(stderr,
            "[Wav Upsample] Unable to allocate memory for new samples\n");
    return NULL;
  }

  for (size_t i = 0; i < new_nr_of_samples; i++) {
    upsample_samples[i].m_data = (float*)malloc(sizeof(float) * nr_of_channels);
    if (upsample_samples[i].m_data == NULL) {
      fprintf(stderr, "[Wav Upsample] Unable to allocate memory for samples\n");
    }

    for (uint8_t c = 0; c < nr_of_channels; c++) {
      upsample_samples[i].m_data[c] =
          data->m_samples[i / upsample_scale].m_data[c];
    }
  }

  return upsample_samples;
}

static inline WavSample* wav_downsample_by_average(WavData* data,
                                                   size_t nr_of_samples,
                                                   uint8_t downsample_scale,
                                                   uint8_t nr_of_channels) {
  size_t new_nr_of_samples = (nr_of_samples / downsample_scale);

  WavSample* downsample_samples =
      (WavSample*)malloc(sizeof(WavSample) * new_nr_of_samples);
  if (data->m_samples == NULL) {
    fprintf(stderr,
            "[Wav Downsample] Unable to allocate memory for new samples\n");
    return NULL;
  }

  for (size_t i = 0; i < new_nr_of_samples; i++) {
    downsample_samples[i].m_data =
        (float*)malloc(sizeof(float) * nr_of_channels);
    if (downsample_samples[i].m_data == NULL) {
      fprintf(stderr, "[Wav Upsample] Unable to allocate memory for samples\n");
    }

    for (uint8_t c = 0; c < nr_of_channels; c++) {
      float sum = 0;
      for (int j = 0; j < downsample_scale; j++) {
        sum += data->m_samples[(i * downsample_scale) + j].m_data[c];
      }
      downsample_samples[i].m_data[c] = (sum / downsample_scale);
    }
  }

  return downsample_samples;
}

// M is the downsample_scale
static inline WavSample* wav_downsample_by_M(WavData* data,
                                             size_t nr_of_samples,
                                             uint8_t downsample_scale,
                                             uint8_t nr_of_channels) {
  // TODO: This shares the same code as downsample_by_average
  size_t new_nr_of_samples = (nr_of_samples / downsample_scale);

  WavSample* downsample_samples =
      (WavSample*)malloc(sizeof(WavSample) * new_nr_of_samples);
  if (data->m_samples == NULL) {
    fprintf(stderr,
            "[Wav Downsample] Unable to allocate memory for new samples\n");
    return NULL;
  }

  for (size_t i = 0; i < new_nr_of_samples; i++) {
    downsample_samples[i].m_data =
        (float*)malloc(sizeof(float) * nr_of_channels);
    if (downsample_samples[i].m_data == NULL) {
      fprintf(stderr, "[Wav Upsample] Unable to allocate memory for samples\n");
    }

    for (uint8_t c = 0; c < nr_of_channels; c++) {
      downsample_samples[i].m_data[c] =
          data->m_samples[i * downsample_scale].m_data[c];
    }
  }

  return downsample_samples;
}

static void wav_resample_data(WavHeader* resample_header,
                              WavData* resampled_data,
                              WavHeader* original_header, WavData* data) {
  if (resample_header->sample_rate != 5512) {
    fprintf(
        stderr,
        "[Wav Resample] Resample only supports resampling to 5512 for now!\n");
    return;
  }

  if (original_header->sample_rate == 48000) {
    // 48000 * 7 / 61 is close enough to 5512

    // Upsample by 7
    WavData upsampled_data;
    upsampled_data.m_samples = wav_upsample(data, data->nr_of_samples, 7,
                                            resample_header->num_of_channels);
    upsampled_data.nr_of_samples = data->nr_of_samples * 7;

    // Downsample by 61
    resampled_data->m_samples =
        wav_downsample_by_average(&upsampled_data, upsampled_data.nr_of_samples,
                                  61, resample_header->num_of_channels);
    resampled_data->nr_of_samples = upsampled_data.nr_of_samples / 61;

    wav_decoder_data_free(&upsampled_data);
  } else {
    fprintf(stderr,
            "[Wav Resample] Resample only support original sample rate of "
            "48000 Hz for now!\n");
    return;
  }
}

static inline void wav_header_calculate(WavHeader* header, size_t samples) {
  header->subchunk_1_size = 16;
  header->subchunk_2_size =
      samples * header->num_of_channels * (header->bits_per_sample / 8);
  header->block_align = header->num_of_channels * (header->bits_per_sample / 8);
  header->byte_rate = header->sample_rate * header->num_of_channels *
                      (header->bits_per_sample / 8);
  header->chunk_size = 36 + header->subchunk_2_size;
}

static inline void wav_header_print(const WavHeader* header) {
  printf("\nWAV Header\n");
  printf("\tChunk size: %d bytes\n", header->chunk_size);
  printf("\tSubchunk1Size: %d bytes\n", header->subchunk_1_size);
  printf("\tAudio format: %d\n", header->audio_format);
  printf("\tChannels: %d channels\n", header->num_of_channels);
  printf("\tSample rate: %d Hz\n", header->sample_rate);
  printf("\tByte rate : %d bytes per second\n", header->byte_rate);
  printf("\tBlock align: %d bytes per sample\n", header->block_align);
  printf("\tBits per sample: %d bits per sample for one channel\n",
         header->bits_per_sample);
  printf("\tSubchunk2Size: %d bytes\n", header->subchunk_2_size);
  printf("\n");
}

static inline int wav_samples_to_csv(const char* filename, WavData* data) {
  FILE* fp = fopen(filename, "w");

  if (fp == NULL) {
    fprintf(stderr, "[WavData] Unable to open or create %s to write\n",
            filename);
    return 1;
  }

  for (size_t i = 0; i < data->nr_of_samples; i++) {
    fprintf(fp, "%zu;", i);
    for (uint8_t c = 0; c < data->nr_of_channels; c++) {
      fprintf(fp, "%0.5f;", data->m_samples[i].m_data[c]);
    }

    fprintf(fp, "\n");
  }

  return 0;
}

static inline int wav_resample(ByteBuffer* buffer, uint8_t nr_of_channels,
                               uint32_t sample_rate, uint16_t bits_per_sample,
                               size_t audio_length_in_seconds) {
  // Create the resample header
  WavHeader resample_header;
  resample_header.num_of_channels = nr_of_channels;
  resample_header.sample_rate = sample_rate;
  resample_header.bits_per_sample = bits_per_sample;

  WavHeader original_header;
  wav_decoder_header_get(&original_header, buffer);
  resample_header.audio_format = original_header.audio_format;

  if (audio_length_in_seconds == 0) {  // Full length
    audio_length_in_seconds =
        (original_header.subchunk_2_size / original_header.block_align) /
        original_header.sample_rate;
  }

  // The total nr of samples needed for resampling
  size_t total_nr_of_samples =
      audio_length_in_seconds * resample_header.sample_rate;
  // The total nr of samples from the original audio file
  size_t original_total_nr_of_samples =
      audio_length_in_seconds * original_header.sample_rate;
  // The total available nr of samples in the original file
  size_t original_sample_max =
      (original_header.subchunk_2_size / original_header.block_align);

  wav_header_calculate(&resample_header, total_nr_of_samples);

  wav_header_print(&original_header);
  printf("Audio length: %zu seconds\n", audio_length_in_seconds);
  printf("Resampling to: \n");
  wav_header_print(&resample_header);

  WavData data;
  WavData resampled_data;
  size_t i = 0;
  size_t j = 0;
  // Only when audio_length stays the same as the original, the resampled
  // version will be a bit shorter
  while (i < original_sample_max && j < total_nr_of_samples) {
    if (i + SAMPLE_PROCESS_SIZE < original_sample_max) {
      wav_decoder_data_get(&data, buffer, SAMPLE_PROCESS_SIZE);
    } else {
      wav_decoder_data_get(&data, buffer, original_sample_max - i);
    }
    i += data.nr_of_samples;

    wav_resample_data(&resample_header, &resampled_data, &original_header,
                      &data);
    j += resampled_data.nr_of_samples;
    wav_decoder_data_free(&data);

    wav_decoder_data_free(&resampled_data);
  }

  printf("Processed samples: %zu/%zu\n", i, original_total_nr_of_samples);
  printf("Resampled samples: %zu/%zu\n", j, total_nr_of_samples);

  return 0;
}