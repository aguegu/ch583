#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_KEY_POLLING_VALUE    100

#define HAL_KEY_SW_1             0x01  // key1
#define HAL_KEY_SW_2             0x02  // key2
#define HAL_KEY_SW_3             0x04  // key3
#define HAL_KEY_SW_4             0x08  // key4

#define KEY1_BV                  BV(22)
#define KEY2_BV                  BV(4)
#define KEY3_BV                  ()
#define KEY4_BV                  ()

#define KEY1_PU                  (R32_PB_PU |= KEY1_BV)
#define KEY2_PU                  (R32_PB_PU |= KEY2_BV)
#define KEY3_PU                  ()
#define KEY4_PU                  ()

#define KEY1_DIR                 (R32_PB_DIR &= ~KEY1_BV)
#define KEY2_DIR                 (R32_PB_DIR &= ~KEY2_BV)
#define KEY3_DIR                 ()
#define KEY4_DIR                 ()

#define KEY1_IN                  (ACTIVE_LOW(R32_PB_PIN & KEY1_BV))
#define KEY2_IN                  (ACTIVE_LOW(R32_PB_PIN & KEY2_BV))
#define KEY3_IN                  ()
#define KEY4_IN                  ()

#define HAL_PUSH_BUTTON1()       (KEY1_IN)
#define HAL_PUSH_BUTTON2()       (KEY2_IN)
#define HAL_PUSH_BUTTON3()       (0)
#define HAL_PUSH_BUTTON4()       (0)

typedef struct {
  uint8_t current;
  uint8_t changed;
} HalKeyChangeEvent;

typedef void (*HalKeyCallback)(HalKeyChangeEvent status);

void HAL_KeyInit(void);
void HAL_KeyPoll(void);
void HAL_KeyConfig(const HalKeyCallback cback);
uint8_t HAL_KeyRead(void);

#ifdef __cplusplus
}
#endif

#endif
