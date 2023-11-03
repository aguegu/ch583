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

  printf("ChipID: %02x\t SysClock: %ldHz\n", R8_CHIP_ID, GetSysClock());

  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
  GPIOB_ITModeCfg(GPIO_Pin_4, GPIO_ITMode_FallEdge); // 下降沿唤醒
  PFIC_EnableIRQ(GPIO_B_IRQn);
  PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

  printf("IDLE mode sleep \n");
  DelayMs(1);
  LowPower_Idle();
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
