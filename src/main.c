#include <stdio.h>

#include "wav.h"

size_t get_file_size(FILE* fp) {
  // Determine the file size
  fseek(fp, 0, SEEK_END);

  size_t file_size = ftell(fp);
  rewind(fp);
  return file_size;
}

int main() {
  size_t read_size = 12 * 1024;

  FILE* fp = fopen("wav_audio_48000_stereo.wav", "r");

  if (fp == NULL) {
    fprintf(stderr, "Unable to open file\n");
    return 1;
  }

  size_t file_size = get_file_size(fp);
  printf("File size: %zu bytes\n", file_size);

  ByteBuffer* buffer;
  byte_buffer_init(&buffer, fp, file_size, read_size);

  // 5512 needed for audio fingerprinting
  wav_resample(buffer, 1, 5512, 16, 60);

  byte_buffer_close(buffer);

  fclose(fp);

  return 0;
}
