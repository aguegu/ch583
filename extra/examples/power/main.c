#include <stdio.h>
#include "CH58x_common.h"

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    while (R8_UART1_TFC == UART_FIFO_SIZE);
    R8_UART1_THR = *buf++;
  }
  return size;
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  // SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOB_ModeCfg(GPIO_Pin_18, GPIO_ModeOut_PP_5mA);  // Led1
  GPIOB_ModeCfg(GPIO_Pin_19, GPIO_ModeOut_PP_5mA);  // Led2
  GPIOB_InverseBits(GPIO_Pin_18);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_BaudRateCfg(9600); // uncomment if prefer other baudrate

  printf("ChipID: %02x\t SysClock: %ldHz\n", R8_CHIP_ID, GetSysClock());

  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
  GPIOB_ITModeCfg(GPIO_Pin_4, GPIO_ITMode_FallEdge); // wake on fall edge
  PFIC_EnableIRQ(GPIO_B_IRQn);
  PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

  printf("IDLE mode sleep.\n");
  while (!(R8_UART1_LSR & 0x40)); // flush uart1 tx

  LowPower_Idle();  // idle mode

  // LowPower_Halt(); // halt mode

  // LowPower_Sleep(RB_PWR_RAM30K | RB_PWR_RAM2K); // 只保留30+2K SRAM 供电

  // LowPower_Shutdown(0); // shutdown mode, reset after wakeup

  // HSECFG_Current(HSE_RCur_100);

  printf("wake.. \n");

  while(1) {
    GPIOB_InverseBits(GPIO_Pin_18 | GPIO_Pin_19);
    DelayMs(500);
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  SysTick->SR = 0;
}

__INTERRUPT
__HIGH_CODE
void GPIOB_IRQHandler(void) {
  GPIOB_ClearITFlagBit(GPIO_Pin_4);
}
