#include <stdio.h>
#include "ringbuffer.h"
#include "at.h"
#include "gpio.h"
#include "ssd1306spi.h"

static Gpio led = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_19};
static Gpio uart1Tx = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_9};
static Gpio uart1Rx = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_8};

SSDspi ssd = {
    .sclk = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_13},
    .mosi = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_14},
    .rst = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_17},
    .dc = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_16},
    .cs = {.pin = 0},
};

RingBuffer txBuffer, rxBuffer;

volatile uint32_t jiffies = 0;

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
  while (!(R8_UART1_LSR & RB_LSR_TX_FIFO_EMP)) {
    __WFI();
  }
}

void handleAT(uint8_t *payload, uint8_t len) { sendOK(); }

void handleATMAC(uint8_t *payload, uint8_t len) {
  uint8_t mac[6];
  GetMACAddress(mac);
  for (uint8_t i = 6; i--;) {
    printf("%02X", mac[i]);
  }
  sendOK();
}

void handleATID(uint8_t *payload, uint8_t len) {
  uint8_t id[8];
  FLASH_EEPROM_CMD(CMD_GET_UNIQUE_ID, 0, id, 0);
  for (uint8_t i = 0; i < 8; i++) {
    printf("%02X", id[i]);
  }
  sendOK();
}

void handleATRESET(uint8_t *payload, uint8_t len) {
  sendOK();
  flushUart1Tx();
  SYS_ResetExecute();
}

void handleATECHO(uint8_t *payload, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    printf("%02X", payload[i]);
  }
  sendOK();
}

void handleATSHOW(uint8_t *payload, uint8_t len) {
  payload[len] = 0;
  ssdPutString(&ssd, payload, 0, 0);
  ssdRefresh(&ssd);
  sendOK();
}

const static CommandHandler atHandlers[] = {
    {"AT", TRUE, handleAT},
    {"AT+MAC", TRUE, handleATMAC},
    {"AT+ID", TRUE, handleATID},
    {"AT+RESET", TRUE, handleATRESET},
    {"AT+ECHO=", FALSE, handleATECHO},
    {"AT+SHOW=", FALSE, handleATSHOW},
    {NULL, TRUE, NULL} // End marker
};

BOOL athandler() {
  static uint8_t command[256];
  static uint8_t l = 0;
  static uint8_t content[128];
  BOOL LFrecevied = FALSE;

  while (ringbufferAvailable(&rxBuffer)) {
    uint8_t temp = ringbufferGet(&rxBuffer);
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
        if (atHandlers[i].isEqual &&
            strcmp(command, atHandlers[i].command) == 0) {
          atHandlers[i].handler(NULL, 0);
          handled = TRUE;
          break;
        }

        if (!atHandlers[i].isEqual &&
            startsWith(command, atHandlers[i].command)) {
          uint8_t len =
              genPayload(command + strlen(atHandlers[i].command), content);
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

  ringbufferInit(&txBuffer, 64);
  ringbufferInit(&rxBuffer, 128);

  gpioMode(&led, GPIO_ModeOut_PP_5mA);
  gpioSet(&led);

  gpioSet(&uart1Tx);
  gpioMode(&uart1Rx, GPIO_ModeIN_PU);
  gpioMode(&uart1Tx, GPIO_ModeOut_PP_5mA);

  UART1_DefInit();          // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);

  ssdInit(&ssd, 128, 32);
  ssdPutString(&ssd, "Hello, world.", 0, 0);
  ssdRefresh(&ssd);

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
    while (ringbufferAvailable(&txBuffer) && R8_UART1_TFC != UART_FIFO_SIZE) {
      R8_UART1_THR = ringbufferGet(&txBuffer);
    }
    break;
  case UART_II_RECV_RDY:  // Rx FIFO is full
  case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no
                          // new data comming in within timeout
    while (R8_UART1_RFC) {
      ringbufferPut(&rxBuffer, R8_UART1_RBR, FALSE);
    }
    break;
  }
}
