#ifndef IOM_H
#define IOM_H

#ifdef __cplusplus
extern "C" {
#endif

#define START_DEVICE_EVT      0x0001
#define IOM_PARAM_UPDATE_EVT  0x0002

extern void IOM_Init(void);

extern uint16_t IOM_ProcessEvent(uint8_t task_id, uint16_t events);

#ifdef __cplusplus
}
#endif

#endif
