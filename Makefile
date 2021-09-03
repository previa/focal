CC=gcc
CFLAGS=-I. -Wall -lm
c_source_files := $(shell find src/ -name *.c)
c_header_file := $(shell find src/ -name *.h)

focal: $(c_source_files)
	$(CC) -o $@ $^ $(CFLAGS)
