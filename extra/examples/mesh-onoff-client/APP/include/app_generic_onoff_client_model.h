#ifndef app_generic_onoff_model_cli_H
#define app_generic_onoff_model_cli_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

#define GEN_ONOFF_SYNC_EVT (1 << 0)

struct bt_mesh_gen_onoff_status_data {
  u8_t state;
};

struct gen_onoff_EventHdr {
  u8_t status;
  u32_t opcode;
};

union gen_onoff_Event_t {
  struct bt_mesh_gen_onoff_status_data status;
};

typedef struct {
  struct gen_onoff_EventHdr gen_onoff_Hdr;
  union gen_onoff_Event_t gen_onoff_Event;
} gen_onoff_cli_status_t;

typedef void (*gen_onoff_cli_rsp_handler_t)(const gen_onoff_cli_status_t *val);

/** gen onoff Model Context */
struct bt_mesh_gen_onoff_cli {
  struct bt_mesh_model *model;

  gen_onoff_cli_rsp_handler_t handler;

  u32_t op_req;
  u32_t op_pending;
};

extern const struct bt_mesh_model_op gen_onoff_cli_op[];

struct bt_mesh_gen_onoff_set_val {
  BOOL op_en;      /* Indicate whether optional parameters included */
  u8_t onoff;      /* Target value of Generic OnOff state           */
  u8_t tid;        /* Transaction Identifier                        */
  u8_t trans_time; /* Time to complete state transition (optional)  */
  u8_t delay;      /* Indicate message execution delay (C.1)        */
};

int bt_mesh_gen_onoff_get(u16_t net_idx, u16_t app_idx, u16_t addr);
int bt_mesh_gen_onoff_set(u16_t net_idx, u16_t app_idx, u16_t addr,
                          struct bt_mesh_gen_onoff_set_val const *set);
int bt_mesh_gen_onoff_set_unack(u16_t net_idx, u16_t app_idx, u16_t addr,
                                struct bt_mesh_gen_onoff_set_val const *set);

uint8_t cli_tid_get(void);

int gen_onoff_cli_init(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
