#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"
#include "at.h"

#define LED  GPIO_Pin_19
RingBuffer txBuffer, rxBuffer;

// PMW6: PB0
// PWM8: PB6

// Motor A: PB0/PB1
// Motor B: PB6/PB7

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

// void handleATPWM6(uint8_t * payload, uint8_t len) {
//   if (len == 1) {
//     PWM6_ActDataWidth(payload[0]);
//     sendOK();
//   } else {
//     sendError();
//   }
// }
//
// void handleATPWM8(uint8_t * payload, uint8_t len) {
//   if (len == 1) {
//     PWM8_ActDataWidth(payload[0]);
//     sendOK();
//   } else {
//     sendError();
//   }
// }

typedef struct {
  uint8_t pwmChannel;
  uint8_t pinNegative;
  uint8_t pwmChannelIndex;
} MOTOR;

#define MOTOR_LEN 2

const MOTOR motors[MOTOR_LEN] = {
  { .pwmChannel = CH_PWM6, .pwmChannelIndex = 2, .pinNegative = GPIO_Pin_1 },
  { .pwmChannel = CH_PWM8, .pwmChannelIndex = 4, .pinNegative = GPIO_Pin_7 },
};

void handleATMOTOR(uint8_t *payload, uint8_t len) {
  if (len != MOTOR_LEN) {
    sendError();
  }

  uint8_t index = payload[0] >> 4;

  if (index > MOTOR_LEN) {
    sendError();
  }

  // X000: idle;
  // X0ff / X100: forwards fullspeed;
  // X3ff / X200: backwards fullspeed;
  // X1ff: brake

  *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[index].pwmChannelIndex)) = payload[1];

  if (payload[0] & 0x01) {
    R8_PWM_POLAR |= motors[index].pwmChannel;
  } else {
    R8_PWM_POLAR &= ~motors[index].pwmChannel;
  }

  if (payload[0] & 0x02) {
    GPIOB_SetBits(motors[index].pinNegative);
  } else {
    GPIOB_ResetBits(motors[index].pinNegative);
  }

  sendOK();
}

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },
  // { "AT+PWM6=", FALSE, handleATPWM6 },
  // { "AT+PWM8=", FALSE, handleATPWM8 },
  { "AT+MOTOR=", FALSE, handleATMOTOR },
  { NULL, TRUE, NULL}  // End marker
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

  ringbufferInit(&txBuffer, 64);
  ringbufferInit(&rxBuffer, 128);

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);

  GPIOB_ModeCfg(GPIO_Pin_0, GPIO_ModeOut_PP_5mA);  // PB0 - PWM6
  GPIOB_ModeCfg(GPIO_Pin_6, GPIO_ModeOut_PP_5mA);  // PB6 - PWM8

  PWMX_CLKCfg(240);            // freq = 60M / 240 / 255 = 980.40 hz
  PWMX_CycleCfg(PWMX_Cycle_255);

  for (uint8_t i = 0; i < MOTOR_LEN; i++) {
    PWMX_ACTOUT(motors[i].pwmChannel, 0, High_Level, ENABLE);
    GPIOB_ResetBits(motors[i].pinNegative);
    GPIOB_ModeCfg(motors[i].pinNegative, GPIO_ModeOut_PP_5mA);
  }

  while (1) {
    if (!athandler()) {
      __WFI();
      __nop();
      __nop();
    }
  }

  return 0;
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
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbufferPut(&rxBuffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}
