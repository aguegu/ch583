#include "HAL.h"
#include "MESH_LIB.h"
#include "app.h"
#include "app_mesh_config.h"
#include "config.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

__HIGH_CODE 
__attribute__((noinline)) void Main_Circulation() {
  while (1) {
    TMOS_SystemProcess();
  }
}

uint8_t bt_mesh_lib_init(void) {
  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) ==
      FALSE) {
    PRINT("mesh head file error...\n");
    while (1)
      ;
  }
  uint8_t ret = RF_RoleInit();
  MeshTimer_Init();
  MeshDeamon_Init();
  ble_sm_alg_ecc_init();
  return ret;
}

int main(void) {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

#ifdef DEBUG
  GPIOA_SetBits(bTXD1);
  GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
  UART1_DefInit();
#endif

  APP_DBG(VER_LIB);
  APP_DBG(VER_MESH_LIB);
  APP_DBG(VER_MESH_FILE);

  CH58X_BLEInit();
  HAL_Init();
  bt_mesh_lib_init();
  App_Init();
  Main_Circulation();
}
