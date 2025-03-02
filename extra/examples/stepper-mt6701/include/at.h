#ifndef __AT_H
#define __AT_H

#include <stdio.h>
#include "CH58x_common.h"

void sendOK(void);
void sendError(void);

typedef struct {
  const char* command;
  const BOOL isEqual; // 1: exactly match, 0: startsWith
  void (*handler)(uint8_t *, uint8_t len);
} CommandHandler;

BOOL startsWith(const char * str1, const char * str2);

uint8_t genPayload(char *from, uint8_t *to);

#endif
