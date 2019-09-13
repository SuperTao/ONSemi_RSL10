/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* sys_upd.c
* -  The updater is responsible for downloading new application FW over the
* UART and program it into the Flash memory.
* -------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <string.h>
#include <rsl10.h>
#include <rsl10_flash_rom.h>

#include "sys_boot.h"
#include "drv_targ.h"
#include "drv_uart.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define NXT_TYPE                '\x55'
#define END_TYPE                '\xAA'

#define CRC32_SIZE              sizeof(crc32_t)
#define CRC32_GOOD              0x2144DF1C
#define CRC32_CONFIG            (CRC_32 | CRC_LITTLE_ENDIAN)

#define DMA_ALIGN               ALIGN(sizeof(uint32_t))

#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define ALIGN(x)                __attribute__ ((aligned(x)))

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef uint32_t crc32_t;

typedef enum
{
    NO_ERROR,
    BAD_MSG,
    UNKNOWN_CMD,
    INVALID_CMD,
    GENERAL_FLASH_FAILURE,
    WRITE_FLASH_NOT_ENABLED,
    BAD_FLASH_ADDRESS,
    ERASE_FLASH_FAILED,
    BAD_FLASH_LENGTH,
    INACCESSIBLE_FLASH,
    FLASH_COPIER_BUSY,
    PROG_FLASH_FAILED,
    VERIFY_FLASH_FAILED,
    VERIFY_IMAGE_FAILED,
    NO_VALID_BOOTLOADER
} err_t;

typedef enum
{
    HELLO,
    PROG,
    READ,
    RESTART
} cmd_type_t;

typedef struct
{
    uint32_t adr;                   /* start address of image
                                     * (must by a multiple of sector size) */
    uint32_t length;                /* image length in octets
                                     * (must by a multiple of 2) */
    uint32_t hash;                  /* image hash (CRC32) */
} prog_cmd_arg_t;

typedef struct
{
    uint32_t adr;                   /* start address to read from */
    uint32_t length;                /* read length in octets
                                     * (max sector size) */
} read_cmd_arg_t;

typedef union
{
    /* HELLO cmd has no arguments */
    prog_cmd_arg_t prog;
    read_cmd_arg_t read;

    /* RESTART cmd has no arguments */
} cmd_arg_t;

typedef struct
{
    cmd_type_t type;
    cmd_arg_t arg;
} cmd_msg_t;

typedef DMA_ALIGN struct
{
    Sys_Boot_app_version_t boot_ver;    /* version of the BootLoader */
    Sys_Boot_app_version_t app1_ver;    /* version of the installed primary application,
                                         * or 0 if no primary application is installed */
    uint16_t sector_size;               /* Flash sector size in octets */
    Sys_Boot_app_version_t app2_ver;    /* version of the installed secondary application,
                                         * (if no secondary application is installed,
                                         * this field is not included) */
    Drv_Uart_fcs_t fcs;                 /* calculated by drv_uart */
} hello_resp_msg_t;

typedef DMA_ALIGN struct
{
    uint8_t type;                       /* NXT_TYPE or END_TYPE */
    uint8_t code;                       /* one of error_t */
} resp_msg_t;

typedef struct
{
    prog_cmd_arg_t prop;
    crc32_t crc;
    uint32_t header_a[8];
} image_dscr_t;

/* ----------------------------------------------------------------------------
 * Function      : static void Init(void)
 * ----------------------------------------------------------------------------
 * Description   : System Initialization.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void Init(void)
{
    Drv_Targ_Init();
    Drv_Uart_Init();
}

/* ----------------------------------------------------------------------------
 * Function      : static cmd_msg_t * RecvCmd(void)
 * ----------------------------------------------------------------------------
 * Description   : Receives a command message.
 * Inputs        : None
 * Outputs       : return value     - pointer to message
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static cmd_msg_t * RecvCmd(void)
{
    cmd_msg_t    *cmd_p;
#if (CFG_TIMEOUT > 0)
    uint_fast32_t start_tick = Drv_Targ_GetTicks();
#endif
    do
    {
        /* Feed the watchdag */
        Drv_Targ_Poll();

        /* Receive command */
        Drv_Uart_StartRecv(sizeof(*cmd_p));
        cmd_p = Drv_Uart_FinishRecv();

#if (CFG_TIMEOUT > 0)
        // 计算超时时间，超过这个时间，就进行复位
        /* Check timeout */
        if (Drv_Targ_GetTicks() - start_tick > CFG_TIMEOUT * 1000)
        {
            Drv_Targ_Reset();
        }
#endif
    }
    while (cmd_p == NULL);

    return cmd_p;
}

/* ----------------------------------------------------------------------------
 * Function      : static void SendResp(uint_fast8_t  type, err_t code)
 * ----------------------------------------------------------------------------
 * Description   : Sends the RESP message.
 * Inputs        : type             - NXT_TYPE or END_TYPE
 *                 code             - one of err_t
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void SendResp(uint_fast8_t type, err_t code)
{
    static resp_msg_t resp;

    resp.type = type;
    resp.code = (uint8_t)code;
    Drv_Uart_StartSend(&resp, sizeof(resp), UART_WITHOUT_FCS);
}

/* ----------------------------------------------------------------------------
 * Function      : static void SendError(err_t error)
 * ----------------------------------------------------------------------------
 * Description   : Sends an error message.
 * Inputs        : error            - one of error_t
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void SendError(err_t error)
{
    SendResp(END_TYPE, error);
}

/* ----------------------------------------------------------------------------
 * Function      : static uint32_t * RecvSector(uint_fast32_t remaining_len,
 *                                              err_t         error)
 * ----------------------------------------------------------------------------
 * Description   : Reads a sector.
 * Inputs        : remaining_len    - remaining length of FW image in octets
 *                 error            - one of error_t
 * Outputs       : return value     - pointer to message buffer or NULL
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint32_t * RecvSector(uint_fast32_t remaining_len, err_t error)
{
    uint32_t *data_p = Drv_Uart_FinishRecv();

    if (error != NO_ERROR)
    {
        SendError(error);
        return NULL;
    }

    if (data_p != NULL && remaining_len > 0)
    {
        SendResp(NXT_TYPE, NO_ERROR);

        /* Prepare receiving the next sector */
        Drv_Uart_StartRecv(MIN(remaining_len, FLASH_SECTOR_SIZE));
    }
    return data_p;
}

/* ----------------------------------------------------------------------------
 * Function      : static err_t Verify(uint_fast32_t   adr,
 *                                     const uint32_t *data_p,
 *                                     uint_fast32_t   len)
 * ----------------------------------------------------------------------------
 * Description   : Verifies the programmed data and continues calculating hash.
 * Inputs        : adr              - flash start address
 *                 data_p           - pointer to image data to verify against
 *                 len              - length of image data to verify
 * Outputs       : return value     - NO_ERROR            if verify is OK
 *                                  - VERIFY_FLASH_FAILED if verify failed
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static err_t Verify(uint_fast32_t adr,
                    const uint32_t *data_p, uint_fast32_t len)
{
    for (len = len; len > 0; len -= sizeof(*data_p))
    {
        if (*data_p != *(uint32_t *)adr)
        {
            return VERIFY_FLASH_FAILED;
        }
        CRC->ADD_32 = *data_p++;                /* Update Hash */
        adr += sizeof(*data_p);
    }
    return NO_ERROR;
}

/* ----------------------------------------------------------------------------
 * Function      : static err_t ProgFlash(uint_fast32_t   adr,
 *                                        const uint32_t *data_p,
 *                                        uint_fast32_t   len)
 * ----------------------------------------------------------------------------
 * Description   : Programs data to the Flash and verifies it afterwards.
 * Inputs        : adr              - flash start address
 *                 data_p           - pointer to image data to program
 *                 len              - length of image data to program
 * Outputs       : return value     - NO_ERROR            if programming is OK
 *                                  - VERIFY_FLASH_FAILED if verify failed
 *                                  - or a flash HW error
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static err_t ProgFlash(uint_fast32_t adr,
                       const uint32_t *data_p, uint_fast32_t len)
{
    FlashStatus status;

    status = Flash_WriteBuffer(adr, len / sizeof(unsigned int),
                               (unsigned int *)data_p);
    if (status != FLASH_ERR_NONE)
    {
        return INVALID_CMD + status;
    }
    return Verify(adr, data_p, len);
}

/* ----------------------------------------------------------------------------
 * Function      : static err_t ProgSector(uint_fast32_t   adr,
 *                                         const uint32_t *data_p,
 *                                         uint_fast16_t   sector_len)
 * ----------------------------------------------------------------------------
 * Description   : Erases a sector and programs it with new data.
 * Inputs        : adr              - flash start address
 *                 data_p           - pointer to image data to program
 *                 sector_len       - length of sector
 * Outputs       : return value     - NO_ERROR            if programming is OK
 *                                  - VERIFY_FLASH_FAILED if verify failed
 *                                  - or a flash HW error
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static err_t ProgSector(uint_fast32_t adr,
                        const uint32_t *data_p, uint_fast16_t sector_len)
{
    FlashStatus status;

    status = Flash_EraseSector(adr);
    if (status != FLASH_ERR_NONE)
    {
        return INVALID_CMD + status;
    }
    return ProgFlash(adr, data_p, sector_len);
}

/* ----------------------------------------------------------------------------
 * Function      : static void SaveHeader(image_dscr_t *image_p,
 *                                        uint32_t     *header_p,
 *                                        uint_fast16_t sector_len)
 * ----------------------------------------------------------------------------
 * Description   : Saves the image header. It will be programmed at the end of
 *                 the update to validate the programmed image.
 * Inputs        : image_p          - pointer to image descriptor
 *                 header_p         - pointer to image header
 *                 sector_len       - length of sector
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void SaveHeader(image_dscr_t *image_p,
                       uint32_t     *header_p, uint_fast16_t sector_len)
{
    uint32_t *data_p = header_p;

    /* Calculate Hash over 1st sector */
    for (sector_len  = sector_len;
         sector_len  > 0;
         sector_len -= sizeof(*data_p))
    {
        CRC->ADD_32 = *data_p++;            /* Update Hash */
    }

    /* Save header */
    memcpy(image_p->header_a, header_p, sizeof(image_p->header_a));

    /* Destroy header in buffer */
    memset(header_p, 0xFF, sizeof(image_p->header_a));

    /* Configure the flash to allow writing to the whole flash area */
    FLASH->MAIN_CTRL = MAIN_LOW_W_ENABLE    |
                       MAIN_MIDDLE_W_ENABLE |
                       MAIN_HIGH_W_ENABLE;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;
}

/* ----------------------------------------------------------------------------
 * Function      : static void ProgHeader(image_dscr_t *image_p,
 *                                        err_t         resp_code)
 * ----------------------------------------------------------------------------
 * Description   : Checks the hash and programs the saved image header.
 *                 This will validate the programmed image.
 * Inputs        : image_p          - pointer to image descriptor
 *                 resp_code        - response code
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProgHeader(image_dscr_t *image_p, err_t resp_code)
{
    if (resp_code == NO_ERROR)
    {
        /* Check image hash */
        if (CRC->FINAL != image_p->prop.hash)
        {
            resp_code = VERIFY_IMAGE_FAILED;
        }

        /* Program image header */
        else
        {
            resp_code = ProgFlash(image_p->prop.adr,
                                  image_p->header_a,
                                  sizeof(image_p->header_a));
        }
    }

    /* Disallow writing to the flash */
    FLASH->MAIN_CTRL = 0;
    FLASH->MAIN_WRITE_UNLOCK = FLASH_MAIN_KEY;

    SendResp(END_TYPE, resp_code);
}

/* ----------------------------------------------------------------------------
 * Function      : static bool CopyVersionInfo(Sys_Boot_app_version_t *buffer_p,
 *                                             uint_fast32_t           image_adr)
 * ----------------------------------------------------------------------------
 * Description   : Copies one image version to the HELLO response.
 * Inputs        : image_adr        - image start address
 * Outputs       : return value     - true  if version info valid
 *                                  - false if version info invalid
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CopyVersionInfo(Sys_Boot_app_version_t *buffer_p,
                            uint_fast32_t image_adr)
{
    const Sys_Boot_app_version_t *version_p;
    BootROMStatus status;

    if (image_adr != 0)
    {
        /* Check if a valid application is installed already */
        status = Sys_BootROM_ValidateApp((uint32_t *)image_adr);

        /* We do not use a CRC in the image header,
         * therefore BOOTROM_ERR_BAD_CRC is OK too */
        if (status == BOOTROM_ERR_NONE || status == BOOTROM_ERR_BAD_CRC)
        {
            version_p = Sys_Boot_GetVersion(image_adr);
            if (version_p != NULL)
            {
                *buffer_p = *version_p;
            }
            else
            {
                memcpy(buffer_p, "??????", sizeof(buffer_p->id));
            }
            return true;
        }
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : static void ProcessHello(void)
 * ----------------------------------------------------------------------------
 * Description   : Processes the HELLO command.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProcessHello(void)
{
    static hello_resp_msg_t hello;
    uint_fast16_t size = sizeof(hello) - sizeof(hello.app2_ver);

    memset(&hello, 0, sizeof(hello));
    CopyVersionInfo(&hello.boot_ver, BOOT_BASE_ADR);
    if (CopyVersionInfo(&hello.app1_ver, APP_BASE_ADR))
    {
        if (CopyVersionInfo(&hello.app2_ver,
                            Sys_Boot_GetNextImage(APP_BASE_ADR)))
        {
            size += sizeof(hello.app2_ver);
        }
    }
    hello.sector_size = FLASH_SECTOR_SIZE;
    Drv_Uart_StartSend(&hello, size, UART_WITH_FCS);
}

/* ----------------------------------------------------------------------------
 * Function      : static void ProcessProg(prog_cmd_arg_t *arg_p)
 * ----------------------------------------------------------------------------
 * Description   : Processes the PROG command.
 * Inputs        : arg_p            - pointer to PROG command message
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProcessProg(prog_cmd_arg_t *arg_p)
{
    uint_fast32_t current_adr   = arg_p->adr;
    uint_fast32_t remaining_len = arg_p->length;
    uint_fast32_t sector_len    = MIN(remaining_len, FLASH_SECTOR_SIZE);
    err_t resp_code = NO_ERROR;
    image_dscr_t image;
    uint32_t    *data_p;

    /* Check start address and length of image */
    if ((arg_p->adr != APP_BASE_ADR && arg_p->adr != BOOT_BASE_ADR)              ||
        arg_p->adr + arg_p->length                 > APP_BASE_ADR + APP_MAX_SIZE ||
        arg_p->adr    % FLASH_SECTOR_SIZE         != 0                           ||
        arg_p->length % (2 * sizeof(uint32_t))    != 0                           ||
        arg_p->length                              < APP_MIN_SIZE)
    {
        SendError(INVALID_CMD);
        return;
    }
    image.prop = *arg_p;

    /* Prepare receiving the 1st sector */
    SendResp(NXT_TYPE, NO_ERROR);
    Drv_Uart_StartRecv(sector_len);

    /* Process image */
    while (remaining_len > 0)
    {
        remaining_len -= sector_len;

        /* Feed Watchdog */
        Drv_Targ_Poll();

        /* Wait for next image sector */
        data_p = RecvSector(remaining_len, resp_code);
        if (data_p == NULL)
        {
            return;
        }

        Sys_CRC_Set_Config(CRC32_CONFIG);
        if (current_adr == image.prop.adr)
        {
            /* Save image header */
            CRC->VALUE = CRC_32_INIT_VALUE;     /* Init Hash */
            SaveHeader(&image, data_p, sector_len);
            image.crc = CRC->VALUE;             /* Store Hash for next sector */

            /* Program 1st image sector */
            resp_code = ProgSector(current_adr, data_p, sector_len);
        }
        else
        {
            /* Program next image sector */
            CRC->VALUE = image.crc;             /* Restore Hash */
            if (Verify(current_adr, data_p, sector_len) != NO_ERROR)
            {
                CRC->VALUE = image.crc;         /* Reset Hash */
                resp_code  = ProgSector(current_adr, data_p, sector_len);
            }
            image.crc = CRC->VALUE;             /* Store Hash for next sector */
        }

        current_adr += sector_len;
        sector_len   = MIN(remaining_len, FLASH_SECTOR_SIZE);
    }

    /* Program saved image header */
    ProgHeader(&image, resp_code);
}

#if (CFG_READ_SUPPORT)
/* ----------------------------------------------------------------------------
 * Function      : static void ProcessRead(cmd_msg_t *cmd_p)
 * ----------------------------------------------------------------------------
 * Description   : Processes the READ command.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProcessRead(cmd_msg_t *cmd_p)
{
    const void   *src_p  = (const void *)cmd_p->arg.read.adr;
    uint_fast32_t length = cmd_p->arg.read.length;

    /* we recycle the input buffer as output buffer */
    uint8_t      *resp_p = (uint8_t *)cmd_p;

    /* Only allow READ command, if Debug Lock is not set */
    if (SYSCTRL_DBG_LOCK->DBG_LOCK_RD_ALIAS != DBG_ACCESS_UNLOCKED_BITBAND)
    {
        SendError(UNKNOWN_CMD);
    }
    else if (length == 0 || length > FLASH_SECTOR_SIZE)
    {
        SendError(INVALID_CMD);
    }
    else
    {
        memcpy(resp_p, src_p, length);
        length += sizeof(Drv_Uart_fcs_t);
        Drv_Uart_StartSend(resp_p, length, UART_WITH_FCS);
    }
}

#endif /* if (CFG_READ_SUPPORT) */

/* ----------------------------------------------------------------------------
 * Function      : static void ProcessRestart(void)
 * ----------------------------------------------------------------------------
 * Description   : Processes the RESTART command.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProcessRestart(void)
{
    BootROMStatus status;

    /* Check if the BootLoader is intact */
    status = Sys_BootROM_ValidateApp((uint32_t *)BOOT_BASE_ADR);

    /* We do not use a CRC in the image header,
     * therefore BOOTROM_ERR_BAD_CRC is OK too */
    if (status == BOOTROM_ERR_NONE || status == BOOTROM_ERR_BAD_CRC)
    {
        SendResp(END_TYPE, NO_ERROR);

        /* Wait for completely sent response */
        Drv_Uart_FinishSend();

        /* Execute restart */
        Drv_Targ_Reset();
    }

    SendError(NO_VALID_BOOTLOADER);
}

/* ----------------------------------------------------------------------------
 * Function      : static void ProcessCmd(void)
 * ----------------------------------------------------------------------------
 * Description   : Processes a CMD message.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ProcessCmd(void)
{
	// 接受命令
    cmd_msg_t  *cmd_p = RecvCmd();

    switch (cmd_p->type)
    {
        case HELLO:
        {
            ProcessHello();
        }
        break;

        case PROG:
        {
            ProcessProg(&cmd_p->arg.prog);
        }
        break;

    #if (CFG_READ_SUPPORT)
        case READ:
        {
            ProcessRead(cmd_p);
        }
        break;
    #endif /* if (CFG_READ_SUPPORT) */

        case RESTART:
        {
            ProcessRestart();
        }
        break;

        default:
        {
            SendError(UNKNOWN_CMD);
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : int main(void)
 * ----------------------------------------------------------------------------
 * Description   : Main routine of the Updater.
 * Inputs        : None
 * Outputs       : Returns always 0
 * Assumptions   :
 * ------------------------------------------------------------------------- */
int main(void)
{
	// 时钟，串口初始化
    Init();

    for (;;)
    {
        /* Wait for CMD message */
        ProcessCmd();
        // 喂狗，防止复位
        /* Feed Watchdog */
        Drv_Targ_Poll();
    }

    /* never reached */
    return 0;
}
