#include "gpio.h"

void gpioMode(const Gpio *const gpio, GPIOModeTypeDef mode) {
  switch (mode) {
  case GPIO_ModeIN_Floating:
    *(gpio->portOut + 3) &= ~gpio->pin; // R32_PX_PD_DRV
    *(gpio->portOut + 2) &= ~gpio->pin; // R32_PX_PU
    *(gpio->portOut - 2) &= ~gpio->pin; // R32_PX_DIR
    break;

  case GPIO_ModeIN_PU:
    *(gpio->portOut + 3) &= ~gpio->pin; // R32_PX_PD_DRV
    *(gpio->portOut + 2) |= gpio->pin;  // R32_PX_PU
    *(gpio->portOut - 2) &= ~gpio->pin; // R32_PX_DIR
    break;

  case GPIO_ModeIN_PD:
    *(gpio->portOut + 3) |= gpio->pin;  // R32_PX_PD_DRV
    *(gpio->portOut + 2) &= ~gpio->pin; // R32_PX_PU
    *(gpio->portOut - 2) &= ~gpio->pin; // R32_PX_DIR
    break;

  case GPIO_ModeOut_PP_5mA:
    *(gpio->portOut + 3) &= ~gpio->pin; // R32_PX_PD_DRV
    *(gpio->portOut - 2) |= gpio->pin;  // R32_PX_DIR
    break;

  case GPIO_ModeOut_PP_20mA:
    *(gpio->portOut + 3) |= gpio->pin; // R32_PX_PD_DRV
    *(gpio->portOut - 2) |= gpio->pin; // R32_PX_DIR
    break;

  default:
    break;
  }
}
