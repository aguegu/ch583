#ifndef APP_GENERIC_ONOFF_CLIENT_MODEL_H
#define APP_GENERIC_ONOFF_CLIENT_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

extern const struct bt_mesh_model_op generic_onoff_client_ops[];

struct bt_mesh_generic_onoff_client {
  uint8_t tid;
  BOOL isAckExpected;
  BOOL isAcked;
  BOOL (*readState)();
  void (*onStatus)(uint16_t address, BOOL state);
};

int generic_onoff_client_set_unack(struct bt_mesh_model *model);
int generic_onoff_client_set(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
