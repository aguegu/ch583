/********************************** (C) COPYRIGHT *******************************
 * File Name          : HAL.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2016/05/05
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __HAL_H
#define __HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "RTC.h"

/* hal task Event */
#define HAL_REG_INIT_EVENT    0x2000

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern tmosTaskID halTaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/**
 * @brief   硬件初始化
 */
extern void HAL_Init(void);

/**
 * @brief   硬件层事务处理
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 */
extern tmosEvents HAL_ProcessEvent(tmosTaskID task_id, tmosEvents events);

/**
 * @brief   BLE 库初始化
 */
extern void CH58X_BLEInit(void);

/**
 * @brief   获取内部温感采样值，如果使用了ADC中断采样，需在此函数中暂时屏蔽中断.
 *
 * @return  内部温感采样值.
 */
extern uint16_t HAL_GetInterTempValue(void);

/**
 * @brief   内部32k校准
 */
extern void Lib_Calibration_LSI(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
