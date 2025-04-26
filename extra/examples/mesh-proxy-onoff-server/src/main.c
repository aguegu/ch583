#include "app.h"

__HIGH_CODE
__attribute__((noinline)) void Main_Circulation() {
  while (1) {
    TMOS_SystemProcess();
  }
}

int main(void) {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

#ifdef DEBUG
  GPIOA_SetBits(bTXD1);
  GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
  UART1_DefInit();
#endif

  APP_DBG("%s", VER_LIB);
  APP_DBG("%s", VER_MESH_LIB);

  CH58X_BLEInit();
  HAL_Init();
  bt_mesh_lib_init();
  App_Init();
  Main_Circulation();
}
