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
 * app_dfu.c
 * - This is the DFU component. It is responsible for updating the system with
 *   new firmware.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdlib.h>
#include <rsl10.h>
#include <rsl10_ke.h>

#include "sys_boot.h"
#include "sys_fota.h"
#include "sys_man.h"
#include "app_dfu.h"
#include "app_hdlc.h"
#include "app_conf.h"
#include "msg_handler.h"
#include "drv_targ.h"
#include "drv_flash.h"

#include "sha256.h"
#include "uECC.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define IMAGE_SECTOR_SIZE           FLASH_SECTOR_SIZE
#define IMAGE_HEADER_SIZE           (IMAGE_SECTOR_SIZE / 2)

#if (IMAGE_SECTOR_SIZE % 8 != 0)
    #error IMAGE_SECTOR_SIZE must be a multiple of 8
#endif /* if (IMAGE_SECTOR_SIZE % 8 != 0) */

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    IMAGE_DDOWNLOAD = 1,
} msg_code_t;

typedef enum
{
    MSG_WAIT,
    MSG_BEGIN,
    MSG_DATA,
    MSG_END,
    MSG_SKIP
} msg_state_t;

typedef struct
{
    uint8_t code;
    uint8_t param_a[3];
    uint32_t body_len;
} msg_header_t;

typedef struct
{
    msg_state_t state;
    uint32_t rx_len;
    msg_header_t header;
    uint32_t body_a[IMAGE_SECTOR_SIZE / sizeof(uint32_t)];
} message_t;

typedef struct
{
    uint32_t word0;
    uint32_t word1;
} flash_quantum_t;

typedef enum
{
    IMAGE_DNL_OK                = 0,
    IMAGE_DNL_BAD_DEVID         = 1,
    IMAGE_DNL_BAD_BUILDID       = 2,
    IMAGE_DNL_BAD_SIZE          = 3,
    IMAGE_DNL_BAD_FLASH         = 4,
    IMAGE_DNL_BAD_SIG           = 5,
    IMAGE_DNL_BAD_START         = 6,
    IMAGE_DNL_INTERNAL_FAILURE  = 255
} image_dnl_resp_status_t;

typedef enum
{
    PROG_SUCCESS,
    PROG_ONGOING,
    PROG_FAILURE
} prog_state_t;

typedef struct
{
    prog_state_t state;
    uint32_t flash_start_adr;
    uint32_t erase_len;
    uint32_t prog_len;
    flash_quantum_t vector;
    SHA256_CTX hash;
} image_dnl_t;

typedef struct
{
    uint32_t initial_sp;
    uint32_t reset_handler;
    uint32_t exception_handler_a[5];
    uint32_t version_info;
    uint32_t image_dscr;
} vector_table_t;

static message_t current_msg;
static image_dnl_t image_download;

/* ----------------------------------------------------------------------------
 * Function      : int memtst(const void *mem_p, int c, size_t size)
 * ----------------------------------------------------------------------------
 * Description   : Test a memory block for containing the same bytes.
 * Inputs        : mem_p            - pointer to memory block
 *                 c                - byte value to match
 *                 size             - size of the memory block
 * Outputs       : return value     - == 0  memory block matches
 *                                  - != 0  memory block does not match
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static int memtst(const void *mem_p, int c, size_t size)
{
    const char *char_p = mem_p;

    for (; size > 0; size--)
    {
        int diff = *char_p++ - c;

        if (diff != 0)
        {
            return diff;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Function      : uint_fast32_t GetStackStart(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the the BLE stack start address.
 * Inputs        : None
 * Outputs       : return value     - always the BootLoader application
 *                                    start address
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint_fast32_t GetStackStart(void)
{
    return APP_BASE_ADR;
}

/* ----------------------------------------------------------------------------
 * Function      : uint_fast32_t GetAppStart(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the the application start address corresponding to
 *                 the currently installed BLE stack.
 * Inputs        : None
 * Outputs       : return value     - application start address
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint_fast32_t GetAppStart(void)
{
    uint_fast32_t size = Sys_Boot_GetImageSize(
        Sys_Boot_GetDscr(APP_BASE_ADR));

    size += sizeof(App_Conf_key_t);

    /* image is Flash sector aligned */
    size += -size % FLASH_SECTOR_SIZE;

    /* next image is behind this image */
    return APP_BASE_ADR + size;
}

