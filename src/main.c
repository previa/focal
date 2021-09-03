#include "byte_buffer.h"
#include <stdio.h>

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
	
	while(buffer->m_finished != Yes) {
		int32_t value = byte_buffer_read_int32(buffer, BE);
	}

	byte_buffer_close(buffer);

	return 0;
}
