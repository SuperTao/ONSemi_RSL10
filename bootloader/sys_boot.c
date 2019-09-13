/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* sys_boot.c
* - This module includes the resident part of the BootLoader.
* - It must be compiled with -fno-lto!
* ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <rsl10.h>
#include <rsl10_flash_rom.h>

#include "sys_boot.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define DNL_BASE_ADR            (APP_BASE_ADR + APP_MAX_SIZE / 2)

#if (APP_BASE_ADR % FLASH_SECTOR_SIZE != 0)
#error APP_BASE_ADR must be Flash sector aligned
#endif /* if (APP_BASE_ADR % FLASH_SECTOR_SIZE != 0) */
#if (DNL_BASE_ADR % FLASH_SECTOR_SIZE != 0)
#error DNL_BASE_ADR must be Flash sector aligned
#endif /* if (DNL_BASE_ADR % FLASH_SECTOR_SIZE != 0) */

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    SECTOR_DIRTY,
    SECTOR_BLANK,
    SECTOR_MATCH
} compare_result_t;

/* We recycle the UART buffer as Flash sector buffer */
extern uint32_t Drv_Uart_rx_buffer[];

/* ----------------------------------------------------------------------------
 * BootLoader Version
 * ------------------------------------------------------------------------- */

const Sys_Boot_app_version_t Sys_Boot_version =
{
    VER_ID, BOOT_VER_ENCODE(VER_MAJOR, VER_MINOR, VER_REVISION)
};

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

void Sys_Boot_Updater(void);

uint_fast32_t Sys_Boot_NextImage(uint_fast32_t image_adr);

/* ----------------------------------------------------------------------------
 * Function      : static void Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Configure relevant GPIO.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void Init(void)
{
    /* Configure GPIOs */
    Sys_DIO_Config(CFG_nUPDATE_DIO,
                   DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_ENABLE);
#ifdef CFG_LED_RED_DIO
    Sys_DIO_Config(CFG_LED_RED_DIO, DIO_MODE_GPIO_OUT_0 | DIO_6X_DRIVE);
#endif    /* ifdef CFG_LED_RED_DIO */
#ifdef CFG_LED_BLU_DIO
    Sys_DIO_Config(CFG_LED_BLU_DIO, DIO_MODE_GPIO_OUT_0 | DIO_6X_DRIVE);
#endif    /* ifdef CFG_LED_BLU_DIO */
    // disable RFID module
    Sys_DIO_Config(DIO_ENABLE_RFID, DIO_MODE_GPIO_OUT_0 | DIO_6X_DRIVE);
}

/* ----------------------------------------------------------------------------
 * Function      : static bool CheckUpdatePin(void)
 * ----------------------------------------------------------------------------
 * Description   : Checks if Updater activation is demanded by pin.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CheckUpdatePin(void)
{
    /* Check if Updater activation pin is active */
    return (DIO_DATA->ALIAS[CFG_nUPDATE_DIO] == 0);
}

/* ----------------------------------------------------------------------------
 * Function      : static uint32_t * LoadUpdaterCode(void)
 * ----------------------------------------------------------------------------
 * Description   : Copies the Updater code from Flash to PRAM.
 * Inputs        : None
 * Outputs       : return value     - pointer to Updater
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint32_t * LoadUpdaterCode(void)
{
	// 这几个变量都是定义在sections.ld中
    extern uint32_t __text_init__;
    extern uint32_t __text_start__;
    extern uint32_t __text_end__;
    // C语言引用链接脚本中的变量，不能直接赋值，要采用如下的方式进行引用
    uint32_t *src_p = &__text_init__;
    uint32_t *dst_p = &__text_start__;

    /* Copy the Updater code from Flash to PRAM */
    while (dst_p < &__text_end__)
    {
        *dst_p++ = *src_p++;
    }

    return &__text_start__;
}

/* ----------------------------------------------------------------------------
 * Function      : static compare_result_t CompareSector(
 *                                              uint_fast32_t check_adr,
 *                                              uint_fast32_t ref_adr)
 * ----------------------------------------------------------------------------
 * Description   : Compares the content of a Flash sector with a reference
 *                 sector and at the same time copies the reference sector
 *                 into a RAM buffer.
 * Inputs        : check_adr        - start address of check sector
 *                 ref_adr          - start address of reference sector
 * Outputs       : return value     - SECTOR_DIRTY  sector must be erased
 *                                    prior to program it
 *                                  - SECTOR_BLANK  sector is already blank
 *                                    and can directly be programmed
 *                                  - SECTOR_MATCH  check sector and reference
 *                                    sector have the same content
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static compare_result_t CompareSector(uint_fast32_t check_adr,
                                      uint_fast32_t ref_adr)
{
    uint_fast16_t length;
    uint_fast32_t blank = UINT32_MAX;
    compare_result_t result = SECTOR_MATCH;
    uint32_t       *buffer_p = Drv_Uart_rx_buffer;
    const uint32_t *check_p = (const uint32_t *)check_adr;
    const uint32_t *ref_p   = (const uint32_t *)ref_adr;

    for (length = FLASH_SECTOR_SIZE; length > 0; length -= sizeof(*check_p))
    {
        if (*check_p != *ref_p)
        {
            result = SECTOR_DIRTY;
        }
        blank      &= *check_p++;
        *buffer_p++ = *ref_p++;
    }
    if (result != SECTOR_MATCH && blank == UINT32_MAX)
    {
        result = SECTOR_BLANK;
    }
    return result;
}

/* ----------------------------------------------------------------------------
 * Function      : static bool CopyImage(void)
 * ----------------------------------------------------------------------------
 * Description   : Copies the primary Application image from the Download to
 *                 the Execution Area.
 * Inputs        : size             - image size in bytes
 * Outputs       : return value     - true  if copy was successful
 *                                  - false if copy failed
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CopyImage(uint_fast32_t size)
{
    uint_fast32_t dst_adr;
    uint_fast32_t src_adr;
    uint_fast32_t end_adr;

    /* Is there a image to copy? */
    if (size == 0)
    {
        /* No copy needed */
        return true;
    }

    /* Configure the flash to allow writing to the whole flash area */
    FLASH->MAIN_CTRL = MAIN_LOW_W_ENABLE    |
                       MAIN_MIDDLE_W_ENABLE |
                       MAIN_HIGH_W_ENABLE;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;

    /* Copy Image */
    for (dst_adr = APP_BASE_ADR, src_adr = DNL_BASE_ADR, end_adr = src_adr + size;
         src_adr < end_adr;
         dst_adr += FLASH_SECTOR_SIZE, src_adr += FLASH_SECTOR_SIZE)
    {
        switch (CompareSector(dst_adr, src_adr))
        {
            case SECTOR_DIRTY:
            {
                if (Flash_EraseSector(dst_adr) != FLASH_ERR_NONE)
                {
                    /* Disallow writing to the flash */
                    FLASH->MAIN_CTRL = 0;
                    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
                    return false;
                }

                /* FALL THROUGH */
            }

            case SECTOR_BLANK:
            {
                if (Flash_WriteBuffer(dst_adr,
                                      FLASH_SECTOR_SIZE / sizeof(unsigned int),
                                      (unsigned int *)Drv_Uart_rx_buffer) != FLASH_ERR_NONE)
                {
                    /* Disallow writing to the flash */
                    FLASH->MAIN_CTRL = 0;
                    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
                    return false;
                }
            }
            break;

            case SECTOR_MATCH:
            {
            }
            break;
        }
    }

    /* Invalidate images in Download area */
    Flash_WriteWordPair(dst_adr, 0, 0);
    Flash_WriteWordPair(DNL_BASE_ADR, 0, 0);

    /* Disallow writing to the flash */
    FLASH->MAIN_CTRL = 0;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : static uint_fast32_t ValidateImage(void)
 * ----------------------------------------------------------------------------
 * Description   : Validates the specified application.
 * Inputs        : None
 * Outputs       : return value     - image size in bytes
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint_fast32_t ValidateImage(void)
{
    uint_fast32_t entry;
    const uint32_t *vector_a = (const uint32_t *)DNL_BASE_ADR;

    /* Test Stack Pointer */
    entry = vector_a[0];
    if (entry < DRAM_BASE                                            ||
        entry > DRAM_BASE + DRAM_SIZE + DSP_DRAM_SIZE + BB_DRAM_SIZE ||
        entry % sizeof(uint32_t) != 0)
    {
        return 0;
    }

    /* Test Program Counter */
    entry = vector_a[1];
    if (entry < APP_BASE_ADR + 16 * sizeof(uint32_t) ||
        entry > APP_BASE_ADR + FLASH_SECTOR_SIZE     ||
        entry % sizeof(uint16_t) != 1)
    {
        return 0;
    }

    /* Test App Version pointer */
    entry = vector_a[7];
    if (entry < APP_BASE_ADR + 16 * sizeof(uint32_t) ||
        entry >= APP_BASE_ADR + FLASH_SECTOR_SIZE    ||
        entry % sizeof(uint16_t) != 0)
    {
        return 0;
    }

    /* Test primary BootLoader descriptor */
    entry = vector_a[8];
    if (entry < APP_BASE_ADR + 16 * sizeof(uint32_t) ||
        entry >= APP_BASE_ADR + FLASH_SECTOR_SIZE    ||
        entry % sizeof(uint32_t) != 0)
    {
        return 0;
    }

    return entry;
}

/* ----------------------------------------------------------------------------
 * Function      : static bool ValidateApp(uint_fast32_t app_adr)
 * ----------------------------------------------------------------------------
 * Description   : Validates an application.
 * Inputs        : image_adr        - start address of secondary application
 * Outputs       : return value     - true  if application is valid
 *                                  - false if application is invalid
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool ValidateApp(uint_fast32_t app_adr)
{
    BootROMStatus status = Sys_BootROM_ValidateApp((uint32_t *)app_adr);

    /* We do not use a CRC in the image header,
     * therefore BOOTROM_ERR_BAD_CRC is OK too */
    return (status == BOOTROM_ERR_NONE || status == BOOTROM_ERR_BAD_CRC);
}

/* ----------------------------------------------------------------------------
 * Function      : static bool CheckBuildId(
 *                                  const Sys_Boot_descriptor_t *dscr_p,
 *                                  const Sys_Boot_descriptor_t *ref_dscr_p)
 * ----------------------------------------------------------------------------
 * Description   : Checks the build ID of one image against a reference image.
 * Inputs        : dscr_p           - pointer to image descriptor
 *                 ref_dscr_p       - pointer to reference image descriptor
 * Outputs       : return value     - true  if build ID match
 *                                  - false if build ID are different
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CheckBuildId(const Sys_Boot_descriptor_t *dscr_p,
                         const Sys_Boot_descriptor_t *ref_dscr_p)
{
    const uint32_t *id_p;
    const uint32_t *ref_id_p;
    uint_fast32_t id_size;

    if (dscr_p == NULL || ref_dscr_p == NULL)
    {
        return false;
    }

    id_p     = dscr_p->build_id_a;
    ref_id_p = ref_dscr_p->build_id_a;
    for (id_size  = sizeof(dscr_p->build_id_a);
         id_size  > 0;
         id_size -= sizeof(dscr_p->build_id_a[0]))
    {
        if (*id_p++ != *ref_id_p++)
        {
            return false;
        }
    }
    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : static void StartPrimaryApp(void)
 * ----------------------------------------------------------------------------
 * Description   : Starts the primary application.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StartPrimaryApp(void)
{
    Sys_BootROM_StartApp((uint32_t *)APP_BASE_ADR);
}

/* ----------------------------------------------------------------------------
 * Function      : static void StartSecondaryApp(void)
 * ----------------------------------------------------------------------------
 * Description   : Starts the the secondary application.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StartSecondaryApp(void)
{
    uint_fast32_t image_adr = Sys_Boot_NextImage(APP_BASE_ADR);

    if (image_adr != 0)
    {
        Sys_BootROM_StartApp((uint32_t *)image_adr);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : static void StartApp(void)
 * ----------------------------------------------------------------------------
 * Description   : Starts the Application.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StartApp(void)
{
    /* Check if a valid primary application is installed */
    if (ValidateApp(APP_BASE_ADR))
    {
        /* 1st try starting secondary Application */
        StartSecondaryApp();

        /* Start primary Application */
        StartPrimaryApp();
    }
}

/* ----------------------------------------------------------------------------
 * Function      : uint_fast32_t Sys_Boot_NextImage(uint_fast32_t image_adr)
 * ----------------------------------------------------------------------------
 * Description   : Gets the start address of the next image if available.
 * Inputs        : image_adr        - image start address
 * Outputs       : return value     - next image start address
 *                                  - 0 for no next image found
 * Assumptions   :
 * ------------------------------------------------------------------------- */
uint_fast32_t Sys_Boot_NextImage(uint_fast32_t image_adr)
{
    const Sys_Boot_descriptor_t *dscr_p = Sys_Boot_GetDscr(image_adr);
    uint_fast32_t size   = Sys_Boot_GetImageSize(dscr_p);

    size += APP_SIG_SIZE;
    if (size >= APP_MIN_SIZE && size <= APP_MAX_SIZE / 2)
    {
        /* image is Flash sector aligned */
        size += -size % FLASH_SECTOR_SIZE;

        /* next image is behind this image */
        image_adr += size;

        if (ValidateApp(image_adr) &&
            CheckBuildId(Sys_Boot_GetDscr(image_adr), dscr_p))
        {
            return image_adr;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Function      : static void StartUpdater(void)
 * ----------------------------------------------------------------------------
 * Description   : Starts the Updater.
 *
 *                 The Updater can also reprogram itself, for that it must not
 *                 run from Flash memory. Therefore first the StartUpdater
 *                 copies the Updater code from Flash memory to Program RAM.
 *
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Sys_Boot_Updater(void)
{
    /* Initialize system */
    Sys_Initialize();

    /* Start Updater from PRAM */
    Sys_BootROM_StartApp(LoadUpdaterCode());

    /* If Updater start failed -> wait for Reset */
    for (;;);
}

/* ----------------------------------------------------------------------------
 * Function      : void Sys_Boot_ResetHandler(void)
 * ----------------------------------------------------------------------------
 * Description   : Sys_Boot_ResetHandler is called first after booting. It
 *                 checks if the Application or the Updater should be started.
 *
 *                 The Sys_Boot_ResetHandler must not call any other functions,
 *                 because these functions are linked to addresses in PRAM and
 *                 are not initialized yet. Sys_BootROM_StartApp is an
 *                 exception, it is a macro expanding to a function call into
 *                 the ROM.
 *
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Sys_Boot_ResetHandler(void)
{
    Init();
    // IO引脚电平，用来判断是否需要更新程序
    if (!CheckUpdatePin())
    {
    	// 判断是否有镜像需要拷贝，以及拷贝操作是否成功。
        if (CopyImage(ValidateImage()))
        {
        	// 启动app
            StartApp();
        }

        /* Fall through to Updater, if Application start failed */
    }
    // 启动失败，进入更新程序
    Sys_Boot_Updater();
}
