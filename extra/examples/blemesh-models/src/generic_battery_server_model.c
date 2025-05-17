#include "generic_battery_server_model.h"

static void generic_battery_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx) {
  NET_BUF_SIMPLE_DEFINE(msg, 32);
  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_BATTERY_STATUS); // 0x8224

  if (((struct bt_mesh_generic_battery_server *)(model->user_data))->onReadState) {
    net_buf_simple_add_u8(&msg, ((struct bt_mesh_generic_battery_server *)(model->user_data))->onReadState());
    net_buf_simple_add_mem(&msg, (uint8_t []){ 0xff, 0xff, 0xff }, 3);  // time to discharge
    net_buf_simple_add_mem(&msg, (uint8_t []){ 0xff, 0xff, 0xff }, 3);  // time to charge
    net_buf_simple_add_u8(&msg, 0xff);  // flags
  }

  // APP_DBG("ttl: 0x%02x recv_dst(me): 0x%04x, addr(from): 0x%04x", ctx->recv_ttl,
  //         ctx->recv_dst, ctx->addr);
  // APP_DBG("msg len: 0x%02x: %02x %02x %02x", msg.len, msg.data[0], msg.data[1],
  //         msg.data[2]);

  ctx->send_ttl = BLE_MESH_TTL_DEFAULT;
  int err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
  if (err) {
    APP_DBG("send status failed: %d", err);
  }
}

static void generic_battery_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf) {
  APP_DBG(" ");
  generic_battery_status(model, ctx);
}

const struct bt_mesh_model_op generic_battery_server_ops[] = {
  { BLE_MESH_MODEL_OP_GEN_BATTERY_GET, 0, generic_battery_get },  // 0x8223
  BLE_MESH_MODEL_OP_END,
};

void generic_battery_status_publish(struct bt_mesh_model *model) {
  struct bt_mesh_msg_ctx ctx = {
    .net_idx = bt_mesh_app_key_find(model->pub->key)->net_idx,
    .app_idx = model->pub->key,
    .addr = model->pub->addr,
    .send_ttl = BLE_MESH_TTL_DEFAULT,
  };

  generic_battery_status(model, &ctx);
}
