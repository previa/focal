#pragma once

#include "wav.h"

#define DECODER_PROCESS_SIZE 1024  // In bytes
#define DECODER_SAMPLE_SIZE 1000   // In samples

typedef struct {
    ByteBuffer* buffer;
    FILE* fp;
    size_t file_size;
    WavData* data;
    WavHeader* header;
    size_t nr_of_samples;
    size_t remaining_samples;
} WavDecoder;

static void free_data(WavData* data) {
    if (data->m_samples && data->nr_of_samples != 0) {
        for (size_t i = 0; i < data->nr_of_samples; i++) {
            free(data->m_samples[i].m_data);
        }

        free(data->m_samples);
    }

    data->nr_of_samples = 0;
}

static void set_file_size(WavDecoder* decoder) {
    fseek(decoder->fp, 0, SEEK_END);
    decoder->file_size = ftell(decoder->fp);
    rewind(decoder->fp);
}

static inline void wav_decoder_close(WavDecoder* decoder) {
    byte_buffer_close(decoder->buffer);
    if (decoder->data) {
        free_data(decoder->data);
        free(decoder->data);
    }

    if (decoder->header) {
        free(decoder->header);
    }

    fclose(decoder->fp);
}

static inline int wav_decoder_init(WavDecoder* decoder, const char* filename) {
    decoder->fp = fopen(filename, "rb");
    if (decoder->fp == NULL) {
        fprintf(stderr, "[WavDecoder] Unable to open file %s for reading\n", filename);
        return 1;
    }

    // Get the file size
    set_file_size(decoder);

    // Initialize the ByteBuffer
    byte_buffer_init(&(decoder->buffer), decoder->fp, decoder->file_size, DECODER_PROCESS_SIZE);

    decoder->header = NULL;

    decoder->data = (WavData*)malloc(sizeof(WavData));
    if (decoder->data == NULL) {
        fprintf(stderr, "[WavDecoder] Unable to allocate memory for decoder data\n");
        return 1;
    }

    return 0;
}

static inline int wav_decoder_get_next_samples(WavDecoder* decoder) {
    if (!decoder->remaining_samples) {
        return 0;
    }

    // If the data already contains samples remove them
    if (decoder->data->m_samples) {
        free(decoder->data->m_samples);
    }

    size_t samples = DECODER_SAMPLE_SIZE;
    if (decoder->remaining_samples < DECODER_SAMPLE_SIZE) {
        samples = decoder->remaining_samples;
    }

    // Allocate memory for the samples
    decoder->data->m_samples = (WavSample*)malloc(sizeof(WavSample) * samples);
    if (decoder->data->m_samples == NULL) {
        fprintf(stderr, "[WavDecoder] Unable to allocate memory for samples\n");
        return 1;
    }

    // Get the samples
    for (size_t i = 0; i < samples; i++) {
        // Allocate data for each sample
        decoder->data->m_samples[i].m_data = (float*)malloc(sizeof(float) * decoder->header->num_of_channels);
        if (decoder->data->m_samples[i].m_data == NULL) {
            fprintf(stderr, "[WavDecoder] Unable to allocate memory for channel data\n");
            return 1;
        }

        for (uint8_t c = 0; c < decoder->header->num_of_channels; c++) {
            if (decoder->header->bits_per_sample == 16) {
                decoder->data->m_samples[i].m_data[c] = (float)byte_buffer_read_int16(decoder->buffer, LE) / INT16_MAX;
            } else if (decoder->header->bits_per_sample == 8) {
                decoder->data->m_samples[i].m_data[c] = (float)byte_buffer_read_int8(decoder->buffer, LE) / INT8_MAX;
            }
        }
    }

    decoder->data->nr_of_samples = samples;
    decoder->remaining_samples -= samples;

    return samples;
}

static inline int wav_decoder_get_header(WavDecoder* decoder) {
    decoder->header = (WavHeader*)malloc(sizeof(WavHeader));

    if (decoder->header == NULL) {
        fprintf(stderr, "[WavDecoder] Unable to allocate memory for header\n");
        return 1;
    }

    printf("\nReading WAV header...\n");
    if (byte_buffer_read_int32(decoder->buffer, BE) != HEADER_CHUNK_ID) {
        fprintf(stderr, "[WavDecoder] Unable to parse header: did not find ChunkID\n");
        goto ERROR;
    }

    decoder->header->chunk_size = byte_buffer_read_int32(decoder->buffer, LE);

    if (byte_buffer_read_int32(decoder->buffer, BE) != HEADER_FORMAT) {
        fprintf(stderr, "[WavDecoder] Unable to parse header: did not find Format\n");
        goto ERROR;
    }

    if (byte_buffer_read_int32(decoder->buffer, BE) != HEADER_SUBCHUNK_1_ID) {
        fprintf(stderr, "[WavDecoder] Unable to parse header: did not find Subchunk1ID\n");
        goto ERROR;
    }

    decoder->header->subchunk_1_size = byte_buffer_read_int32(decoder->buffer, LE);

    decoder->header->audio_format = byte_buffer_read_int16(decoder->buffer, LE);
    decoder->header->num_of_channels = byte_buffer_read_int16(decoder->buffer, LE);

    decoder->header->sample_rate = byte_buffer_read_int32(decoder->buffer, LE);
    decoder->header->byte_rate = byte_buffer_read_int32(decoder->buffer, LE);

    decoder->header->block_align = byte_buffer_read_int16(decoder->buffer, LE);
    decoder->header->bits_per_sample = byte_buffer_read_int16(decoder->buffer, LE);

    // TODO: Support other WAV specs
    if (byte_buffer_read_int32(decoder->buffer, BE) != HEADER_SUBCHUNK_2_ID) {
        fprintf(stderr, "[WavDecoder] Unable to parse header: did not find Subchunk2ID\n");
        goto ERROR;
    }

    decoder->header->subchunk_2_size = byte_buffer_read_int32(decoder->buffer, LE);

    decoder->nr_of_samples = decoder->header->subchunk_2_size / decoder->header->block_align;
    decoder->remaining_samples = decoder->nr_of_samples;

    return 0;

ERROR:
    free(decoder->header);
    return 1;
}