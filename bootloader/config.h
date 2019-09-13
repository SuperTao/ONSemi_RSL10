/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
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
#define VER_ID                          "BOOTL*"
#endif    /* ifdef _DEBUG */
#ifdef NDEBUG
#define VER_ID                          "BOOTLD"
#endif    /* ifdef NDEBUG */

#define VER_MAJOR                       2
#define VER_MINOR                       0
#define VER_REVISION                    2

/* ----------------------------------------------------------------------------
 * Options
 * ------------------------------------------------------------------------- */

/*** TARG ***/
#define CFG_BUCK_COV                    0

/* Flags can be used to control if building for development board or dongle */
#define RSL10_DEV                       0
#define RSL10_DONGLE                    1
#ifndef RSL10_DEV_OR_DONGLE
#define RSL10_DEV_OR_DONGLE             RSL10_DEV
#endif

/*** GPIO ***/
#if (RSL10_DEV_OR_DONGLE == RSL10_DONGLE)
#define CFG_nUPDATE_DIO                 6
#define CFG_LED_RED_DIO                 8
#else
#define CFG_nUPDATE_DIO                 4
#define CFG_LED_RED_DIO                 9
#endif

#define DIO_ENABLE_RFID					5

/*** UART ***/
#define CFG_UART_BAUD_RATE              1000000
#if (RSL10_DEV_OR_DONGLE == RSL10_DONGLE)
#define CFG_UART_RXD_DIO                7
#define CFG_UART_TXD_DIO                10
#else
#define CFG_UART_RXD_DIO               12 
#define CFG_UART_TXD_DIO               11
#endif

/*** Updater ***/
#define CFG_TIMEOUT                     30  /* in seconds, 0 = no timeout */
#define CFG_READ_SUPPORT                0

#endif    /* _CONFIG_H */
