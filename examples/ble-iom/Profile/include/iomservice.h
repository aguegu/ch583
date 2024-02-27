#ifndef IOM_SERVICE_H
#define IOM_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO   0x1815
#define UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL 0x2A56
// #define UUID_ORG_BLUETOOTH_DESCRIPTOR_GATT_CHARACTERISTIC_PRESENTATION_FORMAT 0x2904
#define UUID_ORG_BLUETOOTH_DESCRIPTOR_NUMBEROFDIGITALS 0x2909
// Heart Rate Service bit fields
#define IOM_SERVICE                  0x00000001

// extern void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

extern bStatus_t IOM_AddService(uint32_t services);

extern void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

#ifdef __cplusplus
}
#endif

#endif
