/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* sys.boot.h
* - Interface to the Bootloader.
* -------------------------------------------------------------------------- */
#ifndef _SYS_BOOT_H    /* avoids multiple inclusion */
#define _SYS_BOOT_H

#include <stdint.h>
#include <rsl10.h>
#include <rsl10_flash.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define BOOT_BASE_ADR             FLASH_MAIN_BASE
#define BOOT_MAX_SIZE             (4 * FLASH_SECTOR_SIZE)

#define APP_BASE_ADR              (BOOT_BASE_ADR + BOOT_MAX_SIZE)
#define APP_RED_SIZE              (2 * FLASH_SECTOR_SIZE)

/* Exclude the two custom redundancy sectors from application area */
#define APP_MAX_SIZE              (FLASH_MAIN_SIZE - BOOT_MAX_SIZE - APP_RED_SIZE)
#define APP_MIN_SIZE              (FLASH_SECTOR_SIZE / 2)
#define APP_SIG_SIZE              64

#define BOOT_VER_ENCODE(m, n, r)  (((m) << 12) | ((n) << 8) | (r))
#define BOOT_VER_DECODE(num)      ((num >> 12) & 0xF), ((num >> 8) & 0xF), (num & 0xFF)

#define BOOTVECT_GET_VERSION(a)   ((a) + 0x1C)
#define BOOTVECT_GET_DSCR(a)      ((a) + 0x20)
#define BOOTVECT_GET_UPD(a)       ((a) + 0x20)
#define BOOTVECT_GET_NEXT(a)      ((a) + 0x24)

#define SYS_BOOT_VERSION(id, mayor, minor, rev)         \
    __attribute__ ((section(".rodata.boot.version"))) \
    const Sys_Boot_app_version_t Sys_Boot_app_version = \
    {                                                   \
        id, BOOT_VER_ENCODE(mayor, minor, rev)          \
    };

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef char Sys_Boot_app_id_t[6];

typedef struct
{
    Sys_Boot_app_id_t id;   /* App ID string */
    uint16_t num;           /* format: <major[15:12]>.<minor[11:8]>.<revision[7:0]> */
} Sys_Boot_app_version_t;

typedef struct
{
    uint32_t image_size;    /* image size of App */
    uint32_t build_id_a[8]; /* FOTA build ID */
} Sys_Boot_descriptor_t;

/* ----------------------------------------------------------------------------
 * Function      : static inline void Sys_Boot_StartUpdater(void)
 * ----------------------------------------------------------------------------
 * Description   : Starts the Updater.
 *
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static inline void Sys_Boot_StartUpdater(void)
{
    uint_fast32_t vector = *(const uint32_t *)BOOTVECT_GET_UPD(BOOT_BASE_ADR);

    if (vector > BOOT_BASE_ADR && vector < APP_BASE_ADR)
    {
        ((void (*)(void))vector)();
    }
}

/* ----------------------------------------------------------------------------
 * Function      : static inline const Sys_app_version_t *
 *                                Sys_Boot_GetVersion(uint_fast32_t image_adr)
 * ----------------------------------------------------------------------------
 * Description   : Gets a pointer to the image version.
 * Inputs        : image_adr        - image start address
 * Outputs       : return value     - pointer to version info
 *                                  - NULL for no valid version
 * Assumptions   : at image_adr is a valid image
 * ------------------------------------------------------------------------- */
static inline const Sys_Boot_app_version_t * Sys_Boot_GetVersion(uint_fast32_t image_adr)
{
    if (image_adr >= BOOT_BASE_ADR && image_adr < FLASH_MAIN_TOP)
    {
        uint_fast32_t vector = *(const uint32_t *)BOOTVECT_GET_VERSION(image_adr);

        if (vector > image_adr && vector < FLASH_MAIN_TOP)
        {
            return (const Sys_Boot_app_version_t *)vector;
        }
    }
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Function      : static inline const Sys_Boot_descriptor_t *
 *                                  Sys_Boot_GetDscr(uint_fast32_t image_adr)
 * ----------------------------------------------------------------------------
 * Description   : Gets the the BootLoader descriptor if available.
 * Inputs        : image_adr        - image start address
 * Outputs       : return value     - pointer to BootLoader descriptor
 *                                  - NULL for no descriptor
 * Assumptions   : at image_adr is a valid image
 * ------------------------------------------------------------------------- */
static inline const Sys_Boot_descriptor_t * Sys_Boot_GetDscr(uint_fast32_t image_adr)
{
    if (image_adr >= BOOT_BASE_ADR && image_adr < FLASH_MAIN_TOP)
    {
        uint_fast32_t vector = *(const uint32_t *)BOOTVECT_GET_DSCR(image_adr);

        if (vector > image_adr && vector < image_adr + FLASH_SECTOR_SIZE)
        {
            return (const Sys_Boot_descriptor_t *)vector;
        }
    }
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Function      : static inline uint_fast32_t Sys_Boot_GetImageSize(
 *                                          const Sys_Boot_descriptor_t *dscr_p)
 * ----------------------------------------------------------------------------
 * Description   : Gets the aligned image size if available.
 * Inputs        : dscr_p           - pointer to BootLoader descriptor
 * Outputs       : return value     - image size
 *                                  - 0 for no image size found
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static inline uint_fast32_t Sys_Boot_GetImageSize(const Sys_Boot_descriptor_t *dscr_p)
{
    if (dscr_p != NULL)
    {
        return dscr_p->image_size;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Function      : static inline uint_fast32_t Sys_Boot_GetNextImage(
 *                                                  uint_fast32_t image_adr)
 * ----------------------------------------------------------------------------
 * Description   : Gets the start address of the next image if available.
 * Inputs        : image_adr        - image start address
 * Outputs       : return value     - next image start address
 *                                  - 0 for no next image found
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static inline uint_fast32_t Sys_Boot_GetNextImage(uint_fast32_t image_adr)
{
    uint_fast32_t vector = *(const uint32_t *)BOOTVECT_GET_NEXT(BOOT_BASE_ADR);

    if (vector > BOOT_BASE_ADR && vector < APP_BASE_ADR)
    {
        return ((uint_fast32_t (*)(uint_fast32_t))vector)(image_adr);
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Function      : void Sys_Boot_ResetHandler(void)
 * ----------------------------------------------------------------------------
 * Description   : Sys_Boot_ResetHandler is called first after booting. It
 *                 checks if the Application or the Updater should be started.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Sys_Boot_ResetHandler(void);

#endif    /* _SYS_BOOT_H */
