#include "MESH_LIB.h"

char toHex(uint8_t c) {
  return c > 9 ? 'A' + c - 10 : c + '0';
}

void putHex(uint8_t c) {
  putchar(toHex((c & 0xf0) >> 4));
  putchar(toHex(c & 0x0f));
}

void app_log(char *s, uint8_t * p, uint8_t len) {
  printf("APP_LOG,");
  printf(s);
  putchar(',');
  for (unsigned char i = 0; i < len; i++) {
    putHex(*p++);
  }
  putchar('\n');
}
