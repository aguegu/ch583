#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL.h"
#include "MESH_LIB.h"
#include "config.h"

#include "generic_onoff_client_model.h"

extern struct bt_mesh_generic_onoff_client generic_onoff_client;

// // LEDs: Low: On, High: Off
// #define LED_UNPROVISION  GPIO_Pin_19
// #define BTN_UNPROVISION  GPIO_Pin_4
//
// // buttons: Low: Pressed, High: Unpressed
// #define LED_ONOFF GPIO_Pin_18
// #define BTN_ONOFF GPIO_Pin_22

// #define APP_RESET_MESH_EVENT (1 << 0)
// #define APP_BUTTON_POLL_EVENT (1 << 1)
// #define APP_GENERIC_ONOFF_CLIENT_ACK_EVENT (1 << 2)

void App_Init(void);

uint8_t bt_mesh_lib_init(void);
void blemesh_on_sync(const struct bt_mesh_comp *app_comp);

int genericOnoffClient_set(BOOL ackRequired);

#ifdef __cplusplus
}
#endif

#endif
