/* ----------------------------------------------------------------------------
 * Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * config.h
 * - Defines of all configuration options for the BootLoader.
 * ------------------------------------------------------------------------- */

#ifndef _CONFIG_H    /* avoids multiple inclusion */
#define _CONFIG_H

/* ----------------------------------------------------------------------------
 * Version information
 * ------------------------------------------------------------------------- */

#ifdef _DEBUG
#define VER_ID                          "FOTA* "
#endif    /* ifdef _DEBUG */
#ifdef NDEBUG
#define VER_ID                          "FOTA  "
#endif    /* ifdef NDEBUG */

#define VER_MAJOR                       2
#define VER_MINOR                       4
#define VER_REVISION                    1

/* ----------------------------------------------------------------------------
 * Options
 * ------------------------------------------------------------------------- */

/*** DFU ***/
#define CFG_DEVICE_NAME                 "IH25 FOTA"
#define CFG_FOTA_SVC_UUID               SYS_FOTA_DFU_SVC_UUID
#define CFG_FALLBACK_ADDR               { 225, 173, 212, 24, 126, 51 }
#define CFG_MAX_ADVERTISING_TIME        60

#define CFG_HDLC_NB_LINKS               1
#define CFG_HDLC_T200                   0.5
#define CFG_HDLC_SDU_MAX_SIZE           2048
#define CFG_BLE_MAX_DATA_SIZE           240

/*** BLE Stack ***/
#define CFG_BLE
#define CFG_BLE_2MBPS
#define CFG_HOST
#define CFG_EMB
#define CFG_APP
#define CFG_PERIPHERAL
//#define CFG_SEC_CON
#define CFG_EXT_DB
#define CFG_RF_ATLAS

/*** BLE Profiles ***/
#define CFG_PRF
#define CFG_NB_PRF                      1

/*** RF ***/
#define CFG_RF_POWER_DBM                0

/*** Debugging ***/
#define RSL10_DEBUG                     DBG_NO    /* one of: DBG_[NO,UART,RTT] */
#define DBG_UART_BAUD_RATE              115200
#define DBG_UART_RXD_DIO                4
#define DBG_UART_TXD_DIO                5

/* configure stack */
#include <rwip_config.h>
#include <rwprf_config.h>

#endif    /* _CONFIG_H */
