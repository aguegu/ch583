#include "uart1.h"

static RingBuffer txBuffer, rxBuffer;

void uart1Init() {
  ringbufferInit(&txBuffer, 64);
  ringbufferInit(&rxBuffer, 128);

  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);
}

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbufferPut(&txBuffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_FIFO_EMP) {
      while (ringbufferAvailable(&txBuffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbufferGet(&txBuffer);
      }
    }
  }
  return size;
}

void uart1FlushTx() {
  while (!(R8_UART1_LSR & RB_LSR_TX_FIFO_EMP)) {
    __WFI();
  }
}

inline BOOL uart1RxAvailable() {
  return ringbufferAvailable(&rxBuffer);
}

uint8_t uart1RxGet() {
  return ringbufferGet(&rxBuffer);
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbufferAvailable(&txBuffer) && R8_UART1_TFC != UART_FIFO_SIZE) {
        R8_UART1_THR = ringbufferGet(&txBuffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbufferPut(&rxBuffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}
