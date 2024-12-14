#ifndef __UART1_H
#define __UART1_H

#include "CH58x_common.h"
#include "ringbuffer.h"

int _write(int fd, char *buf, int size);
void uart1FlushTx();

void uart1Init();

BOOL uart1RxAvailable();

uint8_t uart1RxGet();

#endif