/* ----------------------------------------------------------------------------
 * Function      : bool CheckDevID(const uint8_t *id_p)
 * ----------------------------------------------------------------------------
 * Description   : Checks the image device ID against the installed device ID.
 *                 (If the installed device ID is all 0s -> accept images with
 *                  any device ID)
 * Inputs        : id_p             - pointer to image device ID
 * Outputs       : return value     - true  device ID match
 *                                  - false device ID differ
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CheckDevID(const uint8_t *id_p)
{
    if (memtst(App_Conf_GetDeviceID(), 0,
               sizeof(App_Conf_uuid_t)) != 0)
    {
        if (memcmp(App_Conf_GetDeviceID(), id_p,
                   sizeof(App_Conf_uuid_t)) != 0)
        {
            return false;
        }
    }
    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : bool CheckBuildID(const uint32_t id_a[])
 * ----------------------------------------------------------------------------
 * Description   : Checks the image build ID against the installed
 *                 BLE stack build ID.
 * Inputs        : id_a             - image build ID
 * Outputs       : return value     - true  build ID match
 *                                  - false build ID differ
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool CheckBuildID(const uint32_t id_a[])
{
    return (memcmp(App_Conf_GetBuildID(), id_a,
                   sizeof(App_Conf_build_id_t)) == 0);
}

/* ----------------------------------------------------------------------------
 * Function      : bool ProgramImage(message_t *msg_p, image_dnl_t *dnl_p)
 * ----------------------------------------------------------------------------
 * Description   : Programs image data to flash memory.
 * Inputs        : msg_p            - pointer to message structure
 *                 data_p           - pointer to message part
 * Outputs       : return value     - true  no error so far
 *                                  - false flash memory error
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool ProgramImage(message_t *msg_p, image_dnl_t *dnl_p)
{
    static const uint32_t invalid_mark_a[2] = { 0, 0 };
    // 判断状态是否正在进行，这个状态是接受完数据时改变的
    if (dnl_p->state   == PROG_ONGOING &&
        dnl_p->prog_len < msg_p->rx_len)
    {
        if (dnl_p->prog_len < dnl_p->erase_len)
        {
            uint_fast32_t len;
            uint_fast32_t adr    = dnl_p->flash_start_adr + dnl_p->prog_len;
            uint8_t      *data_p = (uint8_t *)msg_p->body_a +
                                   dnl_p->prog_len % sizeof(msg_p->body_a);

            /* check if at least two words are available to program */
            if (msg_p->rx_len - dnl_p->prog_len < sizeof(flash_quantum_t))
            {
                /* wait for more data except we reached the end of the message */
                if (msg_p->rx_len < msg_p->header.body_len)
                {
                    return true;
                }
                len = msg_p->header.body_len - dnl_p->prog_len;

                /* fill up to a flash prog quantum */
                memset(data_p + len, -1, sizeof(flash_quantum_t) - len);
            }
            // 将数据写入到flash中
            /* program flash */
            if (Drv_Flash_Program(adr, (uint32_t *)data_p))
            {
                /* update hash with read-back data excluding signature */
                len = msg_p->header.body_len - sizeof(App_Conf_key_t);
                if (len > dnl_p->prog_len)
                {
                    len -= dnl_p->prog_len;
                    if (len > sizeof(flash_quantum_t))
                    {
                        len = sizeof(flash_quantum_t);
                    }
                    sha256_update(&dnl_p->hash,
                                  (const uint8_t *)adr,
                                  len);
                }
                dnl_p->prog_len += sizeof(flash_quantum_t);
                return true;
            }
        }
        else
        {
            /* invalidate app image */
            if (dnl_p->erase_len > 0 ||
                Drv_Flash_Program(GetAppStart(), invalid_mark_a))
            {
                /* erase flash */
                if (Drv_Flash_Erase(dnl_p->flash_start_adr + dnl_p->erase_len))
                {
                    dnl_p->erase_len += FLASH_SECTOR_SIZE;
                    return true;
                }
            }
        }
        Drv_Flash_Lock();
        dnl_p->state = PROG_FAILURE;
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : image_dnl_resp_status_t CheckSignature(message_t   *msg_p,
 *                                                        image_dnl_t *dnl_p)
 * ----------------------------------------------------------------------------
 * Description   : Checks the image signature.
 * Inputs        : msg_p            - pointer to message structure
 *                 dnl_p            - pointer to download structure
 * Outputs       : return value     - IMAGE_DNL_OK
 *                                      signature is ok
 *                                  - IMAGE_DNL_BAD_SIG
 *                                      signature is wrong
 *                                  - IMAGE_DNL_BAD_FLASH
 *                                      flash memory error
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static image_dnl_resp_status_t CheckSignature(message_t *msg_p,
                                              image_dnl_t *dnl_p)
{
    App_Conf_key_t *pub_key_p = App_Conf_GetPublicKey();
    const uint8_t  *sig_p = (const uint8_t *)(dnl_p->flash_start_adr +
                                              msg_p->header.body_len -
                                              sizeof(App_Conf_key_t));

    /* finish programming image */
    while (ProgramImage(msg_p, dnl_p));

    if (dnl_p->state == PROG_ONGOING)
    {
        /* finalize hash calculation */
        sha256_final(&dnl_p->hash, dnl_p->hash.data);

        /* check if a public key is available */
        if (memtst(pub_key_p, 0, sizeof(App_Conf_key_t)) != 0)
        {
            /* verify signature */
            if (uECC_verify(*pub_key_p, dnl_p->hash.data, SHA256_BLOCK_SIZE,
                            sig_p,
                            uECC_secp256r1()) == 0)
            {
                Drv_Flash_Lock();
                dnl_p->state = PROG_FAILURE;
                return IMAGE_DNL_BAD_SIG;
            }
        }
        else if (memtst(sig_p + SHA256_BLOCK_SIZE, -1, SHA256_BLOCK_SIZE) == 0)
        {
            /* check hash */
            if (memcmp(dnl_p->hash.data,
                       sig_p,
                       SHA256_BLOCK_SIZE) != 0)
            {
                Drv_Flash_Lock();
                dnl_p->state = PROG_FAILURE;
                return IMAGE_DNL_BAD_SIG;
            }
        }

        /* mark image as valid */
        if (Drv_Flash_Program(dnl_p->flash_start_adr,
                              &dnl_p->vector.word0))
        {
            Drv_Flash_Lock();
            dnl_p->state = PROG_SUCCESS;
            return IMAGE_DNL_OK;
        }
    }
    Drv_Flash_Lock();
    dnl_p->state = PROG_FAILURE;
    return IMAGE_DNL_BAD_FLASH;
}

