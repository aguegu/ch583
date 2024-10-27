#include <stdio.h>
#include "CH58x_common.h"
#include "crc16.h"
#include "ringbuffer.h"

// UART0: PB4: RXD0; RB7: TXD0; PB5: DTR
// UART1: PA8: RXD1; PA9: TXD1
// UART2: PA6: RXD2; PA7: TXD2

#define __bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define LED  GPIO_Pin_19

volatile uint32_t jiffies = 0;

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

RingBuffer txBuffer, rxBuffer;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbuffer_put(&txBuffer, *buf++, TRUE);
    if (R8_UART0_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART0_THR = ringbuffer_get(&txBuffer);
      GPIOB_ResetBits(LED);
    }
  }
  return size;
}

void flushUart0Tx() {
  // while (ringbuffer_available(&txBuffer));
  while (!(R8_UART0_LSR & RB_LSR_TX_ALL_EMP));
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOB_ResetBits(GPIO_Pin_3);
  GPIOB_ModeCfg(GPIO_Pin_3, GPIO_ModeOut_PP_5mA);

  GPIOB_SetBits(GPIO_Pin_7);
  GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA); // TXD: PB7, pushpull, but set it high beforehand
  GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA); // DTR: PB5, pushpull
  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);      // RXD: RB4, in with pullup

  ringbuffer_init(&txBuffer, 64);
  ringbuffer_init(&rxBuffer, 128);

  UART0_BaudRateCfg(9600);

  R8_UART0_MCR = RB_MCR_HALF | RB_MCR_TNOW;
  R8_UART0_FCR = (3 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
  R8_UART0_LCR = RB_LCR_WORD_SZ;  // dataBit: 8, parity: none, stopbit: 1
  R8_UART0_DIV = 1;

  R8_UART0_IER = RB_IER_TXD_EN | RB_IER_DTR_EN;
  UART0_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);

  PFIC_EnableIRQ(UART0_IRQn);

  uint8_t command[128];

  while(1) {
    command[0] = 0x01;
    command[1] = 0x04;
    *(uint16_t *)(command + 2) = __bswap_16(0x2000);
    *(uint16_t *)(command + 4) = __bswap_16(0x0002);

    appendCrc16(command, 6);
    // uint8_t l = 0;
    //
    // while (ringbuffer_available(&rxBuffer)) {
    //   command[l++] = ringbuffer_get(&rxBuffer);
    // }
    //
    // if (l) {
    //   for (uint8_t i = 0; i < l; i++) {
    //     printf("%02x ", command[i]);
    //   }
    //
    //   printf("\n");
    //   flushUart0Tx();
    // }

    // printf("ChipID: %02x\t SysClock: %ldHz\n", R8_CHIP_ID, GetSysClock());
    // printf("%ld\n", jiffies);
    // flushUart0Tx();

    GPIOB_SetBits(GPIO_Pin_3);

    for (uint8_t i = 0; i < 8; i++) {
      ringbuffer_put(&txBuffer, command[i], TRUE);
      if (R8_UART0_LSR & RB_LSR_TX_ALL_EMP) {
        R8_UART0_THR = ringbuffer_get(&txBuffer);
        GPIOB_ResetBits(LED);
      }
    }

    // printf("\n");
    flushUart0Tx();
    GPIOB_ResetBits(GPIO_Pin_3);
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
void UART0_IRQHandler(void) {
  switch (UART0_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbuffer_available(&txBuffer) && R8_UART0_TFC < UART_FIFO_SIZE) {
        R8_UART0_THR = ringbuffer_get(&txBuffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART0_RFC) {
        ringbuffer_put(&rxBuffer, R8_UART0_RBR, FALSE);
      }
      break;
  }
}

01 04 20 00 00 02 7a 0b
01 04 04 43 65 e6 66 34 55


<Buffer 01 04 20 00 00 02 7a 0b>
<Buffer 01 04 20 00 00 02 7a 0b>
<Buffer 01 04 04 43 65 19 9a 75 e4>
<Buffer 01 04 04 43 65 19 9a 75 e4>
