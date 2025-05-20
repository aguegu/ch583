#include "generic_onoff_server_model.h"

static void generic_onoff_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx) {
  NET_BUF_SIMPLE_DEFINE(msg, 32);
  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS); // 0x8204

  if (((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState) {
    net_buf_simple_add_u8(&msg, ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState());
  }

  APP_DBG("ttl: 0x%02x recv_dst(me): 0x%04x, addr(from): 0x%04x", ctx->recv_ttl,
          ctx->recv_dst, ctx->addr);
  APP_DBG("msg len: 0x%02x: %02x %02x %02x", msg.len, msg.data[0], msg.data[1],
          msg.data[2]);

  ctx->send_ttl = BLE_MESH_TTL_DEFAULT;
  int err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);

  APP_DBG("err: %d", err);

  if (err) {
    APP_DBG("send status failed: %d", err);
  }
}

static void generic_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf) {
  APP_DBG(" ");
  generic_onoff_status(model, ctx);
}

static void generic_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf) {
  APP_DBG("ttl: 0x%02x addr(from): 0x%04x recv_dst(me): 0x%04x rssi: %d",
          ctx->recv_ttl, ctx->addr, ctx->recv_dst, ctx->recv_rssi);
  APP_DBG("buf len: 0x%02x, size: 0x%02x, data: %02x %02x %02x %02x", buf->len,
          buf->size, buf->data[0], buf->data[1], buf->data[2], buf->data[3]);

  // data: OnOff, TID (not used), Transition(not used), Delay(not used)
  ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onWriteState(buf->data[0]);

  generic_onoff_status(model, ctx);
}

static void generic_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf) {
  APP_DBG("ttl: 0x%02x addr(from): 0x%04x recv_dst(me): 0x%04x rssi: %d",
          ctx->recv_ttl, ctx->addr, ctx->recv_dst, ctx->recv_rssi);
  APP_DBG("buf len: 0x%02x, size: 0x%02x, data: %02x %02x %02x %02x", buf->len,
          buf->size, buf->data[0], buf->data[1], buf->data[2], buf->data[3]);
  // APP_DBG(" ");
  // app_log("bt_mesh_model", model, 6);
  // app_log("bt_mesh_msg_ctx", ctx, sizeof(struct bt_mesh_msg_ctx));
  // app_log("net_buf_simple", buf->data, buf->len);
  ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onWriteState(buf->data[0]);
}

const struct bt_mesh_model_op generic_onoff_server_ops[] = {
  { BLE_MESH_MODEL_OP_GEN_ONOFF_GET, 0, generic_onoff_get },  // 0x8201
  { BLE_MESH_MODEL_OP_GEN_ONOFF_SET, 2, generic_onoff_set },  // 0x8202
  { BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, generic_onoff_set_unack },  // 0x8203
  BLE_MESH_MODEL_OP_END,
};

const static uint32_t periodResolutions[4] = { 100, 1000, 10000, 600000 };

uint32_t periodToMilliseconds(uint8_t period) {
  return periodResolutions[period >> 6] * (period & 0x3f);
}

int generic_onoff_srv_pub_update(struct bt_mesh_model *model) {
  APP_DBG("addr: %x", model->pub->addr);
  // APP_DBG("period: %x", model->pub->period);
  APP_DBG("period_start: %x", model->pub->period_start);

  uint32_t period = periodToMilliseconds(model->pub->period);
  APP_DBG("period: %ld ms", period);

  // bt_mesh_model_msg_init(model->pub->msg, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS); // 0x8204
  // if (((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState) {
  //   net_buf_simple_add_u8(model->pub->msg, ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState());
  // }

  return -1;
}
