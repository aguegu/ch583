#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"

#define LED  GPIO_Pin_19
RingBuffer txBuffer, rxBuffer;

volatile uint32_t jiffies = 0;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbuffer_put(&txBuffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART1_THR = ringbuffer_get(&txBuffer);
      // GPIOB_ResetBits(LED);
    }
  }
  return size;
}

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    } else {
      __WFI();
      __nop();
      __nop();
    }
  }
}

void flushUart1Tx() {
  // while (ringbuffer_available(&txBuffer));
  while (!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
}

BOOL athandler() {
  static uint8_t command[256];
  static uint8_t l = 0;
  BOOL LFrecevied = FALSE;

  while (ringbuffer_available(&rxBuffer)) {
    uint8_t temp = ringbuffer_get(&rxBuffer);
    if (temp == '\n') {
      LFrecevied = TRUE;
      break;
    } else {
      command[l++] = temp;
    }
  }

  if (LFrecevied) {
    for (uint8_t i = 0; i < l; i++) {
      putchar(command[i]);
    }
    l = 0;
    putchar('\n');
    flushUart1Tx();
  }
  return LFrecevied;
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  ringbuffer_init(&txBuffer, 64);
  ringbuffer_init(&rxBuffer, 128);

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);

  uint8_t command[256];
  uint8_t l = 0;
  BOOL LFrecevied = FALSE;

  while (1) {
    if (!athandler()) {
      __WFI();
      __nop();
      __nop();
    }
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  SysTick->SR = 0;
  jiffies++;
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbuffer_available(&txBuffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbuffer_get(&txBuffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbuffer_put(&rxBuffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}
