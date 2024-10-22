#include "HAL.h"
#include "app.h"

#define APP_RESET_MESH_EVENT (1 << 0)
#define APP_BUTTON_POLL_EVENT (1 << 1)

static uint8_t App_TaskID = 0;

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

void pinsInit() {
  GPIOB_ModeCfg(BUTTON_UNPROVISION, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(LED_UNPROVISION, GPIO_ModeOut_PP_5mA);
  GPIOB_ResetBits(LED_UNPROVISION);
}

void buttonsPoll() {
  static uint32_t pinResetPressedAt;
  static BOOL pinResetPressed = FALSE;
  static uint32_t buttons = BUTTON_UNPROVISION;
  uint32_t buttonsNow = GPIOB_ReadPortPin(BUTTON_UNPROVISION);

  if (buttonsNow != buttons) {
    if (((buttonsNow ^ buttons) & BUTTON_UNPROVISION) && !(buttonsNow & BUTTON_UNPROVISION) ) {
      APP_DBG("RESET pressed");
      pinResetPressed = TRUE;
      pinResetPressedAt = TMOS_GetSystemClock();
    }
    APP_DBG("buttons: %08x", buttonsNow);
  }

  if (pinResetPressed && !(buttonsNow & BUTTON_UNPROVISION)) {
    if (TMOS_GetSystemClock() - pinResetPressedAt > 9600) { // 9600 * 0.625 ms = 6s
      APP_DBG("duration: %d, about to self unprovision", TMOS_GetSystemClock() - pinResetPressedAt);
      tmos_set_event(App_TaskID, APP_RESET_MESH_EVENT);
      pinResetPressed = FALSE;
    }
  }

  buttons = buttonsNow;
}

void App_Init() {
  App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);
  pinsInit();

  blemesh_on_sync();
  tmos_set_event(App_TaskID, APP_BUTTON_POLL_EVENT); /* Kick off polling */
}

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & APP_RESET_MESH_EVENT) {
    bt_mesh_reset();
    return (events ^ APP_RESET_MESH_EVENT);
  }

  if (events & APP_BUTTON_POLL_EVENT) {
    buttonsPoll();
    tmos_start_task(App_TaskID, APP_BUTTON_POLL_EVENT, MS1_TO_SYSTEM_TIME(500));
    return events ^ APP_BUTTON_POLL_EVENT;
  }
  return 0;
}
