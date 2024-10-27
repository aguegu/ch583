#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CH58x_common.h"
#include <stdlib.h>

typedef struct {
  uint8_t *buffer;
  uint8_t indexIn;
  uint8_t indexOut;
  uint8_t mask;
} RingBuffer;

void ringbuffer_init(RingBuffer * rb, uint16_t size);

void ringbuffer_put(RingBuffer * rb, uint8_t c, BOOL waitForConsuming);

uint8_t ringbuffer_get(RingBuffer * rb);

BOOL ringbuffer_available(RingBuffer *rb);

#ifdef __cplusplus
}
#endif

#endif
