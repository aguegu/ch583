#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#include "config.h"
#include "HAL.h"

#include "generic_onoff_server_model.h"

extern struct bt_mesh_generic_onoff_server generic_onoff_server;

void App_Init(void);

uint8_t bt_mesh_lib_init(void);
void blemesh_on_sync(const struct bt_mesh_comp *app_comp);

#ifdef __cplusplus
}
#endif

#endif
