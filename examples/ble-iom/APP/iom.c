#include "config.h"
#include "iom.h"
#include "iomservice.h"

// Delay of start connect paramter update
#define DEFAULT_CONN_PARAM_UPDATE_DELAY      1600
// Fast advertising interval in 625us units
#define DEFAULT_FAST_ADV_INTERVAL            32
// Duration of fast advertising duration in (625us)
#define DEFAULT_FAST_ADV_DURATION            30000
// Slow advertising interval in 625us units
#define DEFAULT_SLOW_ADV_INTERVAL            1600
// Duration of slow advertising duration in (625us) (set to 0 for continuous advertising)
#define DEFAULT_SLOW_ADV_DURATION            0

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    20
// Maximum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    160
// Slave latency to use if parameter update request
#define DEFAULT_DESIRED_SLAVE_LATENCY        1
// Supervision timeout value (units of 10ms)
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000

static uint8_t iom_TaskID; // Task ID for internal task/event processing
static gapRole_States_t gapProfileState = GAPROLE_INIT;

static uint16_t gapConnHandle;
// Heart rate measurement value stored in this structure
static attHandleValueNoti_t iomDigitals;

static BOOL isDigitalsNotifyOn = FALSE;

static uint8_t scanRspData[] = {
  4, // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  'I', 'O', 'M'
};

static uint8_t advertData[] = {
  // flags
  0x02,
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
  // service UUIDs to notify central devices what services are included
  0x03,       // length of this data
  GAP_ADTYPE_16BIT_MORE,
  LO_UINT16(UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO),
  HI_UINT16(UUID_ORG_BLUETOOTH_SERVICE_AUTOMATIONIO),
};

static void iom_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void IOMGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void iomDigitalsNotify(void);
static void iomCB(uint8_t event);

// Bond Manager Callbacks
static gapBondCBs_t iomBondCB = {
  NULL, // Passcode callback
  NULL  // Pairing state callback
};

// GAP Role Callbacks
static gapRolesCBs_t iomPeripheralCB = {
  IOMGapStateCB, // Profile State Change Callbacks
  NULL,          // When a valid RSSI is read from controller
  NULL
};

void IOM_Init() {
  iom_TaskID = TMOS_ProcessEventRegister(IOM_ProcessEvent);

  // Setup the GAP Peripheral Role Profile
  {
    uint8_t initial_advertising_enable = TRUE;
    // Set the GAP Role Parameters
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
  }

  // Set the GAP Characteristics
  {
    uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "ble-iom";
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);  // 0x2A00
    uint16_t attAppearance = 0x0086;  // wearable computer
    GGS_SetParameter(GGS_APPEARANCE_ATT, 2, &attAppearance);  // 0x2A00
  }

  // Setup the GAP Bond Manager
  {
    uint32_t passkey = 0; // passkey "000000"
    uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8_t  mitm = FALSE;
    uint8_t  ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8_t  bonding = TRUE;
    GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
    GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
    GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
    GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
    GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
  }

  GGS_AddService(GATT_ALL_SERVICES);         // GAP
  GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes

  IOM_AddService(GATT_ALL_SERVICES);
  IOM_Register(iomCB);

  tmos_set_event(iom_TaskID, START_DEVICE_EVT);
}

uint16_t IOM_ProcessEvent(uint8_t task_id, uint16_t events) {
  PRINT("in IOM_ProcessEvent, events: %04x\r\n", events);
  if (events & SYS_EVENT_MSG) {   // 0x8000
    uint8_t *pMsg;

    if ((pMsg = tmos_msg_receive(iom_TaskID)) != NULL) {
      // iom_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
      tmos_msg_deallocate(pMsg);  // Release the TMOS message
    }
    return (events ^ SYS_EVENT_MSG);  // return unprocessed events
  }

  if (events & START_DEVICE_EVT) { // Start the Device
    GAPRole_PeripheralStartDevice(iom_TaskID, &iomBondCB, &iomPeripheralCB);
    return (events ^ START_DEVICE_EVT);
  }

  if (events & IOM_CONN_PARAM_UPDATE_EVT) {
    // Send param update.
    GAPRole_PeripheralConnParamUpdateReq(gapConnHandle,
                                         DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                         DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                         DEFAULT_DESIRED_SLAVE_LATENCY,
                                         DEFAULT_DESIRED_CONN_TIMEOUT,
                                         iom_TaskID);

    return (events ^ IOM_CONN_PARAM_UPDATE_EVT);
  }

  return 0;
}



static void iomDigitalsNotify(void) {
  iomDigitals.pValue = GATT_bm_alloc(gapConnHandle, ATT_HANDLE_VALUE_NOTI, 1, NULL, 0);

  if (iomDigitals.pValue != NULL) {
    IOM_GetParameter(IOM_DIGITALS_PARAM, iomDigitals.pValue);
    iomDigitals.len = 1;
    if (IOM_DigitalsNotify(gapConnHandle, &iomDigitals) != SUCCESS) {
      GATT_bm_free((gattMsg_t *)&iomDigitals, ATT_HANDLE_VALUE_NOTI);
    }
  }
}

static void IOMGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent) {
  if (newState == GAPROLE_CONNECTED) { // if connected
    if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
      // Get connection handle
      gapConnHandle = pEvent->linkCmpl.connectionHandle;

      // Set timer to update connection parameters
      tmos_start_task(iom_TaskID, IOM_CONN_PARAM_UPDATE_EVT, DEFAULT_CONN_PARAM_UPDATE_DELAY);
    }
  } else if (gapProfileState == GAPROLE_CONNECTED && newState != GAPROLE_CONNECTED) { // if disconnected
    uint8_t advState = TRUE;

    // reset client characteristic configuration descriptors
    IOM_HandleConnStatusCB(gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED);

    // link loss -- use fast advertising
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_FAST_ADV_INTERVAL);
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_FAST_ADV_INTERVAL);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_FAST_ADV_DURATION);

    // Enable advertising
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
  } else if (gapProfileState == GAPROLE_ADVERTISING && newState == GAPROLE_WAITING) { // if advertising stopped

    if (GAP_GetParamValue(TGAP_DISC_ADV_INT_MIN) == DEFAULT_FAST_ADV_INTERVAL) { // if fast advertising switch to slow
      uint8_t advState = TRUE;

      GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_SLOW_ADV_INTERVAL);
      GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_SLOW_ADV_INTERVAL);
      GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_SLOW_ADV_DURATION);
      GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
    }
  }

  gapProfileState = newState;
}

static void iomCB(uint8_t event) {
  PRINT("iomCB: event: %02x\r\n", event);
  if (event == IOM_DIGITALS_NOTI_ENABLED) {
    isDigitalsNotifyOn = TRUE;
  } else if (event == IOM_DIGITALS_NOTI_DISABLED) {
    isDigitalsNotifyOn = FALSE;
  } else if (event == IOM_DIGITALS_SET) {
    PRINT("call iomDigitalsNotify\r\n");
    iomDigitalsNotify();
  }
}

// static void iom_ProcessTMOSMsg(tmos_event_hdr_t *pMsg) {
//   switch(pMsg->event) {
//     default:
//       break;
//   }
// }
