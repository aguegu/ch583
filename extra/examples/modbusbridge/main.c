#include <stdio.h>
#include "CH58x_common.h"
#include "crc16.h"
#include "ringbuffer.h"
#include "at.h"

// UART0: PB4: RXD0; RB7: TXD0; PB5: DTR
// UART1: PA8: RXD1; PA9: TXD1
// UART2: PA6: RXD2; PA7: TXD2

#define __bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define LED  GPIO_Pin_19

volatile uint32_t jiffies = 0;
volatile uint32_t lastReceivedAt = 0;

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

RingBuffer tx0Buffer, rx0Buffer;
RingBuffer tx1Buffer, rx1Buffer;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbuffer_put(&tx1Buffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART1_THR = ringbuffer_get(&tx1Buffer);
    }
  }
  return size;
}

void flushUart0Tx() {
  // while (ringbuffer_available(&tx0Buffer));
  while (!(R8_UART0_LSR & RB_LSR_TX_ALL_EMP));
}

void flushUart1Tx() {
  while (!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
}

void handleAT(uint8_t * payload, uint8_t len) {
  sendOK();
}

void handleATMAC(uint8_t * payload, uint8_t len) {
  uint8_t mac[6];
  GetMACAddress(mac);
  for (uint8_t i = 6; i--;) {
    printf("%02X", mac[i]);
  }
  sendOK();
}

void handleATID(uint8_t * payload, uint8_t len) {
  uint8_t id[8];
  FLASH_EEPROM_CMD(CMD_GET_UNIQUE_ID, 0, id, 0);
  for (uint8_t i = 0; i < 8; i++) {
    printf("%02X", id[i]);
  }
  sendOK();
}

void handleATRESET(uint8_t * payload, uint8_t len) {
  sendOK();
  flushUart1Tx();
  SYS_ResetExecute();
}

void handleATECHO(uint8_t * payload, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    printf("%02X", payload[i]);
  }
  sendOK();
}

void handleATFWD(uint8_t * payload, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    ringbuffer_put(&tx0Buffer, payload[i], TRUE);
    if (R8_UART0_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART0_THR = ringbuffer_get(&tx0Buffer);
      GPIOB_ResetBits(LED);
    }
  }

  flushUart0Tx();
  lastReceivedAt = jiffies;
  GPIOB_SetBits(LED);

  while (jiffies != (uint32_t)(lastReceivedAt + 2)) {
    __WFI();
    __nop();
    __nop();
  }

  while (ringbuffer_available(&rx0Buffer)) {
    printf("%02X", ringbuffer_get(&rx0Buffer));
  }

  sendOK();
}

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },
  { "AT+FWD=", FALSE, handleATFWD},
  { NULL, TRUE, NULL}  // End marker
};

BOOL athandler() {
  static uint8_t command[256];
  static uint8_t l = 0;
  static uint8_t content[128];
  BOOL LFrecevied = FALSE;

  while (ringbuffer_available(&rx1Buffer)) {
    uint8_t temp = ringbuffer_get(&rx1Buffer);
    if (temp == '\n') {
      LFrecevied = TRUE;
      break;
    } else {
      command[l++] = temp;
    }
  }

  if (LFrecevied) {
    BOOL handled = FALSE;
    if (command[l - 1] == '\r') {
      command[l - 1] = 0;

      for (uint8_t i = 0; atHandlers[i].command != NULL; i++) {
        if (atHandlers[i].isEqual && strcmp(command, atHandlers[i].command) == 0) {
          atHandlers[i].handler(NULL, 0);
          handled = TRUE;
          break;
        }

        if (!atHandlers[i].isEqual && startsWith(command, atHandlers[i].command)) {
          uint8_t len = genPayload(command + strlen(atHandlers[i].command), content);
          atHandlers[i].handler(content, len);
          handled = TRUE;
          break;
        }
      }
    }

    if (!handled) {
      sendError();
    }

    l = 0;
    flushUart1Tx();
  }
  return LFrecevied;
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  ringbuffer_init(&tx0Buffer, 64);
  ringbuffer_init(&rx0Buffer, 128);

  ringbuffer_init(&tx1Buffer, 64);
  ringbuffer_init(&rx1Buffer, 128);

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOB_ResetBits(GPIO_Pin_3);
  GPIOB_ModeCfg(GPIO_Pin_3, GPIO_ModeOut_PP_5mA);

  GPIOB_SetBits(GPIO_Pin_7);
  GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA); // TXD: PB7, pushpull, but set it high beforehand
  GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA); // DTR: PB5, pushpull
  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);      // RXD: RB4, in with pullup

  UART0_BaudRateCfg(9600);

  R8_UART0_MCR = RB_MCR_HALF | RB_MCR_TNOW;
  R8_UART0_FCR = (3 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
  R8_UART0_LCR = RB_LCR_WORD_SZ;  // dataBit: 8, parity: none, stopbit: 1
  R8_UART0_DIV = 1;

  R8_UART0_IER = RB_IER_TXD_EN | RB_IER_DTR_EN;
  UART0_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART0_IRQn);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);

  while(1) {
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
  jiffies++;
  SysTick->SR = 0;
}

__INTERRUPT
__HIGH_CODE
void UART0_IRQHandler(void) {
  switch (UART0_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbuffer_available(&tx0Buffer) && R8_UART0_TFC < UART_FIFO_SIZE) {
        R8_UART0_THR = ringbuffer_get(&tx0Buffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART0_RFC) {
        ringbuffer_put(&rx0Buffer, R8_UART0_RBR, FALSE);
        lastReceivedAt = jiffies;
      }
      break;
  }
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbuffer_available(&tx1Buffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbuffer_get(&tx1Buffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbuffer_put(&rx1Buffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}

// 01 04 20 00 00 02 7a 0b
// 01 04 04 43 65 e6 66 34 55
