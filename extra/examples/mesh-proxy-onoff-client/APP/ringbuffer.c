#include "ringbuffer.h"

void ringbuffer_init(RingBuffer * rb, uint16_t size) {
  rb->buffer = (uint8_t *)malloc(size);
  rb->indexIn = 0;
  rb->indexOut = 0;
  rb->mask = size - 1;
}

void ringbuffer_put(RingBuffer * rb, uint8_t c, BOOL waitForConsuming) {
  uint8_t indexInNext = (rb->indexIn + 1) & rb->mask;
  while (waitForConsuming && indexInNext == rb->indexOut) {
    __WFI();
  };
  rb->buffer[rb->indexIn] = c;
  rb->indexIn = indexInNext;
}

uint8_t ringbuffer_get(RingBuffer * rb) {
  uint8_t indexOutNext = (rb->indexOut + 1) & rb->mask;
  uint8_t c = rb->buffer[rb->indexOut];
  rb->indexOut = indexOutNext;
  return c;
}

BOOL ringbuffer_available(RingBuffer *rb) {
  return rb->indexIn != rb->indexOut;
}
