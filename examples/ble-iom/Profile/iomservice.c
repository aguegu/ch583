#include "config.h"
#include "iomservice.h"

// static gattCharCfg_t iomMeasClientCharCfg[GATT_MAX_NUM_CONN];

// void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType) {
//   // Make sure this is not loopback connection
//   if (connHandle != LOOPBACK_CONNHANDLE) {
//     // Reset Client Char Config if connection has dropped
//     if ((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
//        ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
//         (!linkDB_Up(connHandle)))) {
//    // GATTServApp_InitCharCfg(connHandle, iomMeasClientCharCfg);
//     }
//   }
// }

// Heart rate service
const uint8_t iomServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(IOM_SERV_UUID), HI_UINT16(IOM_SERV_UUID)};

// Heart Rate Service attribute
static const gattAttrType_t iomService = { ATT_BT_UUID_SIZE, iomServUUID };

static gattAttribute_t iomAttrTbl[] = {
  // Heart Rate Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* gattAttrType_t */
    GATT_PERMIT_READ,                       /* permissions */
    0,                                      /* handle */
    (uint8_t *)&iomService            /* pValue */
  },
};

static uint8_t   iom_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                      uint16_t maxLen, uint8_t method);
static bStatus_t iom_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, uint16_t offset,
                                       uint8_t method);

gattServiceCBs_t iomCBs = {
  iom_ReadAttrCB,  // Read callback function pointer
  iom_WriteAttrCB, // Write callback function pointer
  NULL                   // Authorization callback function pointer
};


bStatus_t IOM_AddService(uint32_t services) {
  uint8_t status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  // GATTServApp_InitCharCfg(INVALID_CONNHANDLE, iomMeasClientCharCfg);

  if (services & IOM_SERVICE) {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(iomAttrTbl,
                                         GATT_NUM_ATTRS(iomAttrTbl),
                                         GATT_MAX_ENCRYPT_KEY_SIZE,
                                         &iomCBs);
  }

  return (status);
}

static uint8_t iom_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                    uint16_t maxLen, uint8_t method) {
  bStatus_t status = SUCCESS;

  uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
  PRINT("ReadAttrCB: uuid: 0x%04x, offset: %d\n", uuid, offset);
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if (offset > 0) {
    return (ATT_ERR_ATTR_NOT_LONG);
  }

  // if (uuid == BODY_SENSOR_LOC_UUID) {
  //   *pLen = 1;
  //   pValue[0] = *pAttr->pValue;
  // } else {
    status = ATT_ERR_ATTR_NOT_FOUND;
  // }

  return (status);
}

static bStatus_t iom_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, uint16_t offset,
                                       uint8_t method) {
  bStatus_t status = SUCCESS;
  uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
  PRINT("WriteAttrCB: uuid: 0x%04x, *pValue: %d, offset: %d, len: %d\n", uuid, *pValue, offset, len);
  switch (uuid) {
    // case HEARTRATE_CTRL_PT_UUID:  // 0x2A39, https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.heart_rate_control_point.xml
    //   if (offset > 0) {
    //     status = ATT_ERR_ATTR_NOT_LONG;
    //   } else if (len != 1) {
    //     status = ATT_ERR_INVALID_VALUE_SIZE;
    //   } else if (*pValue != HEARTRATE_COMMAND_ENERGY_EXP) { // 0x01
    //     status = HEARTRATE_ERR_NOT_SUP;
    //   } else {
    //     *(pAttr->pValue) = pValue[0];
    //     (*heartRateServiceCB)(HEARTRATE_COMMAND_SET);
    //   }
    //   break;
    // case GATT_CLIENT_CHAR_CFG_UUID: // 0x2902
    //   status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
    //                                             offset, GATT_CLIENT_CFG_NOTIFY);
    //   if (status == SUCCESS) {
    //     uint16_t charCfg = BUILD_UINT16(pValue[0], pValue[1]);
    //     (*heartRateServiceCB)((charCfg == GATT_CFG_NO_OPERATION) ? HEARTRATE_MEAS_NOTI_DISABLED : HEARTRATE_MEAS_NOTI_ENABLED);
    //   }
    //   break;
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  return (status);
}
