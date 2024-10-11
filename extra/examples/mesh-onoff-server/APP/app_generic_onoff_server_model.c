#include "app_generic_onoff_server_model.h"
#include "MESH_LIB.h"
#include "app_mesh_config.h"
#include "config.h"

BOOL read_led_state(uint32_t led_pin) {
  return (GPIOB_ReadPortPin(led_pin) > 0) ? 0 : 1;
}

void set_led_state(uint32_t led_pin, BOOL on) {
  GPIOB_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);
  on ? GPIOB_ResetBits(led_pin) : GPIOB_SetBits(led_pin);
}

void toggle_led_state(uint32_t led_pin) {
  GPIOB_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);
  GPIOB_InverseBits(led_pin);
}

static void generic_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx) {
  NET_BUF_SIMPLE_DEFINE(msg, 32);
  int err;

  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);
  net_buf_simple_add_u8(&msg, read_led_state(MSG_PIN));

  APP_DBG("ttl: 0x%02x recv_dst(me): 0x%04x, addr(from): 0x%04x", ctx->recv_ttl,
          ctx->recv_dst, ctx->addr);
  APP_DBG("msg len: 0x%02x: %02x %02x %02x", msg.len, msg.data[0], msg.data[1],
          msg.data[2]);

  ctx->send_ttl = BLE_MESH_TTL_DEFAULT;
  err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
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
  // APP_DBG("ttl: 0x%02x addr(from): 0x%04x recv_dst(me): 0x%04x rssi: %d",
  //         ctx->recv_ttl, ctx->addr, ctx->recv_dst, ctx->recv_rssi);
  // APP_DBG("buf len: 0x%02x, size: 0x%02x, data: %02x %02x %02x %02x", buf->len,
  //         buf->size, buf->data[0], buf->data[1], buf->data[2], buf->data[3]);

  // data: OnOff, TID (not used), Transition(not used), Delay(not used)
  set_led_state(MSG_PIN, buf->data[0]);
  generic_onoff_status(model, ctx);
}

static void generic_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf) {
  // APP_DBG(" ");
  app_log("bt_mesh_model", model, 6);
  app_log("bt_mesh_msg_ctx", ctx, sizeof(struct bt_mesh_msg_ctx));
  app_log("net_buf_simple", buf->data, buf->len);

  uint8_t status;
  status = read_led_state(MSG_PIN);
  if (status != buf->data[0]) {
    set_led_state(MSG_PIN, buf->data[0]);
  }
}

const struct bt_mesh_model_op generic_onoff_op[] = {
  {BLE_MESH_MODEL_OP_GEN_ONOFF_GET, 0, generic_onoff_get},  // 0x8201
  {BLE_MESH_MODEL_OP_GEN_ONOFF_SET, 2, generic_onoff_set},  // 0x8202
  {BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, generic_onoff_set_unack},  // 0x8203
  BLE_MESH_MODEL_OP_END,
};