/* ----------------------------------------------------------------------------
 * Function      : image_dnl_resp_status_t CheckImage(message_t *msg_p,
 *                                   image_dnl_t *dnl_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Checks the image data.
 * Inputs        : msg_p            - pointer to message structure
 *                 dnl_p            - pointer to download structure
 *                 size             - message part size
 * Outputs       : return value     - IMAGE_DNL_OK
 *                                      everything so far ok
 *                                  - IMAGE_DNL_BAD_SIZE
 *                                      image has wrong size
 *                                  - IMAGE_DNL_BAD_DEVID
 *                                      image incompatible with device
 *                                  - IMAGE_DNL_BAD_BUILDID
 *                                      image incompatible with BLE stack
 *                                  - IMAGE_DNL_BAD_START
 *                                      image has wrong start address
 *                                  - IMAGE_DNL_BAD_FLASH
 *                                      flash memory error
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static image_dnl_resp_status_t CheckImage(message_t *msg_p,
                                          image_dnl_t *dnl_p,
                                          uint_fast16_t size)
{
    const vector_table_t        *vector_p = (void *)msg_p->body_a;
    const Sys_Fota_version_t    *version_info_p;
    const Sys_Boot_descriptor_t *image_dscr_p;
    uint_fast32_t image_start;
    uint_fast32_t image_size;
    uint_fast32_t offset;

    if (msg_p->rx_len >= IMAGE_HEADER_SIZE)
    {
        /* check for flash error */
        if (dnl_p->state == PROG_FAILURE)
        {
            return IMAGE_DNL_BAD_FLASH;
        }

        /* header was already checked */
        return IMAGE_DNL_OK;
    }
    if (msg_p->rx_len + size < IMAGE_HEADER_SIZE)
    {
        /* header is still incomplete */
        return IMAGE_DNL_OK;
    }

    image_start    = vector_p->reset_handler & ~(IMAGE_SECTOR_SIZE - 1);
    offset         = vector_p->version_info - image_start;
    version_info_p = (const void *)((const uint8_t *)vector_p + offset);
    offset         = vector_p->image_dscr - image_start;
    image_dscr_p   = (const void *)((const uint8_t *)vector_p + offset);
    image_size     = image_dscr_p->image_size + sizeof(App_Conf_key_t);

    if (image_size != msg_p->header.body_len)
    {
        /* sub-image size does not match IMAGE_DOWNLOAD body length */
        return IMAGE_DNL_BAD_SIZE;
    }
    else if (!CheckDevID(version_info_p->dev_id))
    {
        /* sub-image is incompatible with Device ID */
        return IMAGE_DNL_BAD_DEVID;
    }
    else if (image_start == GetStackStart())
    {
        /* it is a FOTA stack sub-image */
        if (image_size < APP_MIN_SIZE)
        {
            /* FOTA stack sub-image is too small */
            return IMAGE_DNL_BAD_SIZE;
        }
        else if (image_size > APP_MAX_SIZE / 2)
        {
            /* FOTA stack sub-image is too large */
            return IMAGE_DNL_BAD_SIZE;
        }
        else
        {
            dnl_p->flash_start_adr = image_start + APP_MAX_SIZE / 2;
        }
    }
    else if (image_start == GetAppStart())
    {
        /* it is a Application sub-image */
        if (!CheckBuildID(image_dscr_p->build_id_a))
        {
            /* Application sub-image is incompatible with installed FOTA stack */
            return IMAGE_DNL_BAD_BUILDID;
        }
        else if (image_size < APP_MIN_SIZE)
        {
            /* Application sub-image is too small */
            return IMAGE_DNL_BAD_SIZE;
        }
        else if (image_start + image_size > APP_BASE_ADR + APP_MAX_SIZE)
        {
            /* Application sub-image is too large */
            return IMAGE_DNL_BAD_SIZE;
        }
        else
        {
            dnl_p->flash_start_adr = image_start;
        }
    }
    else
    {
        return IMAGE_DNL_BAD_START;
    }

    /* init flash programming */
    memcpy(&dnl_p->vector, msg_p->body_a, sizeof(dnl_p->vector));
    sha256_init(&dnl_p->hash);
    sha256_update(&dnl_p->hash, (uint8_t *)&dnl_p->vector,
                  sizeof(dnl_p->vector));
    dnl_p->prog_len  = sizeof(dnl_p->vector);
    dnl_p->erase_len = 0;
    // 改变状态，这个状态main中会用到
    dnl_p->state     = PROG_ONGOING;
    Drv_Flash_Unlock();
    return IMAGE_DNL_OK;
}

