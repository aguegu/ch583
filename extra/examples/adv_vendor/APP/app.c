#include "config.h"
#include "HAL.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app.h"

#define APP_WAIT_ADD_APPKEY_DELAY     1600*10

#define APP_DELETE_LOCAL_NODE_DELAY   3200
// shall not less than APP_DELETE_LOCAL_NODE_DELAY
#define APP_DELETE_NODE_INFO_DELAY    3200

static uint8_t MESH_MEM[1024 * 2] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device  app_dev;

static uint8_t App_TaskID = 0; // Task ID for internal task/event processing

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static uint8_t dev_uuid[16] = {0x1c, 0x72, 0x6b, 0x4a, 0x19, 0x3c, 0x4e, 0x6e,
                               0x96, 0xb8, 0x81, 0x23, 0xbf, 0x42, 0xbc, 0x92};

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif

static void cfg_srv_rsp_handler( const cfg_srv_status_t *val );
static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason);
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index);
static void prov_reset(void);

static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val);
static int  vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len);

static struct bt_mesh_cfg_srv cfg_srv = {
  .relay = BLE_MESH_RELAY_ENABLED,
  .beacon = BLE_MESH_BEACON_ENABLED,
  .default_ttl = 3,
  .net_transmit = BLE_MESH_TRANSMIT(7, 10), // 底层发送数据重试7次，每次间隔10ms（不含内部随机数）
  .relay_retransmit = BLE_MESH_TRANSMIT(7, 10), // 底层转发数据重试7次，每次间隔10ms（不含内部随机数）
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

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};


static struct bt_mesh_model root_models[] = {
  BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
  BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

struct bt_mesh_vendor_model_srv vendor_model_srv = {
  .srv_tid.trans_tid = 0xFF,
  .handler = vendor_model_srv_rsp_handler,
};

uint16_t vnd_model_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t vnd_model_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

struct bt_mesh_model vnd_models[] = {
  BLE_MESH_MODEL_VND_CB(CID_WCH, BLE_MESH_MODEL_ID_WCH_SRV, vnd_model_srv_op, NULL, vnd_model_srv_keys,
                          vnd_model_srv_groups, &vendor_model_srv, NULL),
};

static struct bt_mesh_elem elements[] = {
  {
    .loc = (0),
    .model_count = ARRAY_SIZE(root_models),
    .models = (root_models),
    .vnd_model_count = ARRAY_SIZE(vnd_models),
    .vnd_models = (vnd_models),
  }
};

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

app_mesh_manage_t app_mesh_manage;

uint16_t delete_node_info_address=0;
uint8_t settings_loaded = FALSE;

static void prov_enable(void) {
  if (bt_mesh_is_provisioned()) return;

  bt_mesh_scan_enable(); // Make sure we're scanning for provisioning inviations
  bt_mesh_beacon_enable(); // Enable unprovisioned beacon sending

  APP_DBG("Sending Unprovisioned beacons");
}

static void link_open(bt_mesh_prov_bearer_t bearer) {
  APP_DBG("");
}

static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason) {
  APP_DBG("");
  if (reason != CLOSE_REASON_SUCCESS)
    APP_DBG("reason %x", reason);
}

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index) {
  APP_DBG("net_idx %x, addr %x", net_idx, addr);
}

static void prov_reset(void) {
  APP_DBG("provision reset completed");
  prov_enable();
}

static void cfg_srv_rsp_handler( const cfg_srv_status_t *val ) {
  if (val->cfgHdr.status) { // failed
    APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
    return;
  }

  if (val->cfgHdr.opcode == OP_NODE_RESET) {
    APP_DBG("Node Reset successed");
  } else if (val->cfgHdr.opcode == OP_APP_KEY_ADD) {
    APP_DBG("App Key Added");
  } else if (val->cfgHdr.opcode == OP_MOD_APP_BIND) {
    APP_DBG("Vendor Model Binded");
  } else if (val->cfgHdr.opcode == OP_MOD_SUB_ADD) {
    APP_DBG("Vendor Model Subscription Set");
  } else {
    APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
  }
}

static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val) {
  if (val->vendor_model_srv_Hdr.status) {
      // 有应答数据传输 超时未收到应答
      APP_DBG("Timeout opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
      return;
  }

  if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_MSG) {       // 收到透传数据
    APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.trans.len,
            val->vendor_model_srv_Event.trans.pdata[0],
            val->vendor_model_srv_Event.trans.addr);
    tmos_memcpy(&app_mesh_manage, val->vendor_model_srv_Event.trans.pdata, val->vendor_model_srv_Event.trans.len);
  } else if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_WRT) {   // 收到write数据
    APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.write.len,
            val->vendor_model_srv_Event.write.pdata[0],
            val->vendor_model_srv_Event.write.addr);
  } else if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_IND) {
    // 发送的indicate已收到应答
  } else {
    APP_DBG("Unknow opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
  }
}

static int vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len) {
  struct send_param param = {
      .app_idx = vnd_models[0].keys[0], // 此消息使用的app key，如无特定则使用第0个key
      .addr = addr,                     // 此消息发往的目的地地址
      .trans_cnt = 0x01,                // 此消息的用户层发送次数
      .period = K_MSEC(400),            // 此消息重传的间隔，建议不小于(200+50*TTL)ms，若数据较大则建议加长
      .rand = (0),                      // 此消息发送的随机延迟
      .tid = vendor_srv_tid_get(),      // tid，每个独立消息递增循环，srv使用128~191
      .send_ttl = BLE_MESH_TTL_DEFAULT, // ttl，无特定则使用默认值
  };
//    return vendor_message_srv_indicate(&param, pData, len);  // 调用自定义模型服务的有应答指示函数发送数据，默认超时2s
  return vendor_message_srv_send_trans(&param, pData, len); // 或者调用自定义模型服务的透传函数发送数据，只发送，无应答机制
}

void keyPress(uint8_t keys) {
  APP_DBG("%d", keys);
  // switch(keys) {
  //     default:
  //     {
  //         int status;
  //         uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  //         // 发往配网者节点
  //         status = vendor_model_srv_send(0x0001, data, 8);
  //         if (status)
  //         {
  //             APP_DBG("send failed %d", status);
  //         }
  //         break;
  //     }
  // }
}

void blemesh_on_sync(void) {
  int        err;
  mem_info_t info;

  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    PRINT("head file error...\n");
    while(1);
  }

  info.base_addr = MESH_MEM;
  info.mem_len = ARRAY_SIZE(MESH_MEM);

  uint8_t MacAddr[6];
  GetMACAddress(MacAddr);
  err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MacAddr, &info);

  if (err) {
    APP_DBG("Unable set configuration (err:%d)", err);
    return;
  }

  uint8_t MacAddrReverse[6];
  for (uint8_t i = 0; i < 6; i++)
    MacAddrReverse[i] = MacAddr[5 - i];
  tmos_memcpy(dev_uuid + 10, MacAddrReverse, 6);

  hal_rf_init();
  err = bt_mesh_comp_register(&app_comp);

#if (CONFIG_BLE_MESH_RELAY)
  bt_mesh_relay_init();
#endif /* RELAY  */

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
#endif /* SETTINGS */

  if (err) {
    APP_DBG("Initializing mesh failed (err %d)", err);
    return;
  }

  APP_DBG("Bluetooth initialized");

#if (CONFIG_BLE_MESH_SETTINGS)
  settings_load();
  settings_loaded = TRUE;
#endif

  if (bt_mesh_is_provisioned()) {
    APP_DBG("Mesh network restored from flash");
  } else {
    prov_enable();
  }

  APP_DBG("Mesh initialized");
}

void App_Init() {
  App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);

  vendor_model_srv_init(vnd_models);
  blemesh_on_sync();
  HAL_KeyInit();
  HalKeyConfig(keyPress);
  // tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 1600);
}

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events) {
  if (events & APP_NODE_TEST_EVT) {
    tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 2400);
    return (events ^ APP_NODE_TEST_EVT);
  }

  if (events & APP_RESET_MESH_EVENT) {
    APP_DBG("Reset mesh, delete local node");
    bt_mesh_reset();
    return (events ^ APP_RESET_MESH_EVENT);
  }

  return 0;
}
