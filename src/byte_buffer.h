#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum EndianType {
    BE,  // Big Endian
    LE   // Little Endian
};

enum HasFinished { Yes, No };

typedef struct {
    uint8_t *m_buffer;
    size_t m_offset;
    size_t m_size;
    size_t m_remaining;
    size_t m_read_size;

    FILE *m_file;

    enum HasFinished m_finished;
} ByteBuffer;

// Try loading the given amount (m_read_size) of data from given file (m_file)
// into the given buffer
static int load_data_into_buffer(ByteBuffer *buffer) {
    size_t read = buffer->m_read_size;
    if (buffer->m_remaining < read) {
        read = buffer->m_remaining;
    }

    // First ever read
    if (buffer->m_buffer == NULL) {
        buffer->m_buffer = (uint8_t *)malloc(sizeof(uint8_t) * read);
        if (buffer->m_buffer == NULL) {
            fprintf(stderr, "[ByteBuffer] Unable to allocate data for first file read\n");
            return 0;
        }
        buffer->m_size = 0;
    } else if (buffer->m_offset < buffer->m_size) {  // There is still unread data
        size_t remaining = buffer->m_size - buffer->m_offset;

        // Put the last bytes in front
        memmove(buffer->m_buffer, buffer->m_buffer + buffer->m_offset, remaining);
        buffer->m_buffer = (uint8_t *)realloc(buffer->m_buffer, read + remaining);

        buffer->m_size = remaining;
    } else if (buffer->m_offset == buffer->m_size) {  // There is no data left
        buffer->m_size = 0;
    }

    size_t len = fread(buffer->m_buffer + buffer->m_size, sizeof(uint8_t), read, buffer->m_file);

    if (len != read) {
        fprintf(stderr, "[ByteBuffer] Unable to read from file\n");
        buffer->m_finished = Yes;
        return 0;
    }

    buffer->m_size += len;
    buffer->m_remaining -= len;
    buffer->m_offset = 0;

    return len;
}

// Initialize the byte buffer, the owner of the ByteBuffer** needs to close
// the buffer with the 'byte_buffer_close' function or manually
static inline int byte_buffer_init(ByteBuffer **buffer, FILE *fp, size_t size, size_t read_size) {
    (*buffer) = (ByteBuffer *)malloc(sizeof(ByteBuffer));

    if ((*buffer) == NULL) {
        fprintf(stderr, "[ByteBuffer] Unable to allocate memory for ByteBuffer\n");
        return 1;
    }

    (*buffer)->m_remaining = size;
    (*buffer)->m_file = fp;
    (*buffer)->m_offset = 0;
    (*buffer)->m_read_size = read_size;

    // This is a buffer made for writing
    if (read_size == 0) {
        (*buffer)->m_buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
        (*buffer)->m_size = size;
    }

    (*buffer)->m_finished = No;

    return 0;
}

// Close and free the ByteBuffer
static inline int byte_buffer_close(ByteBuffer *buffer) {
    if (buffer) {
        if (buffer->m_buffer) {
            free(buffer->m_buffer);
        }

        free(buffer);
    }

    return 0;
}

// Check if the ByteBuffer has remaining (in bytes) data to read from,
// if not try loading more data.
// If no data can be read the m_finished flag is set accordingly
static inline int byte_buffer_has_remaining(ByteBuffer *buffer, uint8_t remaining) {
    if (buffer->m_size - buffer->m_offset >= remaining) {
        return 1;
    }

    // Buffer made for writing does not need to load data
    if (buffer->m_read_size == 0) {
        return 0;
    }

    if (!buffer->m_remaining) {
        buffer->m_finished = Yes;
        return 0;
    }

    if (load_data_into_buffer(buffer)) {
        return 1;
    }

    return 0;
}

// Read signed 8 bit of data into int8_t (using the given endianness)
static inline int8_t byte_buffer_read_int8(ByteBuffer *buffer, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 1)) {
        return 0;
    }

    int8_t ret = buffer->m_buffer[buffer->m_offset];

    buffer->m_offset += 1;

    return ret;
}

// Read signed 16 bit of data into int16_t (using the given endianness)
static inline int16_t byte_buffer_read_int16(ByteBuffer *buffer, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 2)) {
        return 0;
    }

    int16_t ret;

    if (type == BE) {
        ret = buffer->m_buffer[buffer->m_offset] << 8 | buffer->m_buffer[buffer->m_offset + 1];
    } else {
        ret = buffer->m_buffer[buffer->m_offset + 1] << 8 | buffer->m_buffer[buffer->m_offset];
    }

    buffer->m_offset += 2;

    return ret;
}

// Read signed 32 bit of data into int32_t (using the given endianness)
static inline int32_t byte_buffer_read_int32(ByteBuffer *buffer, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 4)) {
        return 0;
    }

    int32_t ret;

    if (type == BE) {
        ret = buffer->m_buffer[buffer->m_offset + 0] << 24 | buffer->m_buffer[buffer->m_offset + 1] << 16 |
              buffer->m_buffer[buffer->m_offset + 2] << 8 | buffer->m_buffer[buffer->m_offset + 3];
    } else {
        ret = buffer->m_buffer[buffer->m_offset + 3] << 24 | buffer->m_buffer[buffer->m_offset + 2] << 16 |
              buffer->m_buffer[buffer->m_offset + 1] << 8 | buffer->m_buffer[buffer->m_offset + 0];
    }

    buffer->m_offset += 4;

    return ret;
}

static inline int byte_buffer_write_int8(ByteBuffer *buffer, int8_t num, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 1)) {
        return 0;
    }

    buffer->m_buffer[buffer->m_offset] = (uint8_t)(num);

    buffer->m_offset += 1;

    return 1;
}

static inline int byte_buffer_write_int16(ByteBuffer *buffer, int16_t num, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 2)) {
        return 0;
    }

    if (type == BE) {
        buffer->m_buffer[buffer->m_offset] = (uint8_t)(num >> 8);
        buffer->m_buffer[buffer->m_offset + 1] = (uint8_t)(num);
    } else {
        buffer->m_buffer[buffer->m_offset + 1] = (uint8_t)(num >> 8);
        buffer->m_buffer[buffer->m_offset] = (uint8_t)(num);
    }

    buffer->m_offset += 2;

    return 1;
}

static inline int byte_buffer_write_int32(ByteBuffer *buffer, int32_t num, enum EndianType type) {
    if (!byte_buffer_has_remaining(buffer, 4)) {
        return 0;
    }

    if (type == BE) {
        buffer->m_buffer[buffer->m_offset] = (uint8_t)(num >> 24);
        buffer->m_buffer[buffer->m_offset + 1] = (uint8_t)(num >> 16);
        buffer->m_buffer[buffer->m_offset + 2] = (uint8_t)(num >> 8);
        buffer->m_buffer[buffer->m_offset + 3] = (uint8_t)(num);
    } else {
        buffer->m_buffer[buffer->m_offset + 3] = (uint8_t)(num >> 24);
        buffer->m_buffer[buffer->m_offset + 2] = (uint8_t)(num >> 16);
        buffer->m_buffer[buffer->m_offset + 1] = (uint8_t)(num >> 8);
        buffer->m_buffer[buffer->m_offset] = (uint8_t)(num);
    }

    buffer->m_offset += 4;

    return 1;
}

static inline void byte_buffer_write_buffer(ByteBuffer *buffer) {
    for (size_t i = 0; i < buffer->m_remaining; i++) {
        fputc(buffer->m_buffer[i], buffer->m_file);
    }
}