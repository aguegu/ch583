#ifndef IOM_SERVICE_H
#define IOM_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define IOM_DIGITALS_PARAM  0

#define UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO   0x1815
#define UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL 0x2A56
// #define UUID_ORG_BLUETOOTH_DESCRIPTOR_GATT_CHARACTERISTIC_PRESENTATION_FORMAT 0x2904
#define UUID_ORG_BLUETOOTH_DESCRIPTOR_NUMBEROFDIGITALS 0x2909
// Heart Rate Service bit fields
#define IOM_SERVICE                  0x00000001

#define IOM_DIGITALS_NOTI_ENABLED            1
#define IOM_DIGITALS_NOTI_DISABLED           2
#define IOM_DIGITALS_SET                     3

typedef void (*iomServiceCB_t)(uint8_t event);

// extern void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

extern bStatus_t IOM_AddService(uint32_t services);

extern void IOM_Register(iomServiceCB_t pfnServiceCB);

extern bStatus_t IOM_GetParameter(uint8_t param, void *value);

extern bStatus_t IOM_DigitalsNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti);

extern void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);


#ifdef __cplusplus
}
#endif

#endif
