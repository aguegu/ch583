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
//       // GATTServApp_InitCharCfg(connHandle, iomMeasClientCharCfg);
//     }
//   }
// }
