#ifndef __SYS_H
#define __SYS_H

#include "CH58x_common.h"

typedef void (*TaskFunction)(void);

typedef struct {
  TaskFunction task;
  uint32_t period;
  uint32_t delay;
  BOOL ready;
} Task;

void registerTask(uint8_t index, TaskFunction task, unsigned int period, unsigned int delay);

void dispatchTasks(void);

void delayInJiffy(uint32_t t);
uint32_t getJiffies();

#endif
