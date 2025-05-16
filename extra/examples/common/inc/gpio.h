#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CH58x_common.h"

typedef struct {
  const PUINT32V portOut;
  const uint32_t pin;
} Gpio;

void gpioMode(const Gpio * const gpio, GPIOModeTypeDef mode);

#define gpioReset(gpio) (*((gpio)->portOut + 1) |= (gpio)->pin)
#define gpioSet(gpio) (*((gpio)->portOut) |= (gpio)->pin)
#define gpioWrite(gpio, state) ((state) ? gpioSet(gpio) : gpioReset(gpio))

#define gpioInverse(gpio) (*((gpio)->portOut) ^= (gpio)->pin)
#define gpioRead(gpio) (*((gpio)->portOut - 1) & (gpio)->pin)

#ifdef __cplusplus
}
#endif

#endif
