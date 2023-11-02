#include "CH58x_common.h"

uint8_t TxBuff[] = "Hello, world.\r\n";
uint8_t RxBuff[100];

int main() {
  uint8_t len;
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpoll, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  // UART1_BaudRateCfg(9600); // uncomment if prefer other baudrate

  UART1_SendString(TxBuff, sizeof(TxBuff) - 1); // sizeof would count the tailing '\0'

  while(1) {
    len = UART1_RecvString(RxBuff);
    if (len) {
      UART1_SendString(RxBuff, len);
    }
  }
}
