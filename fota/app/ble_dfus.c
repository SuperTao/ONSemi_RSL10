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
 * ble_dfus.c
 * - Custom Bluetooth DFU server source file
 * ----------------------------------------------------------------------------
 * $Revision: 1.3 $
 * $Date: 2019/03/15 21:11:59 $
 * ------------------------------------------------------------------------- */

#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_dfus.h"
#include "msg_handler.h"
#include "sys_fota.h"
#include "sys_boot.h"

/* ----------------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------------- */

/* switches on/off optional DFU characteristics */
#define SHOW_DEVID                  1
#define SHOW_BOOTVER                0
#define SHOW_STACKVER               1
#define SHOW_APPVER                 1
#define SHOW_BUILDID                0

#define DFU_ENTER_DELAY             0.1


#define  CS_CHAR_TEXT_DESC(idx, text)   \
    CS_CHAR_USER_DESC(idx, sizeof(text) - 1, text, NULL)


extern const Sys_Fota_version_t Sys_Boot_app_version;

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    DFU_ENTER_TIMEOUT = TASK_FIRST_MSG(TASK_ID_APP) + 250,
    DFU_DISCONNECT
} dfu_msg_id_t;

typedef enum
{
    /* Service DFU */
    DFU_SVC,
#if (SHOW_DEVID)
    /* Device ID Characteristic in Service DFU */
    DFU_DEVID_CHAR,
    DFU_DEVID_VAL,
    DFU_DEVID_NAME,
#endif
#if (SHOW_BOOTVER)
    /* BootLoader Version Characteristic in Service DFU */
    DFU_BOOTVER_CHAR,
    DFU_BOOTVER_VAL,
    DFU_BOOTVER_NAME,
#endif
#if (SHOW_STACKVER)
    /* BLE Stack Version Characteristic in Service DFU */
    DFU_STACKVER_CHAR,
    DFU_STACKVER_VAL,
    DFU_STACKVER_NAME,
#endif
#if (SHOW_APPVER)
    /* Application Version Characteristic in Service DFU */
    DFU_APPVER_CHAR,
    DFU_APPVER_VAL,
    DFU_APPVER_NAME,
#endif
#if (SHOW_BUILDID)
    /* Build ID Characteristic in Service DFU */
    DFU_BUILDID_CHAR,
    DFU_BUILDID_VAL,
    DFU_BUILDID_NAME,
#endif
    /* Enter mode Characteristic in Service DFU */
    DFU_ENTER_CHAR,
    DFU_ENTER_VAL,
    DFU_ENTER_NAME,

    /* Max number of services and characteristics */
    DFU_ATT_NB
} dfu_att_t;

