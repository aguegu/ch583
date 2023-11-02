#include <stdio.h>
#include "CH58x_common.h"

volatile uint32_t jiffies = 0;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    while (R8_UART1_TFC == UART_FIFO_SIZE);
    R8_UART1_THR = *buf++;
  }
  return size;
}

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200

  while(1) {
    printf("ChipID: %02x\t SysClock: %ldHz\n", R8_CHIP_ID, GetSysClock());
    printf("%ld\n", jiffies);
    delayInJiffy(60);
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}
