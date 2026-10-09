#ifndef CRC32_H
#define CRC32_H
#include <stdint.h>

/* CRC-32 (ISO 3309 / ITU-T V.42 / Ethernet).
 * Polynomial: 0xEDB88320 (reversed). */

uint32_t crc32_compute(const void *data, uint32_t len);

#endif
