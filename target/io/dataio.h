#ifndef DATAIO_H
#define DATAIO_H
#include "config.h"
#include "io/header.h"

#if PLATFORM_HOST

stego_status_t read_float32_bin(const char *path, float *buf, uint32_t max_count, uint32_t *actual_count);
stego_status_t write_float32_bin(const char *path, const float *buf, uint32_t count);
stego_status_t read_header_file(const char *path, stego_header_t *hdr);
stego_status_t write_header_file(const char *path, stego_header_t *hdr);

#endif

#endif
