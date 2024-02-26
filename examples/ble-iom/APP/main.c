#include "config.h"
#include "HAL.h"
#include "iom.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4]; // referenced in HAL/MCU.c

__HIGH_CODE
__attribute__((noinline))
void Main_Circulation() {
  while(1) {
    TMOS_SystemProcess(); // in BLE_LIB
  }
}

int main(void) {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  #ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
  #endif

  PRINT("%s\n", VER_LIB);
  CH58X_BLEInit();  // in HAL/MCU.c
  HAL_Init();       // in HAL/MCU.c
  GAPRole_PeripheralInit(); // BLE_LIB

  IOM_Init(); // iom.c

  Main_Circulation();
}
