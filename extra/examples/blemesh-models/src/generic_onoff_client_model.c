#include "generic_onoff_client_model.h"

#define BLE_MESH_GEN_ONOFF_SET_MSG_LEN (2 + 4 + 4)

static void generic_onoff_client_status(struct bt_mesh_model *model,
                                        struct bt_mesh_msg_ctx *ctx,
                                        struct net_buf_simple *buf) {
  // APP_DBG("net_idx 0x%04x app_idx 0x%04x src 0x%04x len %u", ctx->net_idx,
  // ctx->app_idx, ctx->addr, buf->len);

  ((struct bt_mesh_generic_onoff_client *)(model->user_data))->isAcked = TRUE;
  ((struct bt_mesh_generic_onoff_client *)(model->user_data))->isAckExpected =
      FALSE;

  ((struct bt_mesh_generic_onoff_client *)(model->user_data))
      ->onStatus(ctx->addr, buf->data[0]);
}

const struct bt_mesh_model_op generic_onoff_client_ops[] = {
    {BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1,
     generic_onoff_client_status}, // 0x8204
    BLE_MESH_MODEL_OP_END,
};

static void updateTid(struct bt_mesh_model *model) {
  uint8_t *const p =
      &(((struct bt_mesh_generic_onoff_client *)(model->user_data))->tid);
  (*p)++;
  if (*p > 191) {
    *p = 128;
  }
}

int generic_onoff_client_set(struct bt_mesh_model *model, BOOL isAckExpected) {
  NET_BUF_SIMPLE_DEFINE(msg, BLE_MESH_GEN_ONOFF_SET_MSG_LEN);
  bt_mesh_model_msg_init(&msg, isAckExpected ? BLE_MESH_MODEL_OP_GEN_ONOFF_SET: BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);

  net_buf_simple_add_u8(
      &msg,
      ((struct bt_mesh_generic_onoff_client *)(model->user_data))->readState());
  net_buf_simple_add_u8(
      &msg, ((struct bt_mesh_generic_onoff_client *)(model->user_data))->tid);

  updateTid(model);

  // APP_DBG("msg len: 0x%02x: %02x %02x %02x %02x", msg.len, msg.data[0],
  // msg.data[1], msg.data[2], msg.data[3]);

  struct bt_mesh_msg_ctx ctx = {
      .net_idx = bt_mesh_app_key_find(model->pub->key)->net_idx,
      .app_idx = model->pub->key,
      .addr = model->pub->addr,
      .send_ttl = BLE_MESH_TTL_DEFAULT,
  };

  ((struct bt_mesh_generic_onoff_client *)(model->user_data))->isAckExpected = isAckExpected;
  ((struct bt_mesh_generic_onoff_client *)(model->user_data))->isAcked = FALSE;

  return bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
}
