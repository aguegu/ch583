#include "config.h"
#include "iomservice.h"



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

#define IOM_DIGITALS_VALUE_POS    2

// Heart rate service
const uint8_t iomServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO), HI_UINT16(UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO)};

const uint8_t iomCharacteristicDigitalUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL), HI_UINT16(UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL)};

const uint8_t iomCharacteristicFormatUUID[ATT_BT_UUID_SIZE] = {
  LO_UINT16(GATT_CHAR_FORMAT_UUID), HI_UINT16(GATT_CHAR_FORMAT_UUID)};

const uint8_t iomNumberOfDigitalsUUID[ATT_BT_UUID_SIZE] = {
  LO_UINT16(UUID_ORG_BLUETOOTH_DESCRIPTOR_NUMBEROFDIGITALS), HI_UINT16(UUID_ORG_BLUETOOTH_DESCRIPTOR_NUMBEROFDIGITALS)};

static iomServiceCB_t iomServiceCB;

// Heart Rate Service attribute
static const gattAttrType_t iomService = { ATT_BT_UUID_SIZE, iomServUUID };

static uint8_t iomDigitalsProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;

static uint8_t digitals = 0;

static gattCharCfg_t iomDigitalsClientCharCfg[GATT_MAX_NUM_CONN];

static uint8_t digitalsFormat[] = {1, 0, 0x00, 0x27, 1, 0, 0};
static uint8_t numberOfDigitals = 2;

static gattAttribute_t iomAttrTbl[] = {
  // Heart Rate Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* gattAttrType_t */
    GATT_PERMIT_READ,                       /* permissions */
    0,                                      /* handle */
    (uint8_t *)&iomService            /* pValue */
  },
  {
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ,
    0,
    &iomDigitalsProps
  },
  {
    { ATT_BT_UUID_SIZE, iomCharacteristicDigitalUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    0,
    &digitals
  },
  {
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    0,
    (uint8_t *)&iomDigitalsClientCharCfg
  },
  {
    { ATT_BT_UUID_SIZE, iomCharacteristicFormatUUID },
    GATT_PERMIT_READ,
    0,
    digitalsFormat
  },
  {
    { ATT_BT_UUID_SIZE, iomNumberOfDigitalsUUID },
    GATT_PERMIT_READ,
    0,
    &numberOfDigitals
  }
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
  GATTServApp_InitCharCfg(INVALID_CONNHANDLE, iomDigitalsClientCharCfg);

  if (services & IOM_SERVICE) {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(iomAttrTbl,
                                         GATT_NUM_ATTRS(iomAttrTbl),
                                         GATT_MAX_ENCRYPT_KEY_SIZE,
                                         &iomCBs);
  }
  return (status);
}

extern void IOM_Register(iomServiceCB_t pfnServiceCB) {
  iomServiceCB = pfnServiceCB;
}

bStatus_t IOM_GetParameter(uint8_t param, void *value) {
  bStatus_t ret = SUCCESS;
  switch (param) {
      case IOM_DIGITALS_PARAM:
          *((uint8_t*)value) = *iomAttrTbl[IOM_DIGITALS_VALUE_POS].pValue;
          break;
      default:
          ret = INVALIDPARAMETER;
          break;
  }

  return (ret);
}

bStatus_t IOM_DigitalsNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti) {
  uint16_t value = GATTServApp_ReadCharCfg(connHandle, iomDigitalsClientCharCfg);
  PRINT("in IOM_DigitalsNotify, value: %04x\r\n", value);
  if (value & GATT_CLIENT_CFG_NOTIFY) { // If notifications enabled
    // Set the handle
    pNoti->handle = iomAttrTbl[IOM_DIGITALS_VALUE_POS].handle;
    return GATT_Notification(connHandle, pNoti, FALSE);
  }
  return bleIncorrectMode;
}

static uint8_t iom_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                    uint16_t maxLen, uint8_t method) {
  bStatus_t status = SUCCESS;

  uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
  PRINT("ReadAttrCB: uuid: 0x%04x, offset: %d *pAttr->pValue: %02x\n", uuid, offset, *pAttr->pValue);
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if (offset > 0) {
    return (ATT_ERR_ATTR_NOT_LONG);
  }

  if (uuid == UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL) {
    *pLen = 1;
    pValue[0] = *pAttr->pValue;
  } else if (uuid == UUID_ORG_BLUETOOTH_DESCRIPTOR_NUMBEROFDIGITALS) {
    *pLen = 1;
    tmos_memcpy(pValue, pAttr->pValue, *pLen);
  } else {
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return (status);
}

static bStatus_t iom_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, uint16_t offset,
                                       uint8_t method) {
  bStatus_t status = SUCCESS;
  uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
  PRINT("WriteAttrCB: uuid: 0x%04x, *pValue: %d, offset: %d, len: %d\r\n", uuid, *pValue, offset, len);
  switch (uuid) {
    case UUID_ORG_BLUETOOTH_CHARACTERISTIC_DIGITAL:
      if (len != 1) {
        status = ATT_ERR_INVALID_VALUE_SIZE;
      } else {
        *(pAttr->pValue) = pValue[0];
        (*iomServiceCB)(IOM_DIGITALS_SET);
      }
      break;
    case GATT_CLIENT_CHAR_CFG_UUID: // 0x2902
      status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                offset, GATT_CLIENT_CFG_NOTIFY);
      if (status == SUCCESS) {
        uint16_t charCfg = BUILD_UINT16(pValue[0], pValue[1]);
        PRINT("charCfg: %04x\r\n", charCfg);
        (*iomServiceCB)((charCfg == GATT_CFG_NO_OPERATION) ? IOM_DIGITALS_NOTI_DISABLED : IOM_DIGITALS_NOTI_ENABLED);
      }
      break;
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  return (status);
}

void IOM_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType) {
  // Make sure this is not loopback connection
  if (connHandle != LOOPBACK_CONNHANDLE) {
    // Reset Client Char Config if connection has dropped
    if ((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
       ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
        (!linkDB_Up(connHandle)))) {
      GATTServApp_InitCharCfg(connHandle, iomDigitalsClientCharCfg);
    }
  }
}
