#include "app_generic_onoff_client_model.h"
#include "app_mesh_config.h"
#include "config.h"

static uint8 generic_onoff_TaskID = 0; // Task ID for internal task/event processing

static struct bt_mesh_generic_onoff_cli *generic_onoff_cli;
static s32_t msg_timeout = K_SECONDS(2);
static uint8_t cli_send_tid = 128;

/** The following are the macro definitions of generic client
 *  model messages length, and a message is composed of three
 *  parts: Opcode + msg_value + MIC
 */
/* Generic onoff client messages length */
#define BLE_MESH_GEN_ONOFF_GET_MSG_LEN (2 + 0 + 4)
#define BLE_MESH_GEN_ONOFF_SET_MSG_LEN (2 + 4 + 4)

static uint16 generic_onoff_ProcessEvent(uint8 task_id, uint16 events);

uint8_t cli_tid_get(void) {
  cli_send_tid++;
  if (cli_send_tid > 191)
    cli_send_tid = 128;
  return cli_send_tid;
}

static void cli_reset(void) {
  APP_DBG("");
  generic_onoff_cli->op_pending = 0U;
  generic_onoff_cli->op_req = 0U;

  tmos_stop_task(generic_onoff_TaskID, GEN_ONOFF_SYNC_EVT);
}

/*******************************************************************************
 * Function Name  : generic_onoff_rsp_recv
 * Description    : 调用应用层传入的回调
 * Input          : None
 * Return         : None
 *******************************************************************************/
static void generic_onoff_rsp_recv(generic_onoff_cli_status_t *val, u8_t status) {
  if (generic_onoff_cli == NULL) {
    return;
  }

  val->generic_onoff_Hdr.opcode = generic_onoff_cli->op_req;
  val->generic_onoff_Hdr.status = status;

  cli_reset();

  if (generic_onoff_cli->handler) {
    generic_onoff_cli->handler(val);
  }
}

static int cli_wait(void) {
  return tmos_start_task(generic_onoff_TaskID, GEN_ONOFF_SYNC_EVT, msg_timeout);
}

/*******************************************************************************
 * Function Name  : cli_prepare
 * Description    :
 * Input          : None
 * Return         : None
 *******************************************************************************/
static int cli_prepare(u32_t op_req, u32_t op) {
  if (!generic_onoff_cli) {
    APP_DBG("No available Configuration Client context!");
    return -EINVAL;
  }

  if (generic_onoff_cli->op_pending) {
    APP_DBG("Another synchronous operation pending");
    return -EBUSY;
  }

  generic_onoff_cli->op_req = op_req;
  generic_onoff_cli->op_pending = op;

  return 0;
}

/*******************************************************************************
 * Function Name  : sync_handler
 * Description    : 通知应用层当前op_code超时了
 * Input          : None
 * Return         : None
 *******************************************************************************/
static void sync_handler(void) {
  APP_DBG("");

  generic_onoff_cli_status_t generic_onoff_cli_status;

  memset(&generic_onoff_cli_status, 0, sizeof(generic_onoff_cli_status_t));

  generic_onoff_cli_status.generic_onoff_Hdr.opcode = generic_onoff_cli->op_pending;

  generic_onoff_rsp_recv(&generic_onoff_cli_status, 0xFF);
}

/*******************************************************************************
 * Function Name  : generic_onoff_cli_init
 * Description    :
 * Input          : None
 * Return         : None
 *******************************************************************************/
int generic_onoff_cli_init(struct bt_mesh_model *model) {
  // if (!bt_mesh_model_in_primary(model)) {
  //   BT_ERR("Configuration Client only allowed in primary element");
  //   return -EINVAL;
  // }
  if (!model->user_data) {
    APP_DBG("No Configuration Client context provided");
    return -EINVAL;
  }

  generic_onoff_cli = model->user_data;
  generic_onoff_cli->model = model;

  /* Configuration Model security is device-key based and both the local
   * and remote keys are allowed to access this model.
   */
  model->keys[0] = BLE_MESH_KEY_DEV_ANY;

  generic_onoff_TaskID = TMOS_ProcessEventRegister(generic_onoff_ProcessEvent);
  APP_DBG("%04x", model->id);
  return 0;
}

static uint16 generic_onoff_ProcessEvent(uint8 task_id, uint16 events) {
  if (events & GEN_ONOFF_SYNC_EVT) {
    sync_handler();
    return (events ^ GEN_ONOFF_SYNC_EVT);
  }
  return 0;
}

/*******************************************************************************
 * Function Name  : generic_onoff_status
 * Description    : 收到 onoff 的状态回复
 * Input          : None
 * Return         : None
 *******************************************************************************/
