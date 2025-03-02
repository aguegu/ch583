#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"
#include "at.h"

#define LED  GPIO_Pin_19
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

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },
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

volatile uint32_t stepCount = 0;

__attribute__((aligned(4))) uint32_t bufferPWMZero = { 0 };

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  SysTick_Config(GetSysClock() / 6000); // 60Hz
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

  GPIOPinRemap(ENABLE, RB_PIN_TMR1);
  GPIOB_ResetBits(GPIO_Pin_10);
  GPIOB_ModeCfg(GPIO_Pin_10, GPIO_ModeOut_PP_5mA);

  // __attribute__((aligned(4))) uint32_t bufferPWM[8] = { 15, 30, 45, 60, 75, 90, 105, 120 };

  // TMR1_PWMInit(High_Level, PWM_Times_1);
  // TMR1_PWMCycleCfg(FREQ_SYS / 400000);   // 400kHz(150)

  // TMR2_DMACfg(ENABLE, (uint16_t)(uint32_t)&PwmBuf[0], (uint16_t)(uint32_t)&PwmBuf[100], Mode_LOOP);

  // TMR1_ITCfg(ENABLE, TMR0_3_IT_DATA_ACT);
  // PFIC_EnableIRQ(TMR1_IRQn);

  // TMR1_Enable();
  // TMR1_DMACfg(ENABLE, (uint16_t)(uint32_t)bufferPWM, (uint16_t)(uint32_t)(bufferPWM+8), Mode_LOOP);

  uint8_t k = 1;

  TMR1_PWMInit(High_Level, PWM_Times_1);
  TMR1_PWMCycleCfg(FREQ_SYS / 1000000);   // 1MHz(60)
  TMR1_PWMActDataWidth(30);
  TMR1_PWMEnable();

  TMR1_Enable();


  while (1) {
    if (!athandler()) {
      __WFI();
      __nop();
      __nop();
    }

    // TMR1_Disable();





    k++;
    if (k == 4) {
      k = 1;
    }

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

__INTERRUPT
__HIGH_CODE
void TMR1_IRQHandler(void) {
  if (TMR1_GetITFlag(TMR0_3_IT_DATA_ACT)) {
    TMR1_ClearITFlag(TMR0_3_IT_DATA_ACT);
    // stepCount++;
    // if (stepCount == 1) {
      // TMR1_PWMActDataWidth(0);
      // TMR1_PWMDisable();
      TMR1_Disable();
      TMR1_PWMCycleCfg(FREQ_SYS / 400000 * (1 + (stepCount & 1)));
      TMR1_Enable();
      // // TMR1_Disable();
      // TMR1_PWMEnable();
    // }
  }
}
