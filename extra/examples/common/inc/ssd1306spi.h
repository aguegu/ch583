#ifndef SSD1306_H
#define SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "gpio.h"

typedef struct {
  const Gpio sclk;
  const Gpio mosi;
  const Gpio rst;
  const Gpio dc;
  const Gpio cs;

  uint8_t width;
  uint8_t height;
  uint16_t bufferLength;
  uint8_t * buffer;
  uint8_t bytesPerColumn;
} SSDspi;

void ssdInit(SSDspi * ssd, uint8_t width, uint8_t height);

void ssdPutBuffer(SSDspi *ssd, const uint8_t * buffer, uint8_t length, uint8_t bytesInColumn, uint8_t row, uint8_t col);

void ssdPutFont(SSDspi * ssd, char c, uint8_t row, uint8_t col);
void ssdPutString(SSDspi * ssd, const char * s, uint8_t row, uint8_t col);
void ssdRefresh(SSDspi * ssd);

void ssdClear(SSDspi *ssd, uint8_t v);

#ifdef __cplusplus
}
#endif

#endif
