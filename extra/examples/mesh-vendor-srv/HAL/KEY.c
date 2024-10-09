#include "HAL.h"

static uint8_t lastRead;
static HalKeyCallback pHalKeyProcessFunction;

void HAL_KeyInit(void) {
  lastRead = 0;
  pHalKeyProcessFunction = NULL;
  KEY1_DIR;
  KEY1_PU;
  KEY2_DIR;
  KEY2_PU;
}

void HAL_KeyConfig(HalKeyCallback cback) {
  pHalKeyProcessFunction = cback;
  tmos_start_task(halTaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE);
}

uint8_t HAL_KeyRead(void) {
  uint8_t keys = 0;

  if (HAL_PUSH_BUTTON1()) {
    keys |= HAL_KEY_SW_1;
  }
  if (HAL_PUSH_BUTTON2()) {
    keys |= HAL_KEY_SW_2;
  }
  return keys;
}

void HAL_KeyPoll(void) {
  uint8_t current = HAL_KeyRead();

  if (current == lastRead)
    return;

  HalKeyChangeEvent e = {
    .current = current,
    .changed = current ^ lastRead,
  };
  lastRead = current;
  if (pHalKeyProcessFunction) {
    (pHalKeyProcessFunction)(e);
  }
}
