#include "app.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

static uint8_t MESH_MEM[1024 * 2] = {0};
static __attribute__((aligned(4))) uint8_t dev_uuid[16];
extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

static void prov_enable(void) {
  if (bt_mesh_is_provisioned()) {
    return;
  }

  bt_mesh_scan_enable(); // Make sure we're scanning for provisioning inviations
  bt_mesh_beacon_enable(); // Enable unprovisioned beacon sending

  if (CONFIG_BLE_MESH_PB_GATT) {
    bt_mesh_proxy_prov_enable();
  }
}

static void link_open(bt_mesh_prov_bearer_t bearer) { APP_DBG(" "); }

static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason) {
  APP_DBG("reason %x", reason);

  if (!bt_mesh_is_provisioned()) {
    prov_enable();
  }
}

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index) {
  APP_DBG("net_idx %x, addr %x, iv_index %x", net_idx, addr, iv_index);
  GPIOB_SetBits(LED_UNPROVISION);
}

static void prov_reset(void) {
  APP_DBG("provision reset completed");
  GPIOB_ResetBits(LED_UNPROVISION);
  prov_enable();
}

static const struct bt_mesh_prov app_prov = {
  .uuid = dev_uuid,
  .link_open = link_open,
  .link_close = link_close,
  .complete = prov_complete,
  .reset = prov_reset,
};

void blemesh_on_sync(const struct bt_mesh_comp *app_comp) {
  mem_info_t info;
  
  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    PRINT("head file error...\n");
    while (1);
  }

  GPIOB_ModeCfg(LED_UNPROVISION, GPIO_ModeOut_PP_5mA);
  GPIOB_ResetBits(LED_UNPROVISION);

  info.base_addr = MESH_MEM;
  info.mem_len = ARRAY_SIZE(MESH_MEM);

  GetMACAddress(dev_uuid);
  int err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, dev_uuid, &info);
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
  err = bt_mesh_comp_register(app_comp);

#if (CONFIG_BLE_MESH_RELAY)
  bt_mesh_relay_init();
#endif

#if (CONFIG_BLE_MESH_PROXY || CONFIG_BLE_MESH_PB_GATT)
  #if (CONFIG_BLE_MESH_PROXY)
    bt_mesh_proxy_beacon_init_register((void *)bt_mesh_proxy_beacon_init);
    gatts_notify_register(bt_mesh_gatts_notify);
    proxy_gatt_enable_register(bt_mesh_proxy_gatt_enable);
  #endif

  #if (CONFIG_BLE_MESH_PB_GATT)
    proxy_prov_enable_register(bt_mesh_proxy_prov_enable);
  #endif

  bt_mesh_proxy_init();
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

#if ((CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_OTA))
  bt_mesh_conn_adv_init();
#endif /* PROXY || PB-GATT || OTA */

#if (CONFIG_BLE_MESH_SETTINGS)
  bt_mesh_settings_init();
#endif /* SETTINGS */

#if (CONFIG_BLE_MESH_PROXY_CLI)
  bt_mesh_proxy_cli_adapt_init();
#endif /* PROXY_CLI */

#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_PROXY_CLI) || (CONFIG_BLE_MESH_OTA))
  bt_mesh_adapt_init();
#endif /* PROXY || PB-GATT || PROXY_CLI || OTA */

  if (err) {
    APP_DBG("Initializing mesh failed (err %d)", err);
    return;
  }

  APP_DBG("Bluetooth initialized");

#if (CONFIG_BLE_MESH_SETTINGS)
  settings_load();
#endif /* SETTINGS */

  if (bt_mesh_is_provisioned()) {
    APP_DBG("Mesh network restored from flash");
  } else {
    prov_enable();
  }
  APP_DBG("Mesh initialized");
}

uint8_t bt_mesh_lib_init(void) {
  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    PRINT("mesh head file error...\n");
    while (1);
  }

  uint8_t ret = RF_RoleInit();

#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_OTA))
  ret = GAPRole_PeripheralInit();
#endif

  MeshTimer_Init();
  MeshDeamon_Init();
  ble_sm_alg_ecc_init();

  return ret;
}
