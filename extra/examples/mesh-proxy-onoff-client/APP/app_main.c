#include <stdarg.h>
#include "HAL.h"
#include "MESH_LIB.h"
#include "app.h"
#include "app_mesh_config.h"
#include "config.h"
#include "ringbuffer.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

__HIGH_CODE
__attribute__((noinline)) void Main_Circulation() {
  while (1) {
    TMOS_SystemProcess();
  }
}

uint8_t bt_mesh_lib_init(void) {
  uint8_t ret;

  if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE) {
    PRINT("mesh head file error...\n");
    while (1);
  }

  ret = RF_RoleInit();

#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_OTA))
  ret = GAPRole_PeripheralInit();
#endif

  MeshTimer_Init();
  MeshDeamon_Init();
  ble_sm_alg_ecc_init();

  return ret;
}

RingBuffer txBuffer;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    ringbuffer_put(&txBuffer, *buf++, TRUE);
    if (R8_UART1_LSR & RB_LSR_TX_FIFO_EMP) {
      R8_UART1_THR = ringbuffer_get(&txBuffer);
    }
  }
  return size;
}

// log levels:
//  10: 'trace',
//  20: 'debug',
//  30: 'info',
//  40: 'warn',
//  50: 'error',
//  60: 'fatal'
void app_log(char level, const char* func, char *format, ...) {
  printf("{\"lvl\": %d", level);
  if (func) {
    printf(", \"func\": \"%s\"", func);
  }
  printf(", \"msg\": \"");
  va_list args; // Declare a variable of type va_list
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\"}\n");
}

int main(void) {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  ringbuffer_init(&txBuffer, 64);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  UART1_INTCfg(ENABLE, RB_IER_THR_EMPTY);
  PFIC_EnableIRQ(UART1_IRQn);

  CH58X_BLEInit();
  HAL_Init();
  bt_mesh_lib_init();
  App_Init();

  Main_Circulation();
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  switch (UART1_GetITFlag()) {
    case UART_II_THR_EMPTY: // trigger when THR and FIFOtx all empty
      while (ringbuffer_available(&txBuffer) && R8_UART1_TFC < UART_FIFO_SIZE) {
        R8_UART1_THR = ringbuffer_get(&txBuffer);
      }
      break;
  }
}
