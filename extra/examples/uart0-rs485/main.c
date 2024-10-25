#include <stdio.h>
#include "CH58x_common.h"

// UART0: PB4: RXD0; RB7: TXD0; PB5: DTR
// UART1: PA8: RXD1; PA9: TXD1
// UART2: PA6: RXD2; PA7: TXD2

#define LED  GPIO_Pin_19

#define RING_BUFFER_SIZE (16)
#define RING_BUFFER_MASK (0x0f)

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

typedef struct {
  uint8_t buffer[RING_BUFFER_SIZE];
  uint8_t indexIn;
  uint8_t indexOut;
} RingBuffer;

void ringbuffer_init(RingBuffer * rb) {
  rb->indexIn = 0;
  rb->indexOut = 0;
}

void ringbuffer_put(RingBuffer * rb, uint8_t c) {
  uint8_t indexInNext = (rb->indexIn + 1) & RING_BUFFER_MASK;
  while (indexInNext == rb->indexOut) {
    __WFI();
  };
  rb->buffer[rb->indexIn] = c;
  rb->indexIn = indexInNext;
}

uint8_t ringbuffer_get(RingBuffer * rb) {
  uint8_t indexOutNext = (rb->indexOut + 1) & RING_BUFFER_MASK;
  uint8_t c = rb->buffer[rb->indexOut];
  rb->indexOut = indexOutNext;
  return c;
}

BOOL ringbuffer_available(RingBuffer *rb) {
  return rb->indexIn != rb->indexOut;
}

RingBuffer txBuffer;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbuffer_put(&txBuffer, *buf++);
    if (R8_UART0_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART0_THR = ringbuffer_get(&txBuffer);
    }
  }
  return size;
}

void flushUart0Tx(RingBuffer *rb) {
  while (!(R8_UART0_LSR & RB_LSR_TX_ALL_EMP));
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOB_SetBits(GPIO_Pin_7);
  GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA); // TXD: PB7, pushpull, but set it high beforehand
  GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA); // DTR: PB5, pushpull
  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);      // RXD: RB4, in with pullup

  ringbuffer_init(&txBuffer);

  UART0_BaudRateCfg(9600);

  R8_UART0_MCR = RB_MCR_HALF | RB_MCR_TNOW;
  R8_UART0_FCR = (3 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
  R8_UART0_LCR = RB_LCR_WORD_SZ;  // dataBit: 8, parity: none, stopbit: 1
  R8_UART0_DIV = 1;

  R8_UART0_IER = RB_IER_TXD_EN | RB_IER_DTR_EN;
  UART0_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);

  PFIC_EnableIRQ(UART0_IRQn);

  while(1) {
    printf("ChipID: %02x\t SysClock: %ldHz\n", R8_CHIP_ID, GetSysClock());
    printf("%ld\n", jiffies);
    flushUart0Tx(&txBuffer);
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
    // case UART_II_RECV_RDY: // Rx FIFO is full
    // case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
    //   while (R8_UART0_RFC) {
    //     rxBufferInCache = R8_UART0_RBR;
    //     rxBufferIndexInNext = (rxBuffer.indexIn + 1) & RING_BUFFER_MASK;
    //
    //     if (rxBufferIndexInNext == rxBuffer.indexOut) {
    //       rxBuffer.indexOut++;
    //       rxBuffer.indexOut &= RING_BUFFER_MASK;
    //     }
    //
    //     if (rxBufferInCache == '\n') {
    //       rxBuffer.isPackageCompeted = TRUE;
    //     }
    //     rxBuffer.buffer[rxBuffer.indexIn] = rxBufferInCache;
    //     rxBuffer.indexIn = rxBufferIndexInNext;
    //   }
    //   break;
  }
}
