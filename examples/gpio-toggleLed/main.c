#include "CH58x_common.h"

// On ch583evt, you may jumper connect
// PB18 - LED1
// PB19 - LED2
// to see Leds blink

int main() {
  SetSysClock(CLK_SOURCE_HSE_16MHz);

  GPIOB_ModeCfg(GPIO_Pin_18, GPIO_ModeOut_PP_5mA);
  GPIOB_ModeCfg(GPIO_Pin_19, GPIO_ModeOut_PP_5mA);

  GPIOB_ResetBits(GPIO_Pin_18);
  GPIOB_SetBits(GPIO_Pin_19);

  while(1) {
    GPIOB_InverseBits(GPIO_Pin_18 | GPIO_Pin_19);
    DelayMs(1000);
  }
}
