#include <stdio.h>
#include "CH58x_common.h"

#define RING_BUFFER_SIZE (256)
#define RING_BUFFER_MASK (0xff)

typedef struct {
  uint8_t buffer[RING_BUFFER_SIZE];
  volatile uint8_t indexIn;
  volatile uint8_t indexOut;
  volatile BOOLEAN isPackageCompeted;
} RingBuffer;

RingBuffer txBuffer, rxBuffer;
volatile uint8_t rxBufferIndexInNext;

volatile uint32_t jiffies = 0;
uint32_t iiTransmitEmptyCount = 0;
volatile uint8_t rxBufferInCache = 0;

uint8_t command[RING_BUFFER_SIZE];
uint8_t resp[RING_BUFFER_SIZE];

int _write(int fd, char *buf, int size) {
  for (int j = 0; j < size; j++) {
    uint8_t i = (txBuffer.indexIn + 1) & RING_BUFFER_MASK;
    while (i == txBuffer.indexOut) {
      GPIOB_InverseBits(GPIO_Pin_18 | GPIO_Pin_19);
      __WFI();
      __nop();
    };

    txBuffer.buffer[txBuffer.indexIn] = *buf++;
    txBuffer.indexIn = i;

    if (txBuffer.isPackageCompeted) {
      txBuffer.isPackageCompeted = FALSE;
      R8_UART1_THR = txBuffer.buffer[txBuffer.indexOut];
      txBuffer.indexOut++;
      txBuffer.indexOut &= RING_BUFFER_MASK;
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
    }
  }
}

uint8_t getNth(RingBuffer * rb, uint8_t nth) {
  return rb->buffer[(rb->indexOut + nth) & RING_BUFFER_MASK];
}

void athandler() {
  if (rxBuffer.isPackageCompeted) {
    rxBuffer.isPackageCompeted = FALSE;
    uint8_t i = 0, c;
    do {
      c = getNth(&rxBuffer, i);
      command[i] = c >= 'a' && c <= 'z' ? c - 0x20 : c;
      i++;
    } while (i < 2 || c != '\n');
    rxBuffer.indexOut += i;
    rxBuffer.indexOut &= RING_BUFFER_MASK;

    if (command[i - 2] != '\r') {
      return;
    }
    command[i - 2] = 0;

    if (strcmp(command, "AT") == 0) {
      printf("\r\nOK\r\n");
    } else if (strcmp(command, "AT+ID") == 0) {
      printf("%02X", R8_CHIP_ID);
      GET_UNIQUE_ID(resp);
      GetMACAddress(resp + 8);
      for (uint i = 0; i < 14; i++) {
        printf("%02X", resp[i]);
      }
      printf("\r\nOK\r\n");
    } else {
ERROR:printf("\r\nERROR\r\n");
    }
  }
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  rxBuffer.indexIn = 0;
  rxBuffer.indexOut = 0;
  rxBuffer.isPackageCompeted = FALSE;

  txBuffer.indexIn = 0;
  txBuffer.indexOut = 0;
  txBuffer.isPackageCompeted = TRUE;

  GPIOB_ModeCfg(GPIO_Pin_18, GPIO_ModeOut_PP_5mA);  // Led1
  GPIOB_ModeCfg(GPIO_Pin_19, GPIO_ModeOut_PP_5mA);  // Led2
  GPIOB_InverseBits(GPIO_Pin_18);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand

  R8_UART1_FCR = 0xc7;
  R8_UART1_LCR = 0x03;
  R8_UART1_IER = 0x40;
  R8_UART1_DIV = 1;

  R16_UART1_DL = GetSysClock() / 8 / 9600;

  R8_UART1_IER = 0x43;
  R8_UART1_MCR = 0x08;
  PFIC_EnableIRQ(UART1_IRQn);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  while(1) {
    athandler();
    delayInJiffy(1);
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
      while (txBuffer.indexOut != txBuffer.indexIn && R8_UART1_TFC < 8) {
        R8_UART1_THR = txBuffer.buffer[txBuffer.indexOut];
        txBuffer.indexOut++;
        txBuffer.indexOut &= RING_BUFFER_MASK;
      }
      iiTransmitEmptyCount = R8_UART1_TFC;
      if (txBuffer.indexOut == txBuffer.indexIn) {
        txBuffer.isPackageCompeted = TRUE;
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        rxBufferInCache = R8_UART1_RBR;
        rxBufferIndexInNext = (rxBuffer.indexIn + 1) & RING_BUFFER_MASK;

        if (rxBufferIndexInNext == rxBuffer.indexOut) {
          rxBuffer.indexOut++;
          rxBuffer.indexOut &= RING_BUFFER_MASK;
        }

        if (rxBufferInCache == '\n') {
          rxBuffer.isPackageCompeted = TRUE;
        }
        rxBuffer.buffer[rxBuffer.indexIn] = rxBufferInCache;
        rxBuffer.indexIn = rxBufferIndexInNext;
      }
      break;
  }
}