static void generic_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple *buf) {
  generic_onoff_cli_status_t generic_onoff_cli_status;

  APP_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u", ctx->net_idx,
          ctx->app_idx, ctx->addr, buf->len);

  if (generic_onoff_cli->op_pending != BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS) {
    APP_DBG("Unexpected Status (0x%08x != 0x%08x)", generic_onoff_cli->op_pending,
            BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);
    return;
  }

  generic_onoff_cli_status.generic_onoff_Event.status.state = buf->data[0];

  generic_onoff_rsp_recv(&generic_onoff_cli_status, SUCCESS);
}

const struct bt_mesh_model_op generic_onoff_cli_op[] = {
  { BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, generic_onoff_status },
  BLE_MESH_MODEL_OP_END,
};

/*******************************************************************************
 * Function Name  : bt_mesh_generic_onoff_get
 * Description    : {BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, generic_onoff_status}
 * Input          : None
 * Return         : None
 *******************************************************************************/
int bt_mesh_generic_onoff_get(u16_t net_idx, u16_t app_idx, u16_t addr) {
  NET_BUF_SIMPLE_DEFINE(msg, BLE_MESH_GEN_ONOFF_GET_MSG_LEN);
  struct bt_mesh_msg_ctx ctx = {
      .net_idx = net_idx,
      .app_idx = app_idx,
      .addr = addr,
      .send_ttl = BLE_MESH_TTL_DEFAULT,
  };

  int err;

  err = cli_prepare(BLE_MESH_MODEL_OP_GEN_ONOFF_GET,
                    BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);
  if (err) {
    return err;
  }

  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_GET);

  err = bt_mesh_model_send(generic_onoff_cli->model, &ctx, &msg, NULL, NULL);
  if (err) {
    APP_DBG("model_send() failed (err %d)", err);
    cli_reset();
    return err;
  }

  return cli_wait();
}

/*******************************************************************************
 * Function Name  : bt_mesh_generic_onoff_set
 * Description    : {BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, generic_onoff_status}
 * Input          : None
 * Return         : None
 *******************************************************************************/
int bt_mesh_generic_onoff_set(u16_t net_idx, u16_t app_idx, u16_t addr,
                          struct bt_mesh_generic_onoff_set_val const *set) {
  NET_BUF_SIMPLE_DEFINE(msg, BLE_MESH_GEN_ONOFF_SET_MSG_LEN);
  struct bt_mesh_msg_ctx ctx = {
      .net_idx = net_idx,
      .app_idx = app_idx,
      .addr = addr,
      .send_ttl = BLE_MESH_TTL_DEFAULT,
  };

  int err;

  err = cli_prepare(BLE_MESH_MODEL_OP_GEN_ONOFF_SET,
                    BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);
  if (err) {
    return err;
  }

  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
  net_buf_simple_add_u8(&msg, set->onoff);
  net_buf_simple_add_u8(&msg, set->tid);
  if (set->op_en) {
    net_buf_simple_add_u8(&msg, set->trans_time);
    net_buf_simple_add_u8(&msg, set->delay);
  }

  err = bt_mesh_model_send(generic_onoff_cli->model, &ctx, &msg, NULL, NULL);
  if (err) {
    APP_DBG("model_send() failed (err %d)", err);
    cli_reset();
    return err;
  }

  return cli_wait();
}

/*******************************************************************************
 * Function Name  : bt_mesh_generic_onoff_set_unack
 * Description    : {BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, generic_onoff_status}
 * Input          : None
 * Return         : None
 *******************************************************************************/
int bt_mesh_generic_onoff_set_unack(u16_t net_idx, u16_t app_idx, u16_t addr,
                                struct bt_mesh_generic_onoff_set_val const *set) {
  NET_BUF_SIMPLE_DEFINE(msg, BLE_MESH_GEN_ONOFF_SET_MSG_LEN);
  struct bt_mesh_msg_ctx ctx = {
      .net_idx = net_idx,
      .app_idx = app_idx,
      .addr = addr,
      .send_ttl = 0,
  };

  int err;

  err = cli_prepare(BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);

  if (err) {
    return err;
  }

  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
  net_buf_simple_add_u8(&msg, set->onoff);
  net_buf_simple_add_u8(&msg, set->tid);
  if (set->op_en) {
    net_buf_simple_add_u8(&msg, set->trans_time);
    net_buf_simple_add_u8(&msg, set->delay);
  }

  err = bt_mesh_model_send(generic_onoff_cli->model, &ctx, &msg, NULL, NULL);

  cli_reset();
  return err;
}