/* ----------------------------------------------------------------------------
 * Function      : uint8_t DfusCallback(uint8_t conidx, uint16_t attidx,
 *                       uint16_t handle, uint8_t *toData,
 *                       const uint8_t *fromData, uint16_t lenData,
 *                       uint16_t operation)
 * ----------------------------------------------------------------------------
 * Description   : User callback data access function for the DFU
 *                 characteristics. This function is called by the BLE
 *                 abstraction whenever a ReadReqInd or WriteReqInd occurs in
 *                 the specified attribute. The callback is linked to the
 *                 attribute in the database construction (see att_db).
 * Inputs        : - conidx    - connection index
 *                 - attidx    - attribute index in the user defined database
 *                 - handle    - attribute handle allocated in the BLE stack
 *                 - to        - pointer to destination buffer
 *                 - from      - pointer to source buffer
 *                               "to" and "from" may be a pointer to the
 *                               'att_db' characteristic buffer or the BLE
 *                               stack buffer, depending if the "operation" is
 *                               a write or a read.
 *                 - length    - length of data to be copied
 *                 - operation - GATTC_READ_REQ_IND or GATTC_WRITE_REQ_IND
 * Outputs       : return value - read or write  status (one of enum hl_err)
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static uint8_t DfusCallback(uint8_t conidx, uint16_t attidx, uint16_t handle,
                            uint8_t *toData, const uint8_t *fromData,
                            uint16_t lenData,  uint16_t operation)
{
    if (operation == GATTC_WRITE_REQ_IND)
    {
        switch (attidx)
        {
            case DFU_ENTER_VAL:
            {
               if (*fromData != 1)
               {
                   return GATT_ERR_WRITE;
               }
               ke_msg_send_basic(DFU_DISCONNECT,
                                 KE_BUILD_ID(TASK_APP, conidx),
                                 KE_BUILD_ID(TASK_APP, conidx));
            }
            break;
        }
    }
    else if (operation == GATTC_READ_REQ_IND)
    {
        switch (attidx)
        {
        #if (SHOW_DEVID)
            case DFU_DEVID_VAL:
            {
                const Sys_Fota_version_t *version;
                version = (const Sys_Fota_version_t *)Sys_Boot_GetVersion(APP_BASE_ADR);
                memcpy(toData, &version->dev_id, lenData);
            }
            break;
        #endif
        #if (SHOW_BOOTVER)
            case DFU_BOOTVER_VAL:
            {
                memcpy(toData, Sys_Boot_GetVersion(BOOT_BASE_ADR), lenData);
            }
            break;
        #endif
        #if (SHOW_STACKVER)
            case DFU_STACKVER_VAL:
            {
                memcpy(toData, Sys_Boot_GetVersion(APP_BASE_ADR), lenData);
            }
            break;
        #endif
        #if (SHOW_APPVER)
            case DFU_APPVER_VAL:
            {
                memcpy(toData, &Sys_Boot_app_version.app_id, lenData);
            }
            break;
        #endif
        #if (SHOW_BUILDID)
            case DFU_BUILDID_VAL:
            {
                memcpy(toData,
                       Sys_Boot_GetDscr(APP_BASE_ADR)->build_id_a, lenData);
            }
            break;
        #endif
        }
    }
    return ATT_ERR_NO_ERROR;
}

/* ----------------------------------------------------------------------------
 * Function      : void DFUS_Start(void)
 * ----------------------------------------------------------------------------
 * Description   : Start the DFU Server
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void DFUS_Start(void)
{
    static const struct att_db_desc dfu_att_db[DFU_ATT_NB] =
    {
        /* Service DFU */
        CS_SERVICE_UUID_128(DFU_SVC, SYS_FOTA_DFU_SVC_UUID),
    #if (SHOW_DEVID)
        /* Device ID Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_DEVID_CHAR, DFU_DEVID_VAL, SYS_FOTA_DFU_DEVID_UUID,
                         PERM(RD, ENABLE),
                         16, NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_DEVID_NAME, "Device ID"),
    #endif
    #if (SHOW_BOOTVER)
        /* BootLoader Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_BOOTVER_CHAR, DFU_BOOTVER_VAL, SYS_FOTA_DFU_BOOTVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(Sys_Boot_app_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_BOOTVER_NAME, "Bootloader Version"),
    #endif
    #if (SHOW_STACKVER)
        /* BLE Stack Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_STACKVER_CHAR, DFU_STACKVER_VAL, SYS_FOTA_DFU_STACKVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(Sys_Boot_app_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_STACKVER_NAME, "BLE Stack Version"),
    #endif
    #if (SHOW_APPVER)
        /* Application Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_APPVER_CHAR, DFU_APPVER_VAL, SYS_FOTA_DFU_APPVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(Sys_Boot_app_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_APPVER_NAME, "Application Version"),
    #endif
    #if (SHOW_BUILDID)
        /* Build ID Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_BUILDID_CHAR, DFU_BUILDID_VAL, SYS_FOTA_DFU_BUILDID_UUID,
                         PERM(RD, ENABLE),
                         32, NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_BUILDID_NAME, "BLE Stack Build ID"),
    #endif
        /* Enter mode Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_ENTER_CHAR, DFU_ENTER_VAL, SYS_FOTA_DFU_ENTER_UUID,
                         PERM(WRITE_REQ, ENABLE),
                         1, NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_ENTER_NAME, "Enter DFU mode")
    };

    GATTM_AddAttributeDatabase(dfu_att_db, DFU_ATT_NB);
}

/* ----------------------------------------------------------------------------
 * Function      : void Dfus_MsgHandler(ke_msg_id_t const msg_id,
 *                                      void const *param,
 *                                      ke_task_id_t const dest_id,
 *                                      ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle all events related to the DFU service
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void Dfus_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                            ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (msg_id)
    {
        case GAPM_CMP_EVT:
        {
            const struct gapm_cmp_evt *p = param;

            if (p->operation == GAPM_SET_DEV_CONFIG)
            {
                DFUS_Start();
            }
        }
        break;
        case DFU_DISCONNECT:
        {
            GAPC_DisconnectAll(CO_ERROR_REMOTE_USER_TERM_CON);
            ke_timer_set(DFU_ENTER_TIMEOUT, dest_id,
                         KE_TIME_IN_SEC(DFU_ENTER_DELAY));
        }
        break;
        case DFU_ENTER_TIMEOUT:
        {
            Sys_Fota_StartDfu(1);
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void DFUS_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize the DFU Server
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void DFUS_Initialize(void)
{
    MsgHandler_Add(GAPM_CMP_EVT, Dfus_MsgHandler);
    MsgHandler_Add(DFU_ENTER_TIMEOUT, Dfus_MsgHandler);
    MsgHandler_Add(DFU_DISCONNECT, Dfus_MsgHandler);
}
