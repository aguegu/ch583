#include <stdio.h>
#include "CH58x_common.h"
#include <math.h>

volatile uint32_t jiffies = 0;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    while (R8_UART1_TFC == UART_FIFO_SIZE);
    R8_UART1_THR = *buf++;
  }
  return size;
}

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

__HIGH_CODE
float Q_rsqrt(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking（邪恶的浮点数位运算黑科技）
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?（这是什么鬼？）
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration （第一次迭代）
  // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed（第二次迭代，可以删除）
	return y;
}

__HIGH_CODE
float Q_rsqrt_2(float number) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking（邪恶的浮点数位运算黑科技）
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?（这是什么鬼？）
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration （第一次迭代）
  y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed（第二次迭代，可以删除）
	return y;
}


int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);
  SysTick_Config(GetSysClock() / 1000000); // 1000000Hz

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD: PA9, pushpull, but set it high beforehand
  UART1_DefInit();  // default baudrate 115200

  while(1) {

    float s0 = 0, s1 = 0, s2 = 0, s3 = 0;

    uint32_t t0 = jiffies;
    for (uint16_t i = 1; i < 251; i++) {
      s0 += 948683.2980505138 / sqrtf(i);
    }

    uint32_t t1 = jiffies;
    for (uint16_t i = 1; i < 251; i++) {
      s1 += 948683.2980505138 / sqrt(i);
    }
    uint32_t t2 = jiffies;

    for (uint16_t i = 1; i < 251; i++) {
      s2 += 948683.2980505138 * Q_rsqrt(i);
    }
    uint32_t t3 = jiffies;

    for (uint16_t i = 1; i < 251; i++) {
      s3 += 948683.2980505138 * Q_rsqrt_2(i);
    }
    uint32_t t4 = jiffies;

    printf("sqrtf, sqrt, Q_rsprt, Q_rsqrt_2\r\n");
    printf("%f, %f, %f, %f\r\n", s0, s1, s2, s3);
    printf("%ld, %ld, %ld, %ld\r\n\r\n", t1-t0, t2-t1, t3-t2, t4-t3);

    delayInJiffy(60);
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}
