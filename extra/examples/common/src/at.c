#include "at.h"

void sendOK() { printf("\r\nOK\r\n"); }

void sendError() { printf("\r\nERROR\r\n"); }

BOOL startsWith(const char *str1, const char *str2) {
  const uint8_t l = strlen(str2);
  return strlen(str1) < l ? FALSE : memcmp(str1, str2, l) == 0;
}

uint8_t hexCharToNum(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return 0x00;
}

uint8_t genPayload(char *from, uint8_t *to) {
  uint8_t j = 0, i = 0;
  while (*from) {
    if (j & 1) {
      to[i] = (to[i] & 0xf0) | hexCharToNum(*from);
      i++;
    } else {
      to[i] = (to[i] & 0x0f) | (hexCharToNum(*from) << 4);
    }
    j++;
    from++;
  }
  return i;
}
