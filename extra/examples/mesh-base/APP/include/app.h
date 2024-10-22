#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#include "config.h"

// LEDs are ON when pins are low
#define LED_UNPROVISION     GPIO_Pin_19
#define BUTTON_UNPROVISION  GPIO_Pin_4

void blemesh_on_sync(void);

#ifdef __cplusplus
}
#endif

#endif
