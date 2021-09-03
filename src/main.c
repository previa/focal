#include <stdio.h>

#include "wav_decoder.h"

int main() {
    size_t read_size = 12 * 1024;

    FILE *fp = fopen("wav_audio_48000_stereo.wav", "r");

    if(fp == NULL) {
	fprintf(stderr, "Unable to open file\n");
	return 1;
    }

    // Determine the file size
    fseek(fp, 0, SEEK_END);

    size_t file_size = ftell(fp);
    rewind(fp);

    printf("File size: %zu\n", file_size);

    ByteBuffer *buffer;
    byte_buffer_init(&buffer, fp, file_size, read_size);
    
    // Get the WAV header
    WavHeader header;
    wav_header_get(&header, buffer);
    wav_header_print(&header);


    byte_buffer_close(buffer);

    return 0;
}
