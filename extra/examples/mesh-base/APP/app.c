#include "MESH_LIB.h"
#include "config.h"
#include "HAL.h"
#include "app.h"

// LEDs are ON when pins are low
#define PIN_UNPROVISION GPIO_Pin_19
#define BUTTON_UNPROVISION  GPIO_Pin_4

static uint8_t MESH_MEM[1024 * 2] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8_t App_TaskID = 0;

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static __attribute__((aligned(4))) uint8_t dev_uuid[16];

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val);
static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason);
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index);
static void prov_reset(void);

static struct bt_mesh_cfg_srv cfg_srv = {
  .relay = BLE_MESH_RELAY_ENABLED,
  .beacon = BLE_MESH_BEACON_ENABLED,
  .default_ttl = 3,
  .net_transmit = BLE_MESH_TRANSMIT(7, 10),
  .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
  .handler = cfg_srv_rsp_handler,
};

void app_prov_attn_on(struct bt_mesh_model *model) {
  APP_DBG("app_prov_attn_on");
}

void app_prov_attn_off(struct bt_mesh_model *model) {
  APP_DBG("app_prov_attn_off");
}

const struct bt_mesh_health_srv_cb health_srv_cb = {
  .attn_on = app_prov_attn_on,
  .attn_off = app_prov_attn_off,
};

static struct bt_mesh_health_srv health_srv = {
  .cb = &health_srv_cb,
};

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

static struct bt_mesh_model root_models[] = {
  BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
  BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

static struct bt_mesh_elem elements[] = {{
  /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
  .loc = (0),
  .model_count = ARRAY_SIZE(root_models),
  .models = (root_models),
}};

const struct bt_mesh_comp app_comp = {
  .cid = 0x07D7, // WCH 公司id
  .elem = elements,
  .elem_count = ARRAY_SIZE(elements),
};

static const struct bt_mesh_prov app_prov = {
  .uuid = dev_uuid,
  .link_open = link_open,
  .link_close = link_close,
  .complete = prov_complete,
  .reset = prov_reset,
};

static void prov_enable(void) {
  if (bt_mesh_is_provisioned()) {
    return;
  }

  bt_mesh_scan_enable();   // Make sure we're scanning for provisioning inviations
  bt_mesh_beacon_enable(); // Enable unprovisioned beacon sending
  APP_DBG("Sending Unprovisioned beacons");
}

static void link_open(bt_mesh_prov_bearer_t bearer) {
  APP_DBG("bearer: %x", bearer);
}

static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason) {
  APP_DBG("bearer: %x", bearer);
  if (reason != CLOSE_REASON_SUCCESS)
    APP_DBG("reason %x", reason);
}

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index) {
  APP_DBG("net_idx %x, addr %x", net_idx, addr);
  GPIOB_SetBits(PIN_UNPROVISION);
}

static void prov_reset(void) {
  APP_DBG("provision reset completed");
  GPIOB_ResetBits(PIN_UNPROVISION);
  prov_enable();
}

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val) {
  if (val->cfgHdr.status) {
    APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
    return;
  }

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
  } else {
    APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
  }
}

void blemesh_on_sync(void) {
  int err;
  mem_info_t info;

  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    PRINT("head file error...\n");
    while (1);
  }

  info.base_addr = MESH_MEM;
  info.mem_len = ARRAY_SIZE(MESH_MEM);

  GetMACAddress(dev_uuid);
  err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, dev_uuid, &info);
  if (err) {
    APP_DBG("Unable set configuration (err:%d)", err);
    return;
  }

  for (uint8_t i = 0; i < 6; i++)
    dev_uuid[15 - i] = dev_uuid[i];

  FLASH_EEPROM_CMD(CMD_GET_UNIQUE_ID, 0, dev_uuid, 0);
  dev_uuid[9] = dev_uuid[6];
  dev_uuid[8] = R8_CHIP_ID; // 0x83 for ch583
  dev_uuid[6] = 'G';
  // https://git.kernel.org/pub/scm/libs/ell/ell.git/commit/?id=718d7ef1acb75bd171474a45801dacf43b67d3fe

  hal_rf_init();
  err = bt_mesh_comp_register(&app_comp);

#if (CONFIG_BLE_MESH_RELAY)
  bt_mesh_relay_init();
#endif

  bt_mesh_prov_retransmit_init();

#if (!CONFIG_BLE_MESH_PB_GATT)
  adv_link_rx_buf_register(&rx_buf);
#endif

  err = bt_mesh_prov_init(&app_prov);

  bt_mesh_mod_init();
  bt_mesh_net_init();
  bt_mesh_trans_init();
  bt_mesh_beacon_init();

  bt_mesh_adv_init();

#if (CONFIG_BLE_MESH_SETTINGS)
  bt_mesh_settings_init();
#endif

  if (err) {
    APP_DBG("Initializing mesh failed (err %d)", err);
    return;
  }

  APP_DBG("Bluetooth initialized");

#if (CONFIG_BLE_MESH_SETTINGS)
  settings_load();
#endif

  if (bt_mesh_is_provisioned()) {
    APP_DBG("Provisioned. Mesh network restored from flash");
  } else {
    prov_enable();
  }

  APP_DBG("Mesh initialized");
}

void pinsInit() {
  GPIOB_ModeCfg(BUTTON_UNPROVISION, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(PIN_UNPROVISION, GPIO_ModeOut_PP_5mA);
  GPIOB_ResetBits(PIN_UNPROVISION);
}

void buttonsPoll() {
  static uint32_t pinResetPressedAt;
  static BOOL pinResetPressed = FALSE;
  static uint32_t buttons = BUTTON_UNPROVISION;
  uint32_t buttonsNow = GPIOB_ReadPortPin(BUTTON_UNPROVISION);

  if (buttonsNow != buttons) {
    if (((buttonsNow ^ buttons) & BUTTON_UNPROVISION) && !(buttonsNow & BUTTON_UNPROVISION) ) {
      APP_DBG("RESET pressed");
      pinResetPressed = TRUE;
      pinResetPressedAt = TMOS_GetSystemClock();
    }
    APP_DBG("buttons: %08x", buttonsNow);
  }

  if (pinResetPressed && !(buttonsNow & BUTTON_UNPROVISION)) {
    if (TMOS_GetSystemClock() - pinResetPressedAt > 9600) { // 9600 * 0.625 ms = 6s
      APP_DBG("duration: %d, about to self unprovision", TMOS_GetSystemClock() - pinResetPressedAt);
      tmos_start_task(App_TaskID, APP_RESET_MESH_EVENT, 160);
      pinResetPressed = FALSE;
    }
  }

  buttons = buttonsNow;
}

void App_Init() {
  App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);
  pinsInit();

  blemesh_on_sync();
  tmos_set_event(App_TaskID, APP_BUTTON_POLL_EVENT); /* Kick off polling */
}

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & APP_RESET_MESH_EVENT) {
    APP_DBG("Reset mesh, delete local node");
    bt_mesh_reset();
    return (events ^ APP_RESET_MESH_EVENT);
  }

  if (events & APP_BUTTON_POLL_EVENT) {
    buttonsPoll();
    tmos_start_task(App_TaskID, APP_BUTTON_POLL_EVENT, MS1_TO_SYSTEM_TIME(500));
    return events ^ APP_BUTTON_POLL_EVENT;
  }
  return 0;
}
