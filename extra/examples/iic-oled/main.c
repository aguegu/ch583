#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"
#include "at.h"

#define LED  GPIO_Pin_19
RingBuffer txBuffer, rxBuffer;

static volatile uint32_t jiffies = 0;

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

void handleATSC(uint8_t * payload, uint8_t len) {
  for (uint8_t slaveAddress = 0x03; slaveAddress < 0x78; slaveAddress++) {
    while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(slaveAddress << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && !I2C_GetFlagStatus(I2C_FLAG_AF));

    if (I2C_GetFlagStatus(I2C_FLAG_AF)) {
      I2C_ClearFlag(I2C_FLAG_AF);
    }

    BOOL acked = I2C_GetFlagStatus(I2C_FLAG_TXE);
    I2C_GenerateSTOP(ENABLE);

    if (acked) {
      printf("%02X", slaveAddress);
    }
  }

  sendOK();
}

void handleATTR(uint8_t * payload, uint8_t len) {
  if (len < 2) {
    return sendError();
  }

  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

  I2C_Send7bitAddress(payload[0] << 1, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && !I2C_GetFlagStatus(I2C_FLAG_AF));  // 0x00070082

  if (I2C_GetFlagStatus(I2C_FLAG_AF)) {
    I2C_ClearFlag(I2C_FLAG_AF);
    I2C_GenerateSTOP(ENABLE);
    return sendError();
  }

  if (len > 2) {
    uint8_t writeCount = len - 2;
    uint8_t *toWrite = payload + 1;
    while (writeCount--) {
      I2C_SendData(*toWrite++);
      while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));  // 0x00070084
    }
  }

  uint8_t readCount = payload[len - 1];

  if (readCount) {
    I2C_GenerateSTART(ENABLE);  // restart
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

    I2C_Send7bitAddress(payload[0] << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // 0x00030002

    while (readCount--) {
      I2C_AcknowledgeConfig(readCount);
      while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)); // 0x00030040
      printf("%02X", I2C_ReceiveData());
    }
  }

  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET); // 0x00000000

  sendOK();
}

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },
  { "AT+SC", TRUE, handleATSC },
  { "AT+TR=", FALSE, handleATTR },
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

  GPIOB_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_PU); // i2c SDA
  GPIOB_ModeCfg(GPIO_Pin_13, GPIO_ModeIN_PU); // i2c SCL

  I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, 0x00);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY | RB_IER_RECV_RDY);
  PFIC_EnableIRQ(UART1_IRQn);

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
    case UART_II_RECV_RDY: // Rx FIFO is full
    case UART_II_RECV_TOUT: // Rx FIFO is not full, but there is something when no new data comming in within timeout
      while (R8_UART1_RFC) {
        ringbufferPut(&rxBuffer, R8_UART1_RBR, FALSE);
      }
      break;
  }
}
