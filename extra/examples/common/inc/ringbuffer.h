#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "CH58x_common.h"

typedef struct {
  uint8_t *buffer;
  uint8_t indexIn;
  uint8_t indexOut;
  uint8_t mask;
} RingBuffer;

void ringbufferInit(RingBuffer * rb, uint16_t size);

void ringbufferPut(RingBuffer * rb, uint8_t c, BOOL waitForConsuming);

uint8_t ringbufferGet(RingBuffer * rb);

BOOL ringbufferAvailable(RingBuffer *rb);

#ifdef __cplusplus
}
#endif

#endif
