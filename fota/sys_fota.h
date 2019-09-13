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
 * sys_fota.h
 * - Public interface to FOTA
 * ------------------------------------------------------------------------- */

#ifndef _FOTA_H
#define _FOTA_H

#include <stdint.h>

#include <sys_boot.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define SYS_FOTA_VERSION(id, mayor, minor, rev)         \
    __attribute__ ((section(".rodata.boot.version"))) \
    const Sys_Fota_version_t Sys_Boot_app_version =     \
    {                                                   \
        { id, BOOT_VER_ENCODE(mayor, minor, rev) }      \
    };

/* b2152466-d6xx-11e8-9f8b-f2801f1b9fd1 */
#define SYS_FOTA_UUID(c)                { 0xd1, 0x9f, 0x1b, 0x1f, \
                                          0x80, 0xf2, 0x8b, 0x9f, \
                                          0xe8, 0x11,  (c), 0xd6, \
                                          0x66, 0x24, 0x15, 0xb2 }

#define SYS_FOTA_DFU_SVC_UUID           SYS_FOTA_UUID(0)
#define SYS_FOTA_DFU_DATA_UUID          SYS_FOTA_UUID(1)
#define SYS_FOTA_DFU_DEVID_UUID         SYS_FOTA_UUID(2)
#define SYS_FOTA_DFU_BOOTVER_UUID       SYS_FOTA_UUID(3)
#define SYS_FOTA_DFU_STACKVER_UUID      SYS_FOTA_UUID(4)
#define SYS_FOTA_DFU_APPVER_UUID        SYS_FOTA_UUID(5)
#define SYS_FOTA_DFU_BUILDID_UUID       SYS_FOTA_UUID(6)
#define SYS_FOTA_DFU_ENTER_UUID         SYS_FOTA_UUID(7)

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef uint8_t Sys_Fota_uuid_t[16];

typedef struct
{
    Sys_Boot_app_version_t app_id;
    Sys_Fota_uuid_t dev_id;
} Sys_Fota_version_t;

/* ----------------------------------------------------------------------------
 * Function      : void Sys_Fota_StartDfu(uint32_t mode)
 * ----------------------------------------------------------------------------
 * Description   : Starts the FOTA from the application
 * Inputs        : mode     0 = from stackless application
 *                          1 = from application with BLE stack
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Sys_Fota_StartDfu(uint32_t mode);

#endif    /* _FOTA_H */
