#ifndef IOM_SERVICE_H
#define IOM_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define IOM_SERV_UUID 0x1815

// Heart Rate Service bit fields
#define IOM_SERVICE                  0x00000001

// extern void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

extern bStatus_t IOM_AddService(uint32_t services);

#ifdef __cplusplus
}
#endif

#endif
