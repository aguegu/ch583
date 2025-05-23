#include "app.h"
#include "HAL.h"
#include "MESH_LIB.h"
#include "app_generic_onoff_client_model.h"
#include "config.h"

// LEDs are ON when pins are low
#define PIN_LED1 GPIO_Pin_18
#define PIN_LED2 GPIO_Pin_19

#define BUTTON_SWITCH GPIO_Pin_22
#define BUTTON_RESET  GPIO_Pin_4

static uint8_t MESH_MEM[1024 * 2] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8_t App_TaskID = 0;

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static __attribute__((aligned(4))) uint8_t dev_uuid[16];

static uint16_t netIndex;

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
  .net_transmit = BLE_MESH_TRANSMIT(7, 10), /* 底层发送数据重试7次，每次间隔10ms（不含内部随机数） */
  .relay_retransmit = BLE_MESH_TRANSMIT(7, 10), /* 底层转发数据重试7次，每次间隔10ms（不含内部随机数） */
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

uint16_t generic_onoff_client_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = { BLE_MESH_KEY_UNUSED };
uint16_t generic_onoff_client_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = { BLE_MESH_ADDR_UNASSIGNED };

void generic_onoff_client_rsp_handler(const generic_onoff_client_status_t *val) {
  APP_DBG("status 0x%02x", val->generic_onoff_Hdr.status);
  APP_DBG("opcode 0x%02x", val->generic_onoff_Hdr.opcode);
  if (val->generic_onoff_Hdr.opcode == BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
    if (val->generic_onoff_Hdr.status == 0xff) {
      printf("{\"level\": \"warn\",\"msg\": \"ACK Timeout\", \"source\": \"BLE_MESH_MODEL_OP_GEN_ONOFF_SET\"}\n");
    } else if (val->generic_onoff_Hdr.status == SUCCESS) {
      printf("{\"level\": \"info\",\"state\": %d, \"source\": %d}\n", val->generic_onoff_Event.status.state, val->generic_onoff_Event.status.source);
    } else {
      APP_DBG("unknown status");
    }
  }

  if (val->generic_onoff_Hdr.opcode == 0) {
    APP_DBG("incoming status report");
    printf("{\"level\": \"info\",\"state\": %d, \"source\": %d}\n", val->generic_onoff_Event.status.state, val->generic_onoff_Event.status.source);
  }
}

struct bt_mesh_generic_onoff_client generic_onoff_client = {
  .handler = generic_onoff_client_rsp_handler,
};

int generic_onoff_client_pub_update(struct bt_mesh_model *model) { APP_DBG(""); }

BLE_MESH_MODEL_PUB_DEFINE(generic_onoff_client_pub, generic_onoff_client_pub_update, 12);

static struct bt_mesh_model root_models[] = {
  BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
  BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
  BLE_MESH_MODEL(BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, generic_onoff_client_ops, &generic_onoff_client_pub, generic_onoff_client_keys, generic_onoff_client_groups, &generic_onoff_client),
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
  GPIOB_SetBits(PIN_LED2);
  netIndex = net_idx;
}

static void prov_reset(void) {
  APP_DBG("provision reset completed");
  GPIOB_ResetBits(PIN_LED2);
  prov_enable();
}

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val) {
  if (val->cfgHdr.status) {
    APP_DBG("warning opcode 0x%02x, status: %02x", val->cfgHdr.opcode, val->cfgHdr.status);
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
  } else if (val->cfgHdr.opcode == OP_MOD_SUB_ADD) {
    APP_DBG("Model Subscription Set");
  } else if (val->cfgHdr.opcode == OP_MOD_PUB_SET) {
    APP_DBG("Model Publication Set");
    APP_DBG("addr: 0x%04x", generic_onoff_client_pub.addr);
    APP_DBG("key: 0x%04x", generic_onoff_client_pub.key);
    APP_DBG("cred: 0x%02x", generic_onoff_client_pub.cred);
    APP_DBG("send_rel: 0x%02x", generic_onoff_client_pub.send_rel);

    APP_DBG("ttl: 0x%02x", generic_onoff_client_pub.ttl);
    APP_DBG("retransmit: 0x%02x", generic_onoff_client_pub.retransmit);
    APP_DBG("period: 0x%02x", generic_onoff_client_pub.period);
    APP_DBG("period_div: 0x%02x", generic_onoff_client_pub.period_div);
    APP_DBG("fast_period: 0x%02x", generic_onoff_client_pub.fast_period);
    APP_DBG("count: 0x%02x", generic_onoff_client_pub.count);
    APP_DBG("period_start: 0x%08lx", generic_onoff_client_pub.period_start);
  } else {
    APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
  }
}



