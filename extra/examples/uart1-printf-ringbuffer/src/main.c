#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"

#define LED  GPIO_Pin_19

volatile uint32_t jiffies = 0;
RingBuffer txBuffer;

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbufferPut(&txBuffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART1_THR = ringbufferGet(&txBuffer);
      GPIOB_ResetBits(LED);
    }
  }
  return size;
}

void flushUart1Tx() {
  // while (ringbufferAvailable(&txBuffer));
  while (!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  ringbufferInit(&txBuffer, 64);

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY);
  PFIC_EnableIRQ(UART1_IRQn);

  while(1) {
    fprintf(stdout, "ChipID: %02x\t SysClock: %ldHz\t", R8_CHIP_ID, GetSysClock());
    fprintf(stdout, "%ld\t", jiffies);
    fflush(stdout); // without fflush, _write may never by called, because of no '\n'
    flushUart1Tx();
    GPIOB_SetBits(LED);

    delayInJiffy(60);
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbufferAvailable(&txBuffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbufferGet(&txBuffer);
      }
      break;
  }
}
