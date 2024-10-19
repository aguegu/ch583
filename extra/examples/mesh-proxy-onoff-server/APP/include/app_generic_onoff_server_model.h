#ifndef APP_GENERIC_ONOFF_SERVER_MODEL_H
#define APP_GENERIC_ONOFF_SERVER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

extern const struct bt_mesh_model_op generic_onoff_server_ops[];

typedef BOOL (*bt_mesh_generic_onoff_server_state_read_t)();
typedef void (*bt_mesh_generic_onoff_server_state_write_t)(BOOL state);

struct bt_mesh_generic_onoff_server {
  bt_mesh_generic_onoff_server_state_read_t onReadState;
  bt_mesh_generic_onoff_server_state_write_t onWriteState;
};

void bt_mesh_generic_onoff_status(struct bt_mesh_model *model, u16_t net_idx, u16_t app_idx, u16_t addr);

#ifdef __cplusplus
}
#endif

#endif
