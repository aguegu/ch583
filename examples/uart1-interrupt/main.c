#include "CH58x_common.h"

uint8_t TxBuff[] = "Hello, world.\r\n";
uint8_t RxBuff[100];
uint8_t trigB;

int main() {
  uint8_t len;
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD: PA8, in with pullup
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200
  // UART1_BaudRateCfg(9600); // uncomment if prefer other baudrate

  UART1_SendString(TxBuff, sizeof(TxBuff) - 1);

  UART1_ByteTrigCfg(UART_7BYTE_TRIG);
  trigB = 7;
  UART1_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
  PFIC_EnableIRQ(UART1_IRQn);

  while(1);
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler(void) {
  volatile uint8_t i;
  switch (UART1_GetITFlag()) {
    case UART_II_LINE_STAT: // 线路状态错误
      UART1_GetLinSTA();
      break;
    case UART_II_RECV_RDY: // 数据达到设置触发点
      for (i = 0; i != trigB; i++)         {
        RxBuff[i] = UART1_RecvByte();
        UART1_SendByte(RxBuff[i]);
      }
      break;
    case UART_II_RECV_TOUT: // 接收超时，暂时一帧数据接收完成
      i = UART1_RecvString(RxBuff);
      UART1_SendString(RxBuff, i);
      break;
    case UART_II_THR_EMPTY: // 发送缓存区空，可继续发送
      break;
    case UART_II_MODEM_CHG: // 只支持串口0
      break;
    default:
      break;
  }
}