void blemesh_on_sync(void) {
  int err;
  mem_info_t info;
  uint8_t flashMac[6];

  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    APP_DBG("head file error...");
    while (1);
  }

  info.base_addr = MESH_MEM;
  info.mem_len = ARRAY_SIZE(MESH_MEM);

  GetMACAddress(flashMac);
  err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, flashMac, &info);
  if (err) {
    APP_DBG("Unable set configuration (err:%d)", err);
    return;
  }

  for (uint8_t i = 0; i < 6; i++)
    dev_uuid[15 - i] = flashMac[i];

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
  GPIOB_ModeCfg(BUTTON_SWITCH, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(BUTTON_RESET, GPIO_ModeIN_PU);

  GPIOB_ModeCfg(PIN_LED1, GPIO_ModeOut_PP_5mA);
  GPIOB_ModeCfg(PIN_LED2, GPIO_ModeOut_PP_5mA);

  GPIOB_SetBits(PIN_LED1);
  GPIOB_ResetBits(PIN_LED2);
}

void buttonsPoll() {
  static uint32_t pinResetPressedAt;
  static BOOL pinResetPressed = FALSE;
  static uint32_t buttons = BUTTON_SWITCH | BUTTON_RESET;
  uint32_t buttonsNow = GPIOB_ReadPortPin(BUTTON_SWITCH | BUTTON_RESET);

  if (buttonsNow != buttons) {
    if ((buttonsNow ^ buttons) & BUTTON_SWITCH) {
      struct bt_mesh_generic_onoff_set_val param = {
        .op_en = FALSE,
        .onoff = !(buttonsNow & BUTTON_SWITCH),
        .tid = client_tid_get(),
        .trans_time = 0,
        .delay = 0,
      };

      // int err = bt_mesh_generic_onoff_set_unack(netIndex, generic_onoff_client_pub.key, generic_onoff_client_pub.addr, &param);
      int err = bt_mesh_generic_onoff_set(netIndex, generic_onoff_client_pub.key, generic_onoff_client_pub.addr, &param);
      if (err) {
        APP_DBG("send failed %d", err);
      }
    }

    if (((buttonsNow ^ buttons) & BUTTON_RESET) && !(buttonsNow & BUTTON_RESET) ) {
      APP_DBG("RESET pressed");
      pinResetPressed = TRUE;
      pinResetPressedAt = TMOS_GetSystemClock();
    }
    APP_DBG("buttons: %08x", buttonsNow);
  }

  if (pinResetPressed && !(buttonsNow & BUTTON_RESET)) {
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

  generic_onoff_client_init(&root_models[2]);
  blemesh_on_sync();

  tmos_start_task(App_TaskID, APP_BUTTON_POLL_EVENT, 0);
}

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & APP_RESET_MESH_EVENT) { // 收到删除命令，删除自身网络信息
    APP_DBG("Reset mesh, delete local node");
    bt_mesh_reset();
    return (events ^ APP_RESET_MESH_EVENT);
  }

  if (events & APP_BUTTON_POLL_EVENT) {
    buttonsPoll();
    tmos_start_task(App_TaskID, APP_BUTTON_POLL_EVENT, MS1_TO_SYSTEM_TIME(100));
    return events ^ APP_BUTTON_POLL_EVENT;
  }
  return 0;
}
