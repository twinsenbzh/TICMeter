/**
 * @file tuya.h
 * @author Dorian Benech
 * @brief
 * @version 1.0
 * @date 2023-10-11
 *
 * @copyright Copyright (c) 2023 GammaTroniques
 *
 */

#ifndef TUYA_H
#define TUYA_H
/*==============================================================================
 Local Include
===============================================================================*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "linky.h"
#include "tuya_iot.h"

/*==============================================================================
 Public Defines
==============================================================================*/

/*==============================================================================
 Public Macro
==============================================================================*/

/*==============================================================================
 Public Type
==============================================================================*/

/*==============================================================================
 Public Variables Declaration
==============================================================================*/
extern TaskHandle_t tuyaTaskHandle;

/*==============================================================================
 Public Functions Declaration
==============================================================================*/
extern void tuya_reset();
extern void tuya_pairing_task(void *pvParameters);
extern void tuya_init();
extern uint8_t tuya_stop();
extern uint8_t tuya_send_data(LinkyData *linky);
extern uint8_t tuya_wait_event(tuya_event_id_t event, uint32_t timeout);

#endif