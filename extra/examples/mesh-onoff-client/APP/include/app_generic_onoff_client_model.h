#ifndef app_generic_onoff_model_cli_H
#define app_generic_onoff_model_cli_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

#define GEN_ONOFF_SYNC_EVT (1 << 0)

struct bt_mesh_generic_onoff_status_data {
  u8_t state;
  u16_t source;
};

struct generic_onoff_EventHdr {
  u8_t status;
  u32_t opcode;
};

union generic_onoff_Event_t {
  struct bt_mesh_generic_onoff_status_data status;
};

typedef struct {
  struct generic_onoff_EventHdr generic_onoff_Hdr;
  union generic_onoff_Event_t generic_onoff_Event;
} generic_onoff_cli_status_t;

typedef void (*generic_onoff_cli_rsp_handler_t)(const generic_onoff_cli_status_t *val);

/** gen onoff Model Context */
struct bt_mesh_generic_onoff_cli {
  struct bt_mesh_model *model;

  generic_onoff_cli_rsp_handler_t handler;

  u32_t op_req;
  u32_t op_pending;
};

extern const struct bt_mesh_model_op generic_onoff_cli_op[];

struct bt_mesh_generic_onoff_set_val {
  BOOL op_en;      /* Indicate whether optional parameters included */
  u8_t onoff;      /* Target value of Generic OnOff state           */
  u8_t tid;        /* Transaction Identifier                        */
  u8_t trans_time; /* Time to complete state transition (optional)  */
  u8_t delay;      /* Indicate message execution delay (C.1)        */
};

int bt_mesh_generic_onoff_get(u16_t net_idx, u16_t app_idx, u16_t addr);
int bt_mesh_generic_onoff_set(u16_t net_idx, u16_t app_idx, u16_t addr,
                          struct bt_mesh_generic_onoff_set_val const *set);
int bt_mesh_generic_onoff_set_unack(u16_t net_idx, u16_t app_idx, u16_t addr,
                                struct bt_mesh_generic_onoff_set_val const *set);

uint8_t cli_tid_get(void);

int generic_onoff_cli_init(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
