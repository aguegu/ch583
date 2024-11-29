#include "ringbuffer.h"


void ringbufferInit(RingBuffer * rb, uint16_t size) {
  rb->buffer = (uint8_t *)malloc(size);
  rb->indexIn = 0;
  rb->indexOut = 0;
  rb->mask = size - 1;
}

__HIGH_CODE void ringbufferPut(RingBuffer * rb, uint8_t c, BOOL waitForConsuming) {
  uint8_t indexInNext = (rb->indexIn + 1) & rb->mask;
  while (waitForConsuming && indexInNext == rb->indexOut) {
    __WFI();
  };
  rb->buffer[rb->indexIn] = c;
  rb->indexIn = indexInNext;
}

__HIGH_CODE uint8_t ringbufferGet(RingBuffer * rb) {
  uint8_t indexOutNext = (rb->indexOut + 1) & rb->mask;
  uint8_t c = rb->buffer[rb->indexOut];
  rb->indexOut = indexOutNext;
  return c;
}

__HIGH_CODE BOOL ringbufferAvailable(RingBuffer *rb) {
  return rb->indexIn != rb->indexOut;
}
