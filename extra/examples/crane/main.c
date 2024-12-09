#include <stdio.h>
#include "CH58x_common.h"
#include "ringbuffer.h"
#include "at.h"

// UART0: PB4: RXD0; PB7: TXD0; PB5: DTR
// UART1: PA8: RXD1; PA9: TXD1
// UART2: PA6: RXD2; PA7: TXD2

// Keyboard:
// Up: PB17, Down: PB9, Left: PB16, Right: PB8, Mid: PA3
// Set: PA1, Rst: PA0

#define KEYBOARD_RIGHT GPIO_Pin_8
#define KEYBOARD_DOWN GPIO_Pin_9
#define KEYBOARD_LEFT GPIO_Pin_16
#define KEYBOARD_UP GPIO_Pin_17

#define KEYBOARD_MID GPIO_Pin_3
#define KEYBOARD_SET GPIO_Pin_1
#define KEYBOARD_RST GPIO_Pin_0

// #define ONBOARD_KEY GPIO_Pin_4
#define ONBOARD_DOWNLOAD GPIO_Pin_22

#define CLAW  GPIO_Pin_2

#define LED1 GPIO_Pin_18
#define LED2 GPIO_Pin_19

volatile uint32_t jiffies = 0;

typedef void (*TaskFunction)(void);

typedef struct {
  TaskFunction task;
  uint32_t period;
  uint32_t delay;
  BOOL ready;
} Task;

#define TASK_LEN (3)

static Task taskList[TASK_LEN];

#define BYTE0(a) ((a) & 0xFF)
#define BYTE1(a) (((a) >> 8) & 0xFF)
#define BYTE2(a) (((a) >> 16) & 0xFF)
#define BYTE3(a) (((a) >> 24) & 0xFF)

// #define __bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define ABS(n)  (((n) < 0) ? -(n) : (n))

typedef struct {
  uint16_t version;
  uint8_t xId;
  int16_t xStart;
  int16_t xEnd;
  uint8_t yId;
  int16_t yStart;
  int16_t yEnd;
  uint8_t zId;
  int16_t zStart;
  int16_t zEnd;
  uint16_t xSpeed;
  uint16_t ySpeed;
  uint16_t zSpeed;
} MotorSetting;

static MotorSetting setting = {
  .version = 0x01,
  .xId = 1,
  .xStart = 0,
  .xEnd = -30000,
  .yId = 2,
  .yStart = 0,
  .yEnd = 16000,
  .zId = 3,
  .zStart = 0,
  .zEnd = -32000,
  .xSpeed = 600,
  .ySpeed = 600,
  .zSpeed = 600,
};

// when y at yEnd, z = -32000 reach top

// y = yStart, z = -32000 | y = yEnd, z = - 240000
// y = yStart, z = zStart | y = yEnd, z =

static uint8_t mode = 0x00;