/* ----------------------------------------------------------------------------
 * Function      : void ImageDownloadResp(image_dnl_resp_status_t status)
 * ----------------------------------------------------------------------------
 * Description   : Sends an image download response message.
 * Inputs        : status           - response status
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void ImageDownloadResp(image_dnl_resp_status_t status)
{
    static msg_header_t resp = { IMAGE_DDOWNLOAD };

    resp.param_a[0] = status;
    App_Hdlc_DataReq(0, &resp.code, sizeof(resp));
}

/* ----------------------------------------------------------------------------
 * Function      : bool ImageDownloadCmd(message_t *msg_p,
 *                                  const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Processes image download command message.
 * Inputs        : msg_p            - pointer to message structure
 *                 data_p           - pointer to message part
 *                 size             - message part size
 * Outputs       : return value     - true  no error so far
 *                                  - false error in message
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool ImageDownloadCmd(message_t *msg_p,
                             const uint8_t *data_p, uint_fast16_t size)
{
    switch (msg_p->state)
    {
        case MSG_BEGIN:
        	// 开始
        /* handle download command begin */
        {
            /* check for minimal body length */
            if (msg_p->header.body_len < IMAGE_HEADER_SIZE)
            {
                /* body too small -> abort */
                ImageDownloadResp(IMAGE_DNL_BAD_SIZE);
                return false;
            }
        }
        break;
        // 结束
        case MSG_END:

        /* handle download command end */
        {
            ImageDownloadResp(CheckSignature(msg_p, &image_download));
        }
        break;

        default:

        /* handle download command data */
        {
            image_dnl_resp_status_t resp;
            uint_fast32_t index = msg_p->rx_len % sizeof(msg_p->body_a);
            uint_fast32_t len   = sizeof(msg_p->body_a) - index;

            if (size + msg_p->rx_len -
                image_download.prog_len > sizeof(msg_p->body_a))
            {
                ImageDownloadResp(IMAGE_DNL_INTERNAL_FAILURE);
                return false;
            }

            /* copy data to ring buffer */
            if (size <= len)
            {
                memcpy((uint8_t *)msg_p->body_a + index, data_p, size);
            }
            else
            {
                memcpy((uint8_t *)msg_p->body_a + index, data_p, len);
                memcpy((uint8_t *)msg_p->body_a, data_p + len, size - len);
            }
            // 对收到的数据进行校验
            /* check image */
            resp = CheckImage(msg_p, &image_download, size);
            if (resp != IMAGE_DNL_OK)
            {
                ImageDownloadResp(resp);
                return false;
            }
        }
        break;
    }

    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : void HandleMsg(message_t *msg_p, const uint8_t *data_p,
 *                                                  uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Handles receiving the current message.
 * Inputs        : msg_p            - pointer to message structure
 *                 data_p           - pointer to message part
 *                 size             - message part size
 * Outputs       : msg_p->state     - MSG_DATA ready for more data
 *                                  - MSG_SKIP message error
 *                                             -> skip more message data
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void HandleMsg(message_t *msg_p,
                      const uint8_t *data_p, uint_fast16_t size)
{
    bool result;

    /* dispatch to message handler */
    switch (msg_p->header.code)
    {
        case IMAGE_DDOWNLOAD:
        {
        	// 下载命令
            result = ImageDownloadCmd(msg_p, data_p, size);
        }
        break;

        default:
        {
            result = false;
        }
        break;
    }

    msg_p->state = result ? MSG_DATA : MSG_SKIP;
}

/* ----------------------------------------------------------------------------
 * Function      : bool DataInd(message_t *msg_p, const uint8_t *data_p,
 *                                                uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Processes the received message part.
 * Inputs        : msg_p            - pointer to message structure
 *                 data_p           - pointer to message part
 *                 size             - message part size
 * Outputs       : return value     - true  receiver is ready for more data
 *                                  - false receiver is now busy
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool DataInd(message_t *msg_p,
                    const uint8_t *data_p, uint_fast16_t size)
{
    /* check for message begin */
	// 数据开头
    if (msg_p->state == MSG_WAIT)
    {
        memcpy(&msg_p->header, data_p, sizeof(msg_p->header));
        data_p += sizeof(msg_p->header);
        size   -= sizeof(msg_p->header);
        msg_p->rx_len = 0;
        msg_p->state  = MSG_BEGIN;
        HandleMsg(msg_p, NULL, 0);
    }

    /* remove message padding */
    if (size + msg_p->rx_len > msg_p->header.body_len)
    {
        size = msg_p->header.body_len - msg_p->rx_len;
    }

    if (size > 0)
    {
        if (msg_p->state == MSG_DATA)
        {
            HandleMsg(msg_p, data_p, size);
        }
        msg_p->rx_len += size;
    }

    /* check for message end */
    if (msg_p->rx_len == msg_p->header.body_len)
    {
        if (msg_p->state == MSG_DATA)
        {
            msg_p->state = MSG_END;
            HandleMsg(msg_p, NULL, 0);
        }
        msg_p->state = MSG_WAIT;
    }

    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : void TimeoutHandler(ke_msg_id_t  msg_id,
 *                                     const void  *param_p,
 *                                     ke_task_id_t dest_id,
 *                                     ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handles advertising supervision timeout.
 * Inputs        : msg_id           - Kernel message ID number
 *                 param_p          - always NULL
 *                 dest_id          - Destination task ID number
 *                 src_id           - Source task ID number
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void TimeoutHandler(ke_msg_id_t msg_id, const void *param_p,
                           ke_task_id_t dest_id, ke_task_id_t src_id)
{
    /* reset if application is still installed */
    if (App_Conf_GetVersion(APP_CONF_APP_VERSION) != NULL)
    {
        Drv_Targ_Reset();
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void StateChangeHandler(ke_msg_id_t  msg_id,
 *                                         const void  *param_p,
 *                                         ke_task_id_t dest_id,
 *                                         ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handles state changes.
 * Inputs        : msg_id           - Kernel message ID number
 *                 param_p          - always NULL
 *                 dest_id          - Destination task ID number
 *                 src_id           - Source task ID number
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StateChangeHandler(ke_msg_id_t msg_id, const void *param_p,
                               ke_task_id_t dest_id, ke_task_id_t src_id)
{
    switch (Sys_Man_GetAppState(dest_id))
    {
        case APP_BLE_ADVERTISING:
        {
        	// 广播阶段设置定时器，超时以后，设备重新启动。
            ke_timer_set(APP_DFU_SUPERVISOR_TIMER, dest_id,
                         KE_TIME_IN_SEC(CFG_MAX_ADVERTISING_TIME));
        }
        break;

        case APP_BLE_CONNECTED:
        {
        	// 连接后，清楚定时器
            ke_timer_clear(APP_DFU_SUPERVISOR_TIMER, TASK_APP);
        }
        break;

        case APP_BLE_LINKUP:
        {
            current_msg.state = MSG_WAIT;
            image_download.state = PROG_SUCCESS;
        }
        break;

        case APP_BLE_DISCONNECTED:
        {
        	// 蓝牙断开，设备重启
            Drv_Targ_Reset();
        }
        break;

        default:
        {
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : bool App_Hdlc_DataInd(const uint8_t *data_p,
 *                                       uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Indicates received data.
 * Inputs        : link             - link ID
 *                 data_p           - pointer to SDU
 *                 size             - SDU size
 * Outputs       : return value     - true  receiver is ready for more data
 *                                  - false receiver is now busy
 * Assumptions   : link is up
 * ------------------------------------------------------------------------- */
bool App_Hdlc_DataInd(uint_fast8_t link,
                      const uint8_t *data_p, uint_fast16_t size)
{
    return DataInd(&current_msg, data_p, size);
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Hdlc_DataCfm(uint_fast8_t   link,
 *                                       const uint8_t *data_p)
 * ----------------------------------------------------------------------------
 * Description   : Confirms sending data and acknowledging it by peer.
 * Inputs        : link             - link ID
 *                 data_p           - pointer to data
 * Outputs       : None
 * Assumptions   : link is up
 * ------------------------------------------------------------------------- */
void App_Hdlc_DataCfm(uint_fast8_t link, const uint8_t *data_p)
{
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Dfu_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Dfu_Init(void)
{
    image_download.state = PROG_SUCCESS;

    MsgHandler_Add(SYS_MAN_STATE_CHANGE_IND, StateChangeHandler);
    // 超时函数，广播超时以后，进行重新启动
    MsgHandler_Add(APP_DFU_SUPERVISOR_TIMER, TimeoutHandler);
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Dfu_Poll(void)
 * ----------------------------------------------------------------------------
 * Description   : Poll routine for handling the background task.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Dfu_Poll(void)
{
    if (ProgramImage(&current_msg, &image_download))
    {
        Drv_Targ_SetBackgroundFlag();
    }
}
