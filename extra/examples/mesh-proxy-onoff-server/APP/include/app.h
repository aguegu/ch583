#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#include "config.h"

// LEDs: Low: On, High: Off
#define LED_UNPROVISION  GPIO_Pin_19
#define BTN_UNPROVISION  GPIO_Pin_4

// buttons: Low: Pressed, High: Unpressed
#define LED_ONOFF GPIO_Pin_18
#define BTN_ONOFF GPIO_Pin_22

#define APP_RESET_MESH_EVENT (1 << 0)
#define APP_BUTTON_POLL_EVENT (1 << 1)

void App_Init(void);

void blemesh_on_sync(const struct bt_mesh_comp *app_comp);

#ifdef __cplusplus
}
#endif

#endif
