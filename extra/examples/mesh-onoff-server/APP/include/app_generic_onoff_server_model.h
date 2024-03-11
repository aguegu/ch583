#ifndef app_generic_onoff_server_model_H
#define app_generic_onoff_server_model_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#define MSG_PIN GPIO_Pin_18

BOOL read_led_state(uint32_t led_pin);

extern const struct bt_mesh_model_op gen_onoff_op[];

void set_led_state(uint32_t led_pin, BOOL on);

void toggle_led_state(uint32_t led_pin);

#ifdef __cplusplus
}
#endif

#endif
