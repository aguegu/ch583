#include "app.h"

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val) {
  if (val->cfgHdr.status) {
    APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
    return;
  }

  APP_DBG("model: 0x%04x", val->model->id)

  if (val->cfgHdr.opcode == OP_NODE_RESET) {
    APP_DBG("Provision Reset successed");
  } else if (val->cfgHdr.opcode == OP_APP_KEY_ADD) {
    APP_DBG("App Key Added");
  } else if (val->cfgHdr.opcode == OP_APP_KEY_DEL) {
    APP_DBG("App Key Deleted");
  } else if (val->cfgHdr.opcode == OP_MOD_APP_BIND) {
    APP_DBG("AppKey Binded");
  } else if (val->cfgHdr.opcode == OP_MOD_APP_UNBIND) {
    APP_DBG("AppKey Unbinded");
  } else if (val->cfgHdr.opcode == OP_MOD_SUB_ADD) {
    APP_DBG("Model Subscription Set");
  } else if (val->cfgHdr.opcode == OP_NET_TRANSMIT_SET) {
    APP_DBG("Net Transmit Set");
  } else if (val->cfgHdr.opcode == OP_MOD_PUB_SET) {
    APP_DBG("Model Publication Set");
  } else {
    APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
  }
}

static struct bt_mesh_cfg_srv cfg_srv = {
#if(CONFIG_BLE_MESH_RELAY)
  .relay = BLE_MESH_RELAY_ENABLED,
#endif
  .beacon = BLE_MESH_BEACON_ENABLED,
#if(CONFIG_BLE_MESH_FRIEND)
  .frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if (CONFIG_BLE_MESH_PROXY)
  .gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
  .default_ttl = 3,
  .net_transmit = BLE_MESH_TRANSMIT(7, 10),
  .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
  .handler = cfg_srv_rsp_handler,
};

static struct bt_mesh_health_srv health_srv;

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

uint16_t generic_onoff_cli_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t generic_onoff_cli_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

const static uint32_t periodResolutions[4] = { 100, 1000, 10000, 600000 };

uint32_t periodToMilliseconds(uint8_t period) {
  return periodResolutions[period >> 6] * (period & 0x3f);
}

int generic_onoff_cli_pub_update(struct bt_mesh_model *model) {
  APP_DBG("addr: %x", model->pub->addr);
  // APP_DBG("period: %x", model->pub->period);
  APP_DBG("period_start: %x", model->pub->period_start);

  uint32_t period = periodToMilliseconds(model->pub->period);
  APP_DBG("period: %ld ms", period);

  // model->pub->msg
  // bt_mesh_model_msg_init(model->pub->msg, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS); // 0x8204
  // net_buf_simple_add_u8(model->pub->msg, generic_onoff_server.onReadState());

  return 0;
}

BLE_MESH_MODEL_PUB_DEFINE(generic_onoff_cli_pub, generic_onoff_cli_pub_update, 12);

static struct bt_mesh_model root_models[] = {
  BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
  BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
  BLE_MESH_MODEL(BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, generic_onoff_client_ops, &generic_onoff_cli_pub, generic_onoff_cli_keys, generic_onoff_cli_groups, &generic_onoff_client),
};

static struct bt_mesh_elem elements[] = {{
    /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
  .loc = (0),
  .model_count = ARRAY_SIZE(root_models),
  .models = (root_models),
}};

const struct bt_mesh_comp app_comp = {
  .cid = 0x07D7, // WCH org id
  .elem = elements,
  .elem_count = ARRAY_SIZE(elements),
};

void App_Init() {
  // APP_DBG("%s", VER_LIB);
  // APP_DBG("%s", VER_MESH_LIB);

  CH58X_BLEInit();
  HAL_Init();
  bt_mesh_lib_init();
  blemesh_on_sync(&app_comp);
}

int genericOnoffClient_set(BOOL ackRequired) {
  int err = generic_onoff_client_set(root_models + 2, ackRequired);
  APP_DBG("err: %d", err);
  return err;
}
