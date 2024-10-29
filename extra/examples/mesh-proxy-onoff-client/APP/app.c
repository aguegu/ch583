#include "HAL.h"
#include "app.h"
#include "app_generic_onoff_client_model.h"

static uint8_t App_TaskID = 0; // Task ID for internal task/event processing

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val) {
  if (val->cfgHdr.status) {
    APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
    return;
  }

  APP_DBG("model: 0x%04x", val->model->id);

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

uint16_t gen_onoff_client_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t gen_onoff_client_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

int generic_onoff_srv_pub_update(struct bt_mesh_model *model) {
  APP_DBG("");
}

BLE_MESH_MODEL_PUB_DEFINE(generic_onoff_srv_pub, generic_onoff_srv_pub_update, 12);

BOOL readButton() {
  return !GPIOB_ReadPortPin(BTN_ONOFF);
}

void logStatus(uint16_t address, BOOL state) {
  printf("{\"level\": \"info\", \"source\": %04x, \"state\": %d}\n", address, state);
}

struct bt_mesh_generic_onoff_client generic_onoff_client = {
  .tid = 128,
  .readState = readButton,
  .onStatus = logStatus,
  .isAcked = FALSE,
  .isAckExpected = FALSE,
};

static struct bt_mesh_model root_models[] = {
  BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
  BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
  BLE_MESH_MODEL(BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, generic_onoff_client_ops, &generic_onoff_srv_pub, gen_onoff_client_keys, gen_onoff_client_groups, &generic_onoff_client),
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

void pinsInit() {
  GPIOB_ModeCfg(BTN_ONOFF, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(BTN_UNPROVISION, GPIO_ModeIN_PU);

  GPIOB_ModeCfg(LED_ONOFF, GPIO_ModeOut_PP_5mA);
  GPIOB_SetBits(LED_ONOFF);
}

void buttonsPoll() {
  static uint32_t pinResetPressedAt;
  static BOOL pinResetPressed = FALSE;
  static uint32_t buttons = BTN_ONOFF | BTN_UNPROVISION;
  uint32_t buttonsNow = GPIOB_ReadPortPin(BTN_ONOFF | BTN_UNPROVISION);

  if (buttonsNow != buttons) {
    if ((buttonsNow ^ buttons) & BTN_ONOFF) {
      // generic_onoff_client_set_unack(root_models + 2);
      generic_onoff_client_set(root_models + 2);
      tmos_start_task(App_TaskID, APP_GENERIC_ONOFF_CLIENT_ACK_EVENT, MS1_TO_SYSTEM_TIME(625));
    }

    if (((buttonsNow ^ buttons) & BTN_UNPROVISION) && !(buttonsNow & BTN_UNPROVISION) ) {
      APP_DBG("RESET pressed");
      pinResetPressed = TRUE;
      pinResetPressedAt = TMOS_GetSystemClock();
    }
    APP_DBG("buttons: %08x", buttonsNow);
  }

  if (pinResetPressed && !(buttonsNow & BTN_UNPROVISION)) {
    if (TMOS_GetSystemClock() - pinResetPressedAt > 9600) { // 9600 * 0.625 ms = 6s
      APP_DBG("duration: %d, about to self unprovision", TMOS_GetSystemClock() - pinResetPressedAt);
      tmos_set_event(App_TaskID, APP_RESET_MESH_EVENT);
      pinResetPressed = FALSE;
    }
  }

  buttons = buttonsNow;
}

void App_Init() {
  App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);
  pinsInit();

  blemesh_on_sync(&app_comp);
  tmos_set_event(App_TaskID, APP_BUTTON_POLL_EVENT); /* Kick off polling */
}

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & APP_RESET_MESH_EVENT) {
    bt_mesh_reset();
    return (events ^ APP_RESET_MESH_EVENT);
  }

  if (events & APP_BUTTON_POLL_EVENT) {
    buttonsPoll();
    tmos_start_task(App_TaskID, APP_BUTTON_POLL_EVENT, MS1_TO_SYSTEM_TIME(100));
    return events ^ APP_BUTTON_POLL_EVENT;
  }

  if (events & APP_GENERIC_ONOFF_CLIENT_ACK_EVENT) {
    if (generic_onoff_client.isAckExpected && !generic_onoff_client.isAcked) {
      APP_DBG("ACK timeout");
    }
    return events ^ APP_GENERIC_ONOFF_CLIENT_ACK_EVENT;
  }
  return 0;
}
