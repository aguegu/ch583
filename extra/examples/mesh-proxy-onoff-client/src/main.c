#include "app.h"
#include "gpio.h"
#include "ringbuffer.h"

const static Gpio led = {.portOut = &R32_PB_OUT, .pin = GPIO_Pin_18};

const static Gpio btnDownload = { .portOut = &R32_PB_OUT, .pin = GPIO_Pin_22 };
const static Gpio btnKey = { .portOut = &R32_PB_OUT, .pin = GPIO_Pin_4 };

const static Gpio uart1Tx = {.portOut = &R32_PA_OUT, .pin = GPIO_Pin_9};

#define MAIN_EVENT_PollButtons (1 << 0)
#define MAIN_EVENT_GenericOnoffClientAck (1 << 1)

tmosTaskID mainTaskId;

__HIGH_CODE
__attribute__((noinline)) void Main_Circulation() {
  while (1) {
    TMOS_SystemProcess();
  }
}

void pollButtons() {
  static BOOL downloadPressed = FALSE;
  static uint32_t downloadPressedAt;

  static uint32_t stateDownloadLast = 0, stateKeyLast = 0;
  static BOOL resetTriggered = FALSE;

  uint32_t stateDownloadNow = !gpioRead(&btnDownload);

  if (stateDownloadNow != stateDownloadLast) {
    if (stateDownloadNow) {
      downloadPressed = TRUE;
      downloadPressedAt = TMOS_GetSystemClock();
    } else {
      downloadPressed = FALSE;
      resetTriggered = FALSE;
      if (TMOS_GetSystemClock() - downloadPressedAt < 3200) { // 2 seconds, short click
        gpioInverse(&led);
      }
    }
  }

  if (downloadPressed && stateDownloadNow) {
    if (TMOS_GetSystemClock() - downloadPressedAt > 9600 && !resetTriggered) {  // pressed over 6 seconds
      bt_mesh_reset();
      APP_DBG("mesh reset");
      resetTriggered = TRUE;
    }
  }

  stateDownloadLast = stateDownloadNow;

  uint32_t stateKeyNow = !gpioRead(&btnKey);

  if (stateKeyNow != stateKeyLast) {
    int err = genericOnoffClient_set(FALSE);
    // int err = genericOnoffClient_set(TRUE);
    // tmos_start_task(mainTaskId, MAIN_EVENT_GenericOnoffClientAck, MS1_TO_SYSTEM_TIME(625));
    APP_DBG("%d, %d", stateKeyNow, err);
  }

  stateKeyLast = stateKeyNow;
}

RingBuffer txBuffer;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbufferPut(&txBuffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_ALL_EMP) {
      R8_UART1_THR = ringbufferGet(&txBuffer);
    }
  }
  return size;
}

BOOL readButton() { return !gpioRead(&btnKey); }

void logStatus(uint16_t address, BOOL state) {
  printf("{\"level\": \"info\", \"source\": %04x, \"state\": %d}\n", address,
         state);
}

struct bt_mesh_generic_onoff_client generic_onoff_client = {
    .tid = 128,
    .readState = readButton,
    .onStatus = logStatus,
    .isAcked = FALSE,
    .isAckExpected = FALSE,
};

void pinsInit() {
  gpioReset(&led);
  gpioMode(&led, GPIO_ModeOut_PP_5mA);

  gpioMode(&btnDownload, GPIO_ModeIN_PU);

  gpioMode(&btnKey, GPIO_ModeIN_PU);
  ringbufferInit(&txBuffer, 64);

  gpioSet(&uart1Tx);
  // gpioMode(&uart1Rx, GPIO_ModeIN_PU);
  gpioMode(&uart1Tx, GPIO_ModeOut_PP_5mA);

  UART1_DefInit();
  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY);
  PFIC_EnableIRQ(UART1_IRQn);
}

static uint16_t Main_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & MAIN_EVENT_PollButtons) {
    pollButtons();
    tmos_start_task(mainTaskId, MAIN_EVENT_PollButtons, MS1_TO_SYSTEM_TIME(100));
    return events ^ MAIN_EVENT_PollButtons;
  }
  if (events & MAIN_EVENT_GenericOnoffClientAck) {
    if (generic_onoff_client.isAckExpected && !generic_onoff_client.isAcked) {
      APP_DBG("ACK timeout");
    }
    return events ^ MAIN_EVENT_GenericOnoffClientAck;
  }
  return 0;
}

int main(void) {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  ringbufferInit(&txBuffer, 64);

  pinsInit();
  App_Init();

  mainTaskId = TMOS_ProcessEventRegister(Main_ProcessEvent);
  tmos_start_task(mainTaskId, MAIN_EVENT_PollButtons, MS1_TO_SYSTEM_TIME(100));

  Main_Circulation();
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
  case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
    while (ringbufferAvailable(&txBuffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
      R8_UART1_THR = ringbufferGet(&txBuffer);
    }
    break;
  }
}
