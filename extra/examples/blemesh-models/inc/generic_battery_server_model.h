#ifndef GENERIC_BATTERY_SERVER_MODEL_H
#define GENERIC_BATTERY_SERVER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

extern const struct bt_mesh_model_op generic_battery_server_ops[];

struct bt_mesh_generic_battery_server {
  uint8_t (*onReadState)();
};

void generic_battery_status_publish(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