void registerTask(uint8_t index, TaskFunction task, unsigned int period, unsigned int delay) {
  taskList[index].task = task;
  taskList[index].period = period;
  taskList[index].delay = delay;
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

uint8_t transmitCommands(uint8_t * tx, uint8_t len, uint8_t *rx) {
  for (uint8_t i = 0; i < len; i++) {
    ringbufferPut(&tx0Buffer, tx[i], TRUE);
    if (R8_UART0_LSR & RB_LSR_TX_FIFO_EMP) {
      while (ringbufferAvailable(&tx0Buffer) && R8_UART0_TFC < UART_FIFO_SIZE) {
        R8_UART0_THR = ringbufferGet(&tx0Buffer);
      }
    }
  }

  flushUart0Tx();
  uint32_t lastReceivedAt = jiffies;

  while (jiffies != (uint32_t)(lastReceivedAt + 4)) {
    __WFI();
  }

  uint8_t j = 0, t;
  while (ringbufferAvailable(&rx0Buffer)) {
    t = ringbufferGet(&rx0Buffer);;
    if (rx) {
      rx[j] = t;
    }
    j++;
  }
  return j;
}

void handleATFWD(uint8_t * payload, uint8_t len) {
  static uint8_t rx[64];

  uint8_t l = transmitCommands(payload, len, rx);

  for (uint8_t i = 0; i < l; i++) {
    printf("%02X", rx[i]);
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
  // { "AT+", FALSE, },
  { NULL, TRUE, NULL}  // End marker
};

void taskAtCommands() {
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
}

void keyboardInit() {
  GPIOB_ModeCfg(KEYBOARD_UP, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(KEYBOARD_DOWN, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(KEYBOARD_LEFT, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(KEYBOARD_RIGHT, GPIO_ModeIN_PU);

  // GPIOB_ModeCfg(ONBOARD_KEY, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(ONBOARD_DOWNLOAD, GPIO_ModeIN_PU);

  GPIOA_ModeCfg(KEYBOARD_MID, GPIO_ModeIN_PU);
  GPIOA_ModeCfg(KEYBOARD_SET, GPIO_ModeIN_PU);
  GPIOA_ModeCfg(KEYBOARD_RST, GPIO_ModeIN_PU);
}

static const uint32_t btnsBMask = KEYBOARD_UP | KEYBOARD_DOWN | KEYBOARD_LEFT | KEYBOARD_RIGHT | ONBOARD_DOWNLOAD;
static const uint32_t btnsAMask = KEYBOARD_MID | KEYBOARD_RST | KEYBOARD_SET;

void taskKeyboardPool() {
  // static uint8_t rx[64];
  static uint32_t btnsBLast = 0, btnsALast = 0;

  uint32_t btnsB = GPIOB_ReadPortPin(btnsBMask) ^ (btnsBMask);
  uint32_t btnsA = GPIOA_ReadPortPin(btnsAMask) ^ (btnsAMask);

  if (btnsB == btnsBLast && btnsA == btnsALast) {
    return;
  }

  switch (btnsB & (KEYBOARD_LEFT | KEYBOARD_RIGHT)) {
    case KEYBOARD_LEFT:
      transmitCommands((uint8_t []){ setting.xId, 0xFB, setting.xStart >= 0 ? 0 : 1, BYTE1(setting.xSpeed), BYTE0(setting.xSpeed), BYTE3(ABS(setting.xStart)), BYTE2(ABS(setting.xStart)), BYTE1(ABS(setting.xStart)), BYTE0(ABS(setting.xStart)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case KEYBOARD_RIGHT:
      transmitCommands((uint8_t []){ setting.xId, 0xFB, setting.xEnd >= 0 ? 0 : 1, BYTE1(setting.xSpeed), BYTE0(setting.xSpeed), BYTE3(ABS(setting.xEnd)), BYTE2(ABS(setting.xEnd)), BYTE1(ABS(setting.xEnd)), BYTE0(ABS(setting.xEnd)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case 0:
      transmitCommands((uint8_t []){ setting.xId, 0xFE, 0x98, 0x01, 0x6B }, 5, NULL);
      break;
  }

  switch (btnsB & (KEYBOARD_DOWN | KEYBOARD_UP)) {
    case KEYBOARD_UP:
      transmitCommands((uint8_t []){ setting.yId, 0xFB, setting.yStart >= 0 ? 0 : 1, BYTE1(setting.ySpeed), BYTE0(setting.ySpeed), BYTE3(ABS(setting.yStart)), BYTE2(ABS(setting.yStart)), BYTE1(ABS(setting.yStart)), BYTE0(ABS(setting.yStart)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case KEYBOARD_DOWN:
      transmitCommands((uint8_t []){ setting.yId, 0xFB, setting.yEnd >= 0 ? 0 : 1, BYTE1(setting.ySpeed), BYTE0(setting.ySpeed), BYTE3(ABS(setting.yEnd)), BYTE2(ABS(setting.yEnd)), BYTE1(ABS(setting.yEnd)), BYTE0(ABS(setting.yEnd)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case 0:
      transmitCommands((uint8_t []){ setting.yId, 0xFE, 0x98, 0x01, 0x6B }, 5, NULL);
      break;
  }

  switch (btnsA & (KEYBOARD_SET | KEYBOARD_RST)) {
    case KEYBOARD_SET:
      transmitCommands((uint8_t []){ setting.zId, 0xFB, setting.zStart >= 0 ? 0 : 1, BYTE1(setting.zSpeed), BYTE0(setting.zSpeed), BYTE3(ABS(setting.zStart)), BYTE2(ABS(setting.zStart)), BYTE1(ABS(setting.zStart)), BYTE0(ABS(setting.zStart)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case KEYBOARD_RST:
      transmitCommands((uint8_t []){ setting.zId, 0xFB, setting.zEnd >= 0 ? 0 : 1, BYTE1(setting.zSpeed), BYTE0(setting.zSpeed), BYTE3(ABS(setting.zEnd)), BYTE2(ABS(setting.zEnd)), BYTE1(ABS(setting.zEnd)), BYTE0(ABS(setting.zEnd)), 0x01, 0x01, 0x6B }, 12, NULL);
      break;
    case 0:
      transmitCommands((uint8_t []){ setting.zId, 0xFE, 0x98, 0x01, 0x6B }, 5, NULL);
      break;
  }


  if (mode == 0x00) {
    if (btnsA & KEYBOARD_MID) {
      transmitCommands((uint8_t []){ setting.xId, 0x0A, 0x6D, 0x6B }, 4, NULL); // reset coordintate
      transmitCommands((uint8_t []){ setting.yId, 0x0A, 0x6D, 0x6B }, 4, NULL);
      transmitCommands((uint8_t []){ setting.zId, 0x0A, 0x6D, 0x6B }, 4, NULL);

      transmitCommands((uint8_t []){ setting.xId, 0xF3, 0xAB, 0x01, 0x01, 0x6B }, 6, NULL); // enable
      transmitCommands((uint8_t []){ setting.yId, 0xF3, 0xAB, 0x01, 0x01, 0x6B }, 6, NULL);
      transmitCommands((uint8_t []){ setting.zId, 0xF3, 0xAB, 0x01, 0x01, 0x6B }, 6, NULL);

      // transmitCommands((uint8_t []){ 0x00, 0xFF, 0x66, 0x6B }, 4, NULL);
      // int16_t zOffset = -8000;
      // transmitCommands((uint8_t []){ setting.zId, 0xFB, zOffset >= 0 ? 0 : 1, BYTE1(setting.zSpeed), BYTE0(setting.zSpeed), BYTE3(ABS(zOffset)), BYTE2(ABS(zOffset)), BYTE1(ABS(zOffset)), BYTE0(ABS(zOffset)), 0x01, 0x01, 0x6B }, 12, NULL);

      mode = 1;
    }
  }

  if (mode == 1) {
    btnsA & KEYBOARD_MID ? GPIOA_SetBits(CLAW) : GPIOA_ResetBits(CLAW);
  }

  if (btnsB & ONBOARD_DOWNLOAD) { // deactive
    mode = 0;
    transmitCommands((uint8_t []){ setting.xId, 0xF3, 0xAB, 0x00, 0x01, 0x6B }, 6, NULL);
    transmitCommands((uint8_t []){ setting.yId, 0xF3, 0xAB, 0x00, 0x01, 0x6B }, 6, NULL);
    transmitCommands((uint8_t []){ setting.zId, 0xF3, 0xAB, 0x00, 0x01, 0x6B }, 6, NULL);
  }

  transmitCommands((uint8_t []){ 0x00, 0xFF, 0x66, 0x6B }, 4, NULL);

  btnsBLast = btnsB;
  btnsALast = btnsA;
}

void dispatchTasks(void) {
  while (1) {
    BOOL idle = TRUE;
    for (uint8_t i = 0; i < TASK_LEN; i++) {
      if (taskList[i].ready) {
        taskList[i].ready = FALSE;
        idle = FALSE;
        taskList[i].task();
      }
    }
    if (idle) {
      __WFI();
      __nop();
      __nop();
    }
  }
}

void taskBlink(void) {
  GPIOB_InverseBits(LED1);
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 1800); // 1800Hz

  keyboardInit();

  GPIOB_ResetBits(CLAW);
  GPIOA_ModeCfg(CLAW, GPIO_ModeOut_PP_20mA);

  ringbufferInit(&tx0Buffer, 64);
  ringbufferInit(&rx0Buffer, 128);

  ringbufferInit(&tx1Buffer, 64);
  ringbufferInit(&rx1Buffer, 128);

  GPIOB_ModeCfg(LED1, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED1);

  GPIOB_ModeCfg(LED2, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED2);

  GPIOB_ResetBits(GPIO_Pin_3);
  GPIOB_ModeCfg(GPIO_Pin_3, GPIO_ModeOut_PP_5mA);

  GPIOB_SetBits(GPIO_Pin_7);
  GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA); // TXD: PB7, pushpull, but set it high beforehand
  GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeOut_PP_5mA); // DTR: PB5, pushpull
  GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);      // RXD: RB4, in with pullup

  UART0_BaudRateCfg(115200);

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

  registerTask(0, taskBlink, 180, 1);
  registerTask(1, taskAtCommands, 12, 0);
  registerTask(2, taskKeyboardPool, 120, 2);  // 1800 / 120 = 15 Hz

  dispatchTasks();
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  static volatile uint8_t i;
  for (i = 0; i < TASK_LEN; i++) {
    if (taskList[i].delay == 0) {
      taskList[i].ready = TRUE;
      taskList[i].delay = taskList[i].period - 1;
    } else {
      taskList[i].delay--;
    }
  }

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

// > 02 39 6B
// < 02 39 00 16 6B
