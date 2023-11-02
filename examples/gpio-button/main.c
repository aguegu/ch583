#include "CH58x_common.h"

// On ch583evt, you may jumper connect
// PB18 - LED1
// PB19 - LED2
// to see Leds blink
// PB4 is connected to the Key Button onboard


int main() {
  SetSysClock(CLK_SOURCE_HSE_16MHz);

  GPIOB_ModeCfg(GPIO_Pin_18, GPIO_ModeOut_PP_5mA);  // Led 1
  GPIOB_ModeCfg(GPIO_Pin_19, GPIO_ModeOut_PP_5mA);  // Led 2
  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);        // Key

  while(1) {
    if (GPIOB_ReadPortPin(GPIO_Pin_4)) {
      GPIOB_SetBits(GPIO_Pin_18);
      GPIOB_ResetBits(GPIO_Pin_19);
    } else {
      GPIOB_ResetBits(GPIO_Pin_18);
      GPIOB_SetBits(GPIO_Pin_19);
    }
    DelayMs(20);
  }
}
