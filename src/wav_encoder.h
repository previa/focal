#pragma once

#include "wav.h"

typedef struct {
    WavHeader* header;
    WavData* data;
    FILE* fp;
    uint32_t audio_length;
    size_t nr_of_samples;
} WavEncoder;

static void calculate_header_values(WavHeader* header, size_t samples) {
    if (header->audio_format == AUDIO_FORMAT_PCM) {
        header->subchunk_1_size = 16;
    } else {
        fprintf(stderr, "[WavEncoder] Unsupported audio format\n");
        return;
    }

    header->subchunk_2_size = samples * header->num_of_channels * (header->bits_per_sample / 8);
    header->block_align = header->num_of_channels * (header->bits_per_sample / 8);
    header->byte_rate = header->sample_rate * header->num_of_channels * (header->bits_per_sample / 8);
    header->chunk_size = 36 + header->subchunk_2_size;
}

static inline void wav_encoder_close(WavEncoder* encoder) {
    if (encoder->data) {
        free_data(encoder->data);
        free(encoder->data);
    }

    if (encoder->header) {
        free(encoder->header);
    }
    fclose(encoder->fp);
}

static inline int wav_encoder_init(WavEncoder* encoder, const char* filename) {
    encoder->fp = fopen(filename, "wb");
    if (encoder->fp == NULL) {
        fprintf(stderr, "[WavEncoder] Unable to open file %s for reading\n", filename);
        return 1;
    }

    encoder->header = NULL;
    encoder->data = (WavData*)malloc(sizeof(WavData));
    encoder->nr_of_samples = 0;

    return 0;
}

static inline int wav_encoder_set_header(WavEncoder* encoder, uint32_t sample_rate, uint16_t bits_per_sample, uint16_t audio_format,
                                         uint16_t num_of_channels, uint32_t audio_length_in_seconds) {
    encoder->header = (WavHeader*)malloc(sizeof(WavHeader));
    if (encoder->header == NULL) {
        fprintf(stderr, "[WavEncoder] Unable to allocate memory for header\n");
        return 1;
    }

    encoder->header->sample_rate = sample_rate;
    encoder->header->bits_per_sample = bits_per_sample;
    encoder->header->audio_format = audio_format;
    encoder->header->num_of_channels = num_of_channels;
    encoder->audio_length = audio_length_in_seconds;

    calculate_header_values(encoder->header, (sample_rate * audio_length_in_seconds));

    encoder->nr_of_samples = (encoder->header->sample_rate * audio_length_in_seconds);

    return 0;
}

static inline int wav_encoder_write_header(WavEncoder* encoder) {
    // Load header into ByteBuffer for writing
    ByteBuffer* buffer;
    byte_buffer_init(&buffer, encoder->fp, HEADER_LENGTH_1, 0);

    byte_buffer_write_int32(buffer, HEADER_CHUNK_ID, BE);
    byte_buffer_write_int32(buffer, encoder->header->chunk_size, LE);
    byte_buffer_write_int32(buffer, HEADER_FORMAT, BE);

    // The "fmt" subchunk describes the sound data's format:
    byte_buffer_write_int32(buffer, HEADER_SUBCHUNK_1_ID, BE);
    byte_buffer_write_int32(buffer, encoder->header->subchunk_1_size, LE);
    byte_buffer_write_int16(buffer, encoder->header->audio_format, LE);
    byte_buffer_write_int16(buffer, encoder->header->num_of_channels, LE);
    byte_buffer_write_int32(buffer, encoder->header->sample_rate, LE);
    byte_buffer_write_int32(buffer, encoder->header->byte_rate, LE);
    byte_buffer_write_int16(buffer, encoder->header->block_align, LE);
    byte_buffer_write_int16(buffer, encoder->header->bits_per_sample, LE);

    // The "data" subchunk contains the size of the data and the actual data
    byte_buffer_write_int32(buffer, HEADER_SUBCHUNK_2_ID, BE);
    byte_buffer_write_int32(buffer, encoder->header->subchunk_2_size, LE);

    byte_buffer_write_buffer(buffer);

    byte_buffer_close(buffer);
    return 0;
}

static inline int wav_encoder_write_data(WavEncoder* encoder) {
    ByteBuffer* buffer;
    byte_buffer_init(&buffer, encoder->fp, encoder->data->nr_of_samples, 0);

    for (size_t i = 0; i < encoder->data->nr_of_samples; i++) {
        for (uint8_t c = 0; c < encoder->header->num_of_channels; c++) {
            if (encoder->header->bits_per_sample == 16) {
                byte_buffer_write_int16(buffer, encoder->data->m_samples[i].m_data[c] * INT16_MAX, LE);
            } else if (encoder->header->bits_per_sample == 8) {
                byte_buffer_write_int8(buffer, encoder->data->m_samples[i].m_data[c] * INT8_MAX, LE);
            } else {
                fprintf(stderr, "[WavEncoder] Unsupported bits per sample (not 8 or 16)\n");
                break;
            }
        }
    }

    // Write buffer to file
    byte_buffer_write_buffer(buffer);

    byte_buffer_close(buffer);

    return 0;
}