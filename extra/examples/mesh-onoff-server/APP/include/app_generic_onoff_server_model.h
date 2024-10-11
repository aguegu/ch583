#ifndef app_generic_onoff_server_model_H
#define app_generic_onoff_server_model_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#define MSG_PIN GPIO_Pin_18

BOOL read_led_state(uint32_t led_pin);

extern const struct bt_mesh_model_op generic_onoff_op[];

void set_led_state(uint32_t led_pin, BOOL on);

void toggle_led_state(uint32_t led_pin);

void bt_mesh_generic_onoff_status(struct bt_mesh_model *model, u16_t net_idx, u16_t app_idx, u16_t addr);

#ifdef __cplusplus
}
#endif

#endif
