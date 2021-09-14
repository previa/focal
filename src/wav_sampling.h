#pragma once

#include "wav_decoder.h"
#include "wav_encoder.h"

enum WavResamplingMethod { DOWNSAMPLE_AVERAGE, DOWNSAMPLE_M };

static inline float wav_downsample_by_M(WavEncoder *encoder, size_t sample, uint8_t channel, uint8_t M) {
    return encoder->data->m_samples[sample * M].m_data[channel];
}

static inline float wav_downsample_by_average(WavEncoder *encoder, size_t sample, uint8_t channel, uint8_t M) {
    float sum = 0;
    for (int j = 0; j < M; j++) {
        sum += encoder->data->m_samples[(sample * M) + j].m_data[channel];
    }
    return (sum / M);
}

// TODO: Make more generic
static inline int wav_downsample(WavDecoder *decoder, WavEncoder *encoder, uint8_t M, enum WavResamplingMethod method) {
    size_t samples = encoder->data->nr_of_samples / M;

    WavSample *new_samples = (WavSample *)malloc(sizeof(WavSample) * samples);
    if (new_samples == NULL) {
        fprintf(stderr, "[WavSampling] Unable to allocate memory for downsampled samples\n");
        return 0;
    }

    for (size_t i = 0; i < samples; i++) {
        for (uint8_t c = 0; c < encoder->header->num_of_channels; c++) {
            new_samples[i].m_data = (float *)malloc(sizeof(float) * encoder->header->num_of_channels);
            if (new_samples[i].m_data == NULL) {
                fprintf(stderr, "[WavSampling] Unable to allocate memory for downsamples samples channel\n");
                break;
            }
            if (method == DOWNSAMPLE_M) {
                new_samples[i].m_data[c] = wav_downsample_by_M(encoder, i, c, M);
            } else if (method == DOWNSAMPLE_AVERAGE) {
                new_samples[i].m_data[c] = wav_downsample_by_average(encoder, i, c, M);
            }
        }
    }

    free_data(encoder->data);
    encoder->data->m_samples = new_samples;

    encoder->data->nr_of_samples = samples;

    return samples;
}

static inline int wav_upsample(WavDecoder *decoder, WavEncoder *encoder, uint8_t M) {
    size_t samples = decoder->data->nr_of_samples * M;

    encoder->data->m_samples = (WavSample *)malloc(sizeof(WavSample) * samples);
    if (encoder->data->m_samples == NULL) {
        fprintf(stderr, "[WavSampling] Unable to allocate memory for samples to upsample\n");
        return 0;
    }

    for (size_t i = 0; i < samples; i++) {
        encoder->data->m_samples[i].m_data = (float *)malloc(sizeof(float) * encoder->header->num_of_channels);
        if (encoder->data->m_samples[i].m_data == NULL) {
            fprintf(stderr, "[WavSampling] Unable to allocate memory for channel data to upsample\n");
            return 0;
        }

        for (uint8_t c = 0; c < encoder->header->num_of_channels; c++) {
            encoder->data->m_samples[i].m_data[c] = decoder->data->m_samples[i / M].m_data[c];
        }
    }

    encoder->data->nr_of_samples = samples;

    return samples;
}

static inline int wav_resample(WavDecoder *decoder, WavEncoder *encoder, enum WavResamplingMethod method) {
    // Write the header
    wav_encoder_write_header(encoder);

    printf("There are %zu samples to get\n", decoder->nr_of_samples);

    if (encoder->header->sample_rate != 5512) {
        fprintf(stderr, "[WavSampling] Only resampling to 5512 Hz is supported for now\n");
        return 1;
    }

    size_t total_samples = 0;
    size_t total_sampled_samples = 0;

    while (total_sampled_samples < encoder->nr_of_samples) {
        // Get new samples
        wav_decoder_get_next_samples(decoder);

        total_samples += decoder->data->nr_of_samples;

        // Make sure there is no encoder data
        if (encoder->data != NULL) {
            free_data(encoder->data);
        }

        if (decoder->header->sample_rate == 48000) {
            // 48000 * 7 / 61 is close to 5512
            wav_upsample(decoder, encoder, 7);
            wav_downsample(decoder, encoder, 61, method);
            total_sampled_samples += encoder->data->nr_of_samples;
        }

        wav_encoder_write_data(encoder);
    }

    printf("Total samples processed: %zu/%zu\n", total_samples, decoder->nr_of_samples);
    printf("Total sampled samples: %zu/%zu\n", total_sampled_samples, encoder->nr_of_samples);

    return 0;
}