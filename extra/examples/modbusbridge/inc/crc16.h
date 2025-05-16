#ifndef CRC16_H
#define CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void appendCrc16(uint8_t *p, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif
