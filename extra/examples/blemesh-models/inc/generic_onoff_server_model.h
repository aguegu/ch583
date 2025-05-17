#ifndef GENERIC_ONOFF_SERVER_MODEL_H
#define GENERIC_ONOFF_SERVER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

extern const struct bt_mesh_model_op generic_onoff_server_ops[];

struct bt_mesh_generic_onoff_server {
  BOOL (*onReadState)();
  void (*onWriteState)(BOOL state);
};

void generic_onoff_status_publish(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
