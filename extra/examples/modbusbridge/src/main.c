#include <stdio.h>
#include "CH58x_common.h"
#include "crc16.h"
#include "ringbuffer.h"
#include "at.h"
#include "gpio.h"

// UART0: PB4: RXD0; PB7: TXD0; PB5: DTR
// UART1: PA8: RXD1; PA9: TXD1
// UART2: PA6: RXD2; PA7: TXD2

#define __bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

const static Gpio led = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_18};
const static Gpio uart1Tx = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_9};
const static Gpio uart1Rx = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_8};

const static Gpio uart0Tx = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_7};  // MAX3485 DI
const static Gpio uart0Rx = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_4};  // MAX3485 RO
const static Gpio uart0Dtr = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_5}; // DE
// MAX3485 RE connected to GND

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
    ringbufferPut(&tx1Buffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_FIFO_EMP) {
      while (ringbufferAvailable(&tx1Buffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbufferGet(&tx1Buffer);
      }
    }
  }
  return size;
}

void flushUart0Tx() {
  while (!(R8_UART0_LSR & RB_LSR_TX_FIFO_EMP)) {
    __WFI();
    __nop();
    __nop();
  };
}

void flushUart1Tx() {
  // while (ringbufferAvailable(&tx1Buffer));
  while (!(R8_UART1_LSR & RB_LSR_TX_FIFO_EMP)) {
    __WFI();
    __nop();
    __nop();
  };
}

float toFloat(uint8_t *p) {
  float f;
  for (uint8_t i = 0; i < 4; i++) {
    ((uint8_t *)(&f))[3 - i] = p[i];
  }
  return f;
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

void modbusTransmit(uint8_t * tx, uint8_t len) {
  appendCrc16(tx, len);
  for (uint8_t i = 0; i < len + 2; i++) {
    ringbufferPut(&tx0Buffer, tx[i], TRUE);
    if (R8_UART0_LSR & RB_LSR_TX_FIFO_EMP) {
      while (ringbufferAvailable(&tx0Buffer) && R8_UART0_TFC < UART_FIFO_SIZE) {
        R8_UART0_THR = ringbufferGet(&tx0Buffer);
      }
    }
  }

  flushUart0Tx();
}

uint8_t modbusReceive(uint8_t * rx) {
  uint8_t len = 0, * p = rx;
  lastReceivedAt = jiffies;
  while (jiffies < (uint32_t)(lastReceivedAt + 4)) {
    __WFI();
    __nop();
    __nop();
  }

  while (ringbufferAvailable(&rx0Buffer)) {
    *p ++ = ringbufferGet(&rx0Buffer);
    len ++;
  }
  return len;
}

void handleATFWD(uint8_t * payload, uint8_t len) {
  static uint8_t rx[64], rxLen;
  modbusTransmit(payload, len);
  rxLen = modbusReceive(rx);

  for (uint8_t i = 0; i < rxLen; i++) {
    printf("%02X", rx[i]);
  }

  sendOK();
}

void handleATALL(uint8_t * payload, uint8_t len) {
  const static uint16_t addresses[8] = {0x2000, 0x2002, 0x2004, 0x2006, 0x2008, 0x200A, 0x200E, 0x4000};
  static uint8_t tx[8] = { 0x01, 0x04, 0x00, 0x00, 0x00, 0x02 };
  static uint8_t rx[64], rxLen;
  // modbusTransmit((uint8_t[]){0x01, 0x04, 0x20, 0x00, 0x00, 0x02}, 6);
  //
  // rxLen = modbusReceive(rx);
  for (uint8_t k = 0; k < 8; k++) {
    *(uint16_t *)(tx + 2) = __bswap_16(addresses[k]);
    modbusTransmit(tx, 6);
    rxLen = modbusReceive(rx);
    printf("%d, ", (int)toFloat(rx + 3));
  }

  sendOK();
}

void handleATRE(uint8_t * payload, uint8_t len) {
  uint8_t rx[64];
  EEPROM_READ(0, rx, 64);
  for (uint8_t i = 0; i < 64; i++) {
    printf("%02X", rx[i]);
  }
  sendOK();
}

void handleATWE(uint8_t * payload, uint8_t len) {
  EEPROM_WRITE(0, payload, len);
  sendOK();
}

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },
  { "AT+FWD=", FALSE, handleATFWD},
  { "AT+ALL", TRUE, handleATALL},
  { "AT+RE", TRUE, handleATRE },
  { "AT+WE=", FALSE, handleATWE },
  { NULL, TRUE, NULL}  // End marker
};

BOOL athandler() {
  static uint8_t command[256];
  static uint8_t l = 0;
  static uint8_t content[128];
  BOOL LFrecevied = FALSE;

  while (ringbufferAvailable(&rx1Buffer)) {
    uint8_t temp = ringbufferGet(&rx1Buffer);
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

  ringbufferInit(&tx0Buffer, 64);
  ringbufferInit(&rx0Buffer, 128);

  ringbufferInit(&tx1Buffer, 64);
  ringbufferInit(&rx1Buffer, 128);

  gpioMode(&led, GPIO_ModeOut_PP_5mA);
  gpioSet(&led);

  gpioSet(&uart0Tx);
  gpioMode(&uart0Rx, GPIO_ModeIN_PU);
  gpioMode(&uart0Tx, GPIO_ModeOut_PP_5mA);
  gpioMode(&uart0Dtr, GPIO_ModeOut_PP_5mA);

  UART0_BaudRateCfg(9600);

  R8_UART0_MCR = RB_MCR_HALF | RB_MCR_TNOW;
  R8_UART0_FCR = (3 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
  R8_UART0_LCR = RB_LCR_WORD_SZ;  // dataBit: 8, parity: none, stopbit: 1
  R8_UART0_DIV = 1;

  R8_UART0_IER = RB_IER_TXD_EN | RB_IER_DTR_EN;
  UART0_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART0_IRQn);

  gpioSet(&uart1Tx);
  gpioMode(&uart1Rx, GPIO_ModeIN_PU);
  gpioMode(&uart1Tx, GPIO_ModeOut_PP_5mA);
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
      while (ringbufferAvailable(&tx0Buffer) && R8_UART0_TFC < UART_FIFO_SIZE) {
        R8_UART0_THR = ringbufferGet(&tx0Buffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART0_RFC) {
        ringbufferPut(&rx0Buffer, R8_UART0_RBR, FALSE);
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
      while (ringbufferAvailable(&tx1Buffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbufferGet(&tx1Buffer);
      }
      break;
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbufferPut(&rx1Buffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}

// 01 04 20 00 00 02 7a 0b
// 01 04 04 43 65 e6 66 34 55
