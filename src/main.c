#include <stdio.h>

#include "wav_sampling.h"

size_t get_file_size(FILE* fp) {
    // Determine the file size
    fseek(fp, 0, SEEK_END);

    size_t file_size = ftell(fp);
    rewind(fp);
    return file_size;
}

int main() {
    WavDecoder decoder;
    // Initialize the decoder
    if (wav_decoder_init(&decoder, "wav_audio_48000_stereo.wav")) {
        return 1;
    }

    wav_decoder_get_header(&decoder);
    wav_print_header(decoder.header);

    WavEncoder encoder;
    // Initialize the encoder
    if (wav_encoder_init(&encoder, "wav_audio_5512_mono.wav")) {
        return 1;
    }

    wav_encoder_set_header(&encoder, 5512, BITS_PER_SAMPLE_16, AUDIO_FORMAT_PCM, MONO, 60);
    wav_print_header(encoder.header);

    wav_resample(&decoder, &encoder, DOWNSAMPLE_AVERAGE);

    wav_decoder_close(&decoder);
    wav_encoder_close(&encoder);

    return 0;
}
