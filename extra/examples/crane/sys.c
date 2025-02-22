#include "sys.h"

#define TASK_LEN (5)

static Task taskList[TASK_LEN];

static volatile uint32_t jiffies = 0;

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    } else {
      __WFI();
      __nop();
      __nop();
    }
  }
}

void registerTask(uint8_t index, TaskFunction task, unsigned int period, unsigned int delay) {
  taskList[index].task = task;
  taskList[index].period = period;
  taskList[index].delay = delay;
}

void dispatchTasks(void) {
  while (1) {
    BOOL idle = TRUE;
    for (uint8_t i = 0; i < TASK_LEN; i++) {
      if (taskList[i].ready) {
        taskList[i].ready = FALSE;
        idle = FALSE;
        taskList[i].task();
      }
    }
    if (idle) {
      __WFI();
      __nop();
      __nop();
    }
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;

  static volatile uint8_t i;
  for (i = 0; i < TASK_LEN; i++) {
    if (taskList[i].delay == 0) {
      taskList[i].ready = TRUE;
      taskList[i].delay = taskList[i].period - 1;
    } else {
      taskList[i].delay--;
    }
  }
}
