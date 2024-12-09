#include "CH58x_common.h"

// On ch583evt, you may jumper connect
// PB18 - LED1
// PB19 - LED2
// to see Leds blink

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  GPIOB_ModeCfg(GPIO_Pin_18, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(GPIO_Pin_19, GPIO_ModeIN_PU);

  GPIOB_ResetBits(GPIO_Pin_18);
  GPIOB_ResetBits(GPIO_Pin_19);

  R32_PB_DIR |= GPIO_Pin_18;
  // GPIOB_SetBits(GPIO_Pin_19);

  while(1) {
    // GPIOB_InverseBits(GPIO_Pin_18 | GPIO_Pin_19);
    R32_PB_DIR ^= GPIO_Pin_19;
    DelayMs(1000);
  }
}


// config GPIO as Open-drain output
// use DIR to control state, H: 0, L: 1
