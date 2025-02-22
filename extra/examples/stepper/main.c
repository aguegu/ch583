#include <stdio.h>
#include "CH58x_common.h"
#include "sys.h"
// #include "ringbuffer.h"
#include "at.h"
#include "uart1.h"
#include "ssd1306.h"

#define LED  GPIO_Pin_19

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
  uart1FlushTx();
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

void handleATSHOW(uint8_t * payload, uint8_t len) {
  payload[len] = 0;

  ssdPutString(payload, 0, 0);
  ssdRefresh();

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
  { "AT+SHOW=", FALSE, handleATSHOW },
  { NULL, TRUE, NULL}  // End marker
};

void taskAtCommands() {
  static uint8_t command[256];
  static uint8_t l = 0;
  static uint8_t content[128];
  BOOL LFrecevied = FALSE;

  while (uart1RxAvailable()) {
    uint8_t temp = uart1RxGet();
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
    uart1FlushTx();
  }
}

void taskBlink(void) {
  GPIOB_InverseBits(LED);
}

typedef struct {
  uint16_t raw;
  uint16_t startAt;
  int32_t corrected;
  int32_t rounds;
  uint8_t phaseLast;
  int32_t accumulated;
} AngleEncoder;

static AngleEncoder mt6701 = {
  .raw = 0,
  .startAt = 0,
  .corrected = 0,
  .rounds = 0,
  .phaseLast = 0,
  .accumulated = 0,
};

void taskReadAngle(void) {
  const uint8_t slaveAddress = 0x06;
  static uint8_t readings[2];

  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

  I2C_Send7bitAddress(slaveAddress << 1, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));  // 0x00070082

  I2C_SendData(0x03);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));  // 0x00070084

  I2C_GenerateSTART(ENABLE);  // restart
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

  I2C_Send7bitAddress(slaveAddress << 1, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // 0x00030002

  uint8_t readCount = 2;

  while (readCount--) {
    I2C_AcknowledgeConfig(readCount);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)); // 0x00030040
    readings[readCount] = I2C_ReceiveData();
  }

  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET); // 0x00000000

  mt6701.raw = (((uint16_t)readings[1] << 8) + readings[0]) >> 2;

  mt6701.corrected = mt6701.raw - mt6701.startAt;
  if (mt6701.corrected < 0) {
    mt6701.corrected += 16384;
  }

  uint8_t phase = mt6701.corrected >> 12;

  if (phase != mt6701.phaseLast) {
    if (phase == 0 && mt6701.phaseLast == 3) {
      mt6701.rounds++;
    } else if (phase == 3 && mt6701.phaseLast == 0) {
      mt6701.rounds--;
    }
    mt6701.phaseLast = phase;
  }
  mt6701.accumulated = mt6701.rounds * 16384 + mt6701.corrected;
}


uint16_t coins = 0;

void taskKeyboardPool() {
   static uint32_t btnsLast = 0;
   uint32_t btns = GPIOB_ReadPortPin(GPIO_Pin_4) ^ (GPIO_Pin_4);

   if (btns == btnsLast) {
     return;
   }

   if (btns & GPIO_Pin_4) {
     coins++;
   }

   btnsLast = btns;

}

void taskDisplay(void) {
  static char line0[16];
  static char line1[16];
  static char line2[16];
  static char line3[16];

  // 16384 / 360 = 45.5111111
  sprintf(line0, "raw: %5d, %3d", mt6701.raw, mt6701.raw * 100 / 4551);
  sprintf(line1, "crt: %5d, %3d", mt6701.corrected, mt6701.corrected * 100 / 4551);

  int32_t fa = mt6701.accumulated * 100 / 16384;
  if (fa == 0) {
    sprintf(line2, "rnd:       0.00");
  } else if (abs(fa) < 100) {
    sprintf(line2, "rnd:      %c0.%02d", fa > 0 ? '+' : '-', abs(fa) % 100);
  } else {
    sprintf(line2, "rnd: %+7d.%02d", fa / 100, abs(fa) % 100);
  }

  // int32_t fa2 = mt6701.accumulated * 10000 / 4551;
  // if (fa2 == 0) {
  //   sprintf(line3, "pos:       0.00");
  // } else if (abs(fa2) < 100) {
  //   sprintf(line3, "pos:      %c0.%02d", fa2 > 0 ? '+' : '-', abs(fa2) % 100);
  // } else {
  //   sprintf(line3, "pos: %+2d.%02d", fa2 / 100, abs(fa2) % 100);
  // }
  sprintf(line3, "%d", coins);

  ssdPutString(line0, 0, 0);
  ssdPutString(line1, 2, 0);
  ssdPutString(line2, 4, 0);
  ssdPutString(line3, 6, 0);
  ssdRefresh();
}



int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 1800); // 1800Hz

  GPIOB_ModeCfg(LED, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED);

  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);

  GPIOB_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_PU); // i2c SDA
  GPIOB_ModeCfg(GPIO_Pin_13, GPIO_ModeIN_PU); // i2c SCL

  I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, 0x00);
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001
  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET); // 0x00000000

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand

  uart1Init();

  ssdInit();

  taskReadAngle();
  mt6701.startAt = mt6701.raw;
  mt6701.corrected = 0;
  mt6701.phaseLast = 0;
  mt6701.rounds = 0;
  mt6701.accumulated = 0;

  registerTask(0, taskBlink, 180, 0);
  registerTask(1, taskAtCommands, 12, 1);
  registerTask(2, taskReadAngle, 15, 0);
  registerTask(3, taskDisplay, 30, 15);
  registerTask(4, taskKeyboardPool, 90, 0);

  dispatchTasks();
}
