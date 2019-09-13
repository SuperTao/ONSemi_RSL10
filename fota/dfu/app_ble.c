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
 * app_ble.c
 * - This is the DFU component. It is responsible for updating the system with
 *   new firmware.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <rsl10.h>
#include <rsl10_protocol.h>

#include "app_ble.h"
#include "app_conf.h"
#include "app_trace.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "sys_man.h"
#include "sys_fota.h"
#include "msg_handler.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define MAX_ADV_NAME_LEN            29

#define UNKNOWN_APPEARANCE          0

#define DEFAULT_MAX_DATA_SIZE       20
#define MAX_DATA_SIZE               240
#define DATA_SIZE_OVERHEAD          7
#if (CFG_BLE_MAX_DATA_SIZE < DEFAULT_MAX_DATA_SIZE || CFG_BLE_MAX_DATA_SIZE > MAX_DATA_SIZE)
    #error CFG_BLE_MAX_DATA_SIZE must be in the range 20...240
#endif /* if (CFG_BLE_MAX_DATA_SIZE < DEFAULT_MAX_DATA_SIZE || CFG_BLE_MAX_DATA_SIZE > MAX_DATA_SIZE) */

#define PREF_SLV_MIN_CON_INTERVAL   6
#define PREF_SLV_MAX_CON_INTERVAL   12
#define PREF_SLV_LATENCY            0
#define PREF_SLV_SUP_TIMEOUT        200
#define PREF_GAPM_TX_OCT_MAX        (DATA_SIZE_OVERHEAD + DEFAULT_MAX_DATA_SIZE)
#define PREF_GAPM_TX_TIME_MAX       (14 * 8 + PREF_GAPM_TX_OCT_MAX * 8)


#define  CS_CHAR_TEXT_DESC(idx, text)   \
    CS_CHAR_USER_DESC(idx, sizeof(text) - 1, text, NULL)


/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    /* Service DFU */
    DFU_SVC,

    /* Data Characteristic in Service DFU */
    DFU_DATA_CHAR,
    DFU_DATA_VAL,
    DFU_DATA_CCC,

    /* Device ID Characteristic in Service DFU */
    DFU_DEVID_CHAR,
    DFU_DEVID_VAL,
    DFU_DEVID_NAME,

    /* BootLoader Version Characteristic in Service DFU */
    DFU_BOOTVER_CHAR,
    DFU_BOOTVER_VAL,
    DFU_BOOTVER_NAME,

    /* BLE Stack Version Characteristic in Service DFU */
    DFU_STACKVER_CHAR,
    DFU_STACKVER_VAL,
    DFU_STACKVER_NAME,

    /* Application Version Characteristic in Service DFU */
    DFU_APPVER_CHAR,
    DFU_APPVER_VAL,
    DFU_APPVER_NAME,

    /* Build ID Characteristic in Service DFU */
    DFU_BUILDID_CHAR,
    DFU_BUILDID_VAL,
    DFU_BUILDID_NAME,

    /* Max number of services and characteristics */
    DFU_ATT_NB
} dfu_att_t;

static const bd_addr_t fallbackAddr = { CFG_FALLBACK_ADDR };

static const union gapc_dev_info_val dev_appearance =
{
    .appearance = UNKNOWN_APPEARANCE
};

static const union gapc_dev_info_val dev_slv_params =
{
    .slv_params =
    {
        PREF_SLV_MIN_CON_INTERVAL,
        PREF_SLV_MAX_CON_INTERVAL,
        PREF_SLV_LATENCY,
        PREF_SLV_SUP_TIMEOUT
    }
};

struct gapm_start_advertise_cmd advertiseCmd =
{
    .op =
    {
        .code = GAPM_ADV_UNDIRECT,
        //.addr_src = GAPM_GEN_NON_RSLV_ADDR, //Why does this not work?
        .addr_src = GAPM_STATIC_ADDR,
    },
    .intv_min = GAPM_DEFAULT_ADV_INTV_MIN,
    .intv_max = GAPM_DEFAULT_ADV_INTV_MAX,
    .channel_map = GAPM_DEFAULT_ADV_CHMAP,
    .info.host =
    {
        .mode = GAP_GEN_DISCOVERABLE,
        .adv_filt_policy = ADV_ALLOW_SCAN_ANY_CON_ANY
        /* ADV_DATA and SCAN_RSP data are set in SetAdvScanData() */
    }
};

static uint16_t dfu_ccc_value;
static uint16_t dfu_link_max_size;

/* ----------------------------------------------------------------------------
 * Function      : AdvScanSetup(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the advertisement data structure
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void AdvScanSetup(void)
{
    uint8_t tx_pwr    = (uint8_t)CFG_RF_POWER_DBM;
    App_Conf_uuid_t     *uuid_p    = App_Conf_GetServiceId();
    App_Conf_dev_name_t *name_p    = App_Conf_GetDeviceName();
    uint_fast16_t name_len  = name_p->length;
    enum gap_ad_type name_type = GAP_AD_TYPE_COMPLETE_NAME;

    /* limit advertised device name length */
    if (name_len > MAX_ADV_NAME_LEN)
    {
        name_len  = MAX_ADV_NAME_LEN;
        name_type = GAP_AD_TYPE_SHORTENED_NAME;
    }

    /* Set advertising data */
    advertiseCmd.info.host.adv_data_len = 0;
    GAPM_AddAdvData(GAP_AD_TYPE_TRANSMIT_POWER,
                    &tx_pwr, sizeof(tx_pwr),
                    advertiseCmd.info.host.adv_data,
                    &advertiseCmd.info.host.adv_data_len);
    GAPM_AddAdvData(GAP_AD_TYPE_MORE_128_BIT_UUID,
                    *uuid_p, sizeof(*uuid_p),
                    advertiseCmd.info.host.adv_data,
                    &advertiseCmd.info.host.adv_data_len);

    /* Set scan response data */
    advertiseCmd.info.host.scan_rsp_data_len = 0;
    GAPM_AddAdvData(name_type,
                    name_p->value_a, name_len,
                    advertiseCmd.info.host.scan_rsp_data,
                    &advertiseCmd.info.host.scan_rsp_data_len);
}

/* ----------------------------------------------------------------------------
 * Function      : StartAdvertising(ke_task_id_t id)
 * ----------------------------------------------------------------------------
 * Description   : Start advertising
 * Inputs        : id               - application task ID
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void StartAdvertising(ke_task_id_t id)
{
    GAPM_StartAdvertiseCmd(&advertiseCmd);
    Sys_Man_SetAppState(id, APP_BLE_ADVERTISING);
}

/* ----------------------------------------------------------------------------
 * Function      : GapcParamUpdateCmd(uint_fast8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Send parameter update command
 * Inputs        : conidx       - Connection index
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void GapcParamUpdateCmd(uint_fast8_t conidx)
{
    struct gapc_param_update_cmd *cmd;

    cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD, KE_BUILD_ID(TASK_GAPC, conidx),
                       TASK_APP, gapc_param_update_cmd);

    /* Perform update of connection parameters. */
    cmd->operation = GAPC_UPDATE_PARAMS;

    /* Internal parameter used to manage internally L2CAP packet identifier for signaling */
    cmd->pkt_id = 0;

    /* Connection interval minimum */
    cmd->intv_min = dev_slv_params.slv_params.con_intv_min;

    /* Connection interval maximum */
    cmd->intv_max = dev_slv_params.slv_params.con_intv_max;

    /* Latency */
    cmd->latency  = dev_slv_params.slv_params.slave_latency;

    /* Supervision timeout */
    cmd->time_out = dev_slv_params.slv_params.conn_timeout;

    /* Minimum Connection Event Duration */
    cmd->ce_len_min = 0;

    /* Maximum Connection Event Duration */
    cmd->ce_len_max = 0;

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : GapcConnectionCfm(uint_fast8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Sends the GAPC_CONNECTION_CFM message
 * Inputs        : - conidx     - Client index
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void GapcConnectionCfm(uint_fast8_t conidx)
{
    struct gapc_connection_cfm cfm =
    {
        .svc_changed_ind_enable = 0,
        .ltk_present = false,
        .auth = GAP_AUTH_REQ_NO_MITM_NO_BOND
    };

    /* Init link state */
    dfu_ccc_value     = ATT_CCC_STOP_NTFIND;
    dfu_link_max_size = DEFAULT_MAX_DATA_SIZE;

    GAPC_ConnectionCfm(conidx, &cfm);
    Sys_Man_SetAppState(KE_BUILD_ID(TASK_APP, conidx), APP_BLE_CONNECTED);
}

/* ----------------------------------------------------------------------------
 * Function      : GapmSetDevConfigCmd(void)
 * ----------------------------------------------------------------------------
 * Description   : Sets the device configuration
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void GapmSetDevConfigCmd(void)
{
    struct gapm_set_dev_config_cmd devConfig =
    {
        .operation = GAPM_SET_DEV_CONFIG,
        .role = GAP_ROLE_PERIPHERAL,
        .renew_dur = GAPM_DEFAULT_RENEW_DUR,
        .addr_type = GAPM_CFG_ADDR_PRIVATE,
        .pairing_mode = GAPM_PAIRING_DISABLE,
        .gap_start_hdl = GAPM_DEFAULT_GAP_START_HDL,
        .gatt_start_hdl = GAPM_DEFAULT_GATT_START_HDL,
        .att_and_ext_cfg = GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN,
        .sugg_max_tx_octets = PREF_GAPM_TX_OCT_MAX,
        .sugg_max_tx_time = PREF_GAPM_TX_TIME_MAX,
        .max_mtu = GAPM_DEFAULT_MTU_MAX,
        .max_mps = GAPM_DEFAULT_MPS_MAX,
        .max_nb_lecb = GAPM_DEFAULT_MAX_NB_LECB,
        .audio_cfg = GAPM_DEFAULT_AUDIO_CFG,
        .tx_pref_rates = GAP_RATE_ANY,
        .rx_pref_rates = GAP_RATE_LE_2MBPS
    };
    uint8_t mac_index;
    uint8_t reverse_addr[GAP_BD_ADDR_LEN] = {0};

    /* try to load BLE address from NVR3 */
    if (!Device_Param_Read(PARAM_ID_PUBLIC_BLE_ADDRESS, reverse_addr))
    {
        /* fallback to static address */
        memcpy(devConfig.addr.addr, fallbackAddr.addr, sizeof(devConfig.addr.addr));
    }

    for (mac_index = 0; mac_index < GAP_BD_ADDR_LEN; mac_index++)
    {
        devConfig.addr.addr[mac_index] = reverse_addr[GAP_BD_ADDR_LEN - 1 - mac_index];
    }
    /* force non-resolvable private address */
//    devConfig.addr.addr[GAP_BD_ADDR_LEN - 1] &= 0x3F;
    GAPM_SetDevConfigCmd(&devConfig);
}

/* ----------------------------------------------------------------------------
 * Function      : uint8_t DfusCallback(uint8_t conidx, uint16_t attidx,
 *                                     uint16_t handle, uint8_t *toData,
 *                                     const uint8_t *fromData,
 *                                     uint16_t lenData, uint16_t operation)
 * ----------------------------------------------------------------------------
 * Description   : DFU Server callback function
 * Inputs        : None
 * Outputs       : None
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
            case DFU_DATA_VAL:
            {
            	// 发送固件传到这里进行处理
                if (dfu_ccc_value)
                {
                    App_Ble_DataInd(conidx, fromData, lenData);
                }
            }
            break;

            case DFU_DATA_CCC:
            {
                uint_fast16_t prev_ccc_value = dfu_ccc_value;

                memcpy(&dfu_ccc_value, fromData, lenData);
                dfu_ccc_value &= ATT_CCC_START_NTF;

                /* on notification status change invoke link activation/deactivation */
                if (prev_ccc_value ^ dfu_ccc_value)
                {
                    if (dfu_ccc_value)
                    {
                    	// 更新连接参数
                        /* Update connection parameters on change of the DFU data CCC */
                        GapcParamUpdateCmd(conidx);
                        App_Ble_ActivationInd(conidx, dfu_link_max_size);
                        Sys_Man_SetAppState(KE_BUILD_ID(TASK_APP, conidx), APP_BLE_LINKUP);
                    }
                    else
                    {
                        App_Ble_DeactivationInd(conidx);
                        Sys_Man_SetAppState(KE_BUILD_ID(TASK_APP, conidx), APP_BLE_CONNECTED);
                    }
                }
            }
            break;
        }
    }
    else if (operation == GATTC_READ_REQ_IND)
    {
        switch (attidx)
        {
            case DFU_DATA_CCC:
            {
                memcpy(toData, &dfu_ccc_value, lenData);
            }
            break;

            case DFU_DEVID_VAL:
            {
                memcpy(toData, App_Conf_GetDeviceID(), lenData);
            }
            break;

            case DFU_BOOTVER_VAL:
            {
                memcpy(toData,
                       App_Conf_GetVersion(APP_CONF_BOOT_VERSION), lenData);
            }
            break;

            case DFU_STACKVER_VAL:
            {
                memcpy(toData,
                       App_Conf_GetVersion(APP_CONF_STACK_VERSION), lenData);
            }
            break;

            case DFU_APPVER_VAL:
            {
                App_Conf_version_t *version_p =
                    App_Conf_GetVersion(APP_CONF_APP_VERSION);

                if (version_p != NULL)
                {
                    memcpy(toData, version_p, lenData);
                }
                else
                {
                    memset(toData, 0, lenData);
                }
            }
            break;

            case DFU_BUILDID_VAL:
            {
                memcpy(toData, App_Conf_GetBuildID(), lenData);
            }
            break;
        }
    }
    return ATT_ERR_NO_ERROR;
}

/* ----------------------------------------------------------------------------
 * Function      : DfusSetup(void)
 * ----------------------------------------------------------------------------
 * Description   : Setup the DFU Server
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void DfusSetup(void)
{
    static const struct att_db_desc dfu_scv_db[DFU_ATT_NB] =
    {
        /* Service DFU */
        CS_SERVICE_UUID_128(DFU_SVC, SYS_FOTA_DFU_SVC_UUID),

        /* Data Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_DATA_CHAR, DFU_DATA_VAL, SYS_FOTA_DFU_DATA_UUID,
                         PERM(NTF, ENABLE) | PERM(WRITE_COMMAND, ENABLE),
                         GAPM_DEFAULT_MTU_MAX, NULL,  DfusCallback),

        /* Client Characteristic Configuration */
        CS_CHAR_CCC(DFU_DATA_CCC, &dfu_ccc_value, DfusCallback),

        /* Device ID Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_DEVID_CHAR, DFU_DEVID_VAL, SYS_FOTA_DFU_DEVID_UUID,
                         PERM(RD, ENABLE),
                         sizeof(App_Conf_uuid_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_DEVID_NAME, "Device ID"),

        /* BootLoader Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_BOOTVER_CHAR, DFU_BOOTVER_VAL, SYS_FOTA_DFU_BOOTVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(App_Conf_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_BOOTVER_NAME, "Bootloader Version"),

        /* BLE Stack Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_STACKVER_CHAR, DFU_STACKVER_VAL, SYS_FOTA_DFU_STACKVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(App_Conf_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_STACKVER_NAME, "BLE Stack Version"),

        /* Application Version Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_APPVER_CHAR, DFU_APPVER_VAL, SYS_FOTA_DFU_APPVER_UUID,
                         PERM(RD, ENABLE),
                         sizeof(App_Conf_version_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_APPVER_NAME, "Application Version"),

        /* Build ID Characteristic in Service DFU */
        CS_CHAR_UUID_128(DFU_BUILDID_CHAR, DFU_BUILDID_VAL, SYS_FOTA_DFU_BUILDID_UUID,
                         PERM(RD, ENABLE),
                         sizeof(App_Conf_build_id_t), NULL, DfusCallback),
        CS_CHAR_TEXT_DESC(DFU_BUILDID_NAME, "BLE Stack Build ID")
    };

    GATTM_AddAttributeDatabase(dfu_scv_db, DFU_ATT_NB);
}

/* ----------------------------------------------------------------------------
 * Function      : GapmGattmHandler(ke_msg_id_t msg_id, const void *param,
 *                                  ke_task_id_t dest_id, ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle GAPM/GATTM messages that need application action
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void GapmGattmHandler(ke_msg_id_t msg_id, const void *param,
                             ke_task_id_t dest_id, ke_task_id_t src_id)
{
    uint8_t conidx = KE_IDX_GET(dest_id);

    switch (msg_id)
    {
        case GAPM_DEVICE_READY_IND:
        {
            /* Reset the GAP manager. Trigger GAPM_CMP_EVT / GAPM_RESET when finished. */
            GAPM_ResetCmd();
        }
        break;

        case GAPM_CMP_EVT:
        {
            const struct gapm_cmp_evt *p = param;

            /* Reset completed. Apply device configuration. */
            if (p->operation == GAPM_RESET)
            {
                GapmSetDevConfigCmd();

                /* Trigger a GAPM_CMP_EVT / GAPM_SET_DEV_CONFIG when finished. */
            }
            else if (p->operation == GAPM_SET_DEV_CONFIG &&
                     p->status    == GAP_ERR_NO_ERROR)
            {
                /* Add DFU Server */
                DfusSetup();
            }
            else if (p->operation == GAPM_RESOLV_ADDR &&    /* IRK not found for address */
                     p->status    == GAP_ERR_NOT_FOUND)
            {
                GapcConnectionCfm(conidx);    /* Confirm connection without LTK. */
            }
        }
        break;

        case GAPM_ADDR_SOLVED_IND:    /* Private address resolution was successful */
        {
            GapcConnectionCfm(conidx);    /* Send connection confirmation with LTK */
        }
        break;

        case GATTM_ADD_SVC_RSP:
        {
            /* If all expected profiles/services have been added */
            if (GATTM_GetServiceAddedCount() == 1)
            {
                StartAdvertising(KE_BUILD_ID(TASK_APP, conidx));
            }
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : GapcGattcHandler(ke_msg_id_t msg_id, const void *param,
 *                                  ke_task_id_t dest_id, ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle GAPC/GATTC messages that need application action
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
static void GapcGattcHandler(ke_msg_id_t msg_id, const void *param,
                             ke_task_id_t dest_id, ke_task_id_t src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    switch (msg_id)
    {
        case GATTC_CMP_EVT:
        {
            const struct gattc_cmp_evt *p = param;
            App_Ble_status_t status;

            /* Sending notification processing is completed. */
            if (p->operation == GATTC_NOTIFY)
            {
                if (p->status == ATT_ERR_NO_ERROR)
                {
                    status = APP_BLE_SUCCESS;
                }
                else
                {
                    status = APP_BLE_FAILURE;
                }
                App_Ble_DataCfm(conidx, p->seq_num, status);
            }
        }
        break;

        case GAPC_CONNECTION_REQ_IND:
        {
            GapcConnectionCfm(conidx);
        }
        break;

        case GAPC_DISCONNECT_IND:
        {
            Sys_Man_SetAppState(KE_BUILD_ID(TASK_APP, conidx), APP_BLE_DISCONNECTED);
            //StartAdvertising(KE_BUILD_ID(TASK_APP, conidx));
        }
        break;

        case GAPC_LE_PKT_SIZE_IND:
        {
            const struct gapc_le_pkt_size_ind *p = param;
            dfu_link_max_size = p->max_tx_octets - DATA_SIZE_OVERHEAD;
        }
        break;

        case GAPC_GET_DEV_INFO_REQ_IND:
        {
            const struct gapc_get_dev_info_req_ind *p = param;
            const union gapc_dev_info_val *info_p;

            switch (p->req)
            {
                case GAPC_DEV_NAME:
                {
                    info_p = (const union gapc_dev_info_val *)App_Conf_GetDeviceName();
                }
                break;

                case GAPC_DEV_APPEARANCE:
                {
                    info_p = &dev_appearance;
                }
                break;

                case GAPC_DEV_SLV_PREF_PARAMS:
                {
                    info_p = &dev_slv_params;
                }
                break;

                default:
                    return;
            }
            GAPC_GetDevInfoCfm(conidx, p->req, info_p);
        }
        break;

        case GAPC_SET_DEV_INFO_REQ_IND:
        {
            GAPC_SetDevInfoCfm(conidx, param, NULL, 0);
        }
        break;

        case GAPC_PARAM_UPDATE_REQ_IND:
        {
            GAPC_ParamUpdateCfm(conidx, true, 0xFFFF, 0xFFFF);
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Ble_Init(void)
{
    /* Initialize the baseband manager */
    BLE_InitNoTL(RESET_NO_ERROR);

    /* Enable the needed Bluetooth interrupts */
    NVIC->ISER[1] = (NVIC_BLE_CSCNT_INT_ENABLE      |
                     NVIC_BLE_SLP_INT_ENABLE        |
                     NVIC_BLE_RX_INT_ENABLE         |
                     NVIC_BLE_EVENT_INT_ENABLE      |
                     NVIC_BLE_CRYPT_INT_ENABLE      |
                     NVIC_BLE_ERROR_INT_ENABLE      |
                     NVIC_BLE_GROSSTGTIM_INT_ENABLE |
                     NVIC_BLE_FINETGTIM_INT_ENABLE  |
                     NVIC_BLE_SW_INT_ENABLE);

    /* Configure application-specific advertising data and scan response data.
     * The advertisement period will change after 30 s as per 5.1.1 */
    AdvScanSetup();

    /* Add application message handlers */
    MsgHandler_Add(GATTC_CMP_EVT, GapcGattcHandler);
    MsgHandler_Add(TASK_ID_GAPC, GapcGattcHandler);
    MsgHandler_Add(GATTM_ADD_SVC_RSP, GapmGattmHandler);
    MsgHandler_Add(TASK_ID_GAPM, GapmGattmHandler);
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_StartAdvertising(ke_task_id_t id)
 * ----------------------------------------------------------------------------
 * Description   : Starts BLE advertising.
 * Inputs        : id               - application task ID
 * Outputs       : None
 * Assumptions   : Only valid in state APP_BLE_DISCONNECTED
 * ------------------------------------------------------------------------- */
void App_Ble_StartAdvertising(ke_task_id_t id)
{
    if (Sys_Man_GetAppState(id) == APP_BLE_DISCONNECTED)
    {
        StartAdvertising(id);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DataReq(uint_fast8_t   link,
 *                                      uint_fast16_t  seq_nb,
 *                                      const uint8_t *data_p,
 *                                      uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Requests sending data
 * Inputs        : link             - link ID
 * Inputs        : seq_nb           - sequence number
 * Inputs        : data_p           - pointer to data
 * Inputs        : size             - size of data
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DataReq(uint_fast8_t link, uint_fast16_t seq_nb,
                     const uint8_t *data_p, uint_fast16_t size)
{
    if (dfu_ccc_value)
    {
        GATTC_SendEvtCmd(link, GATTC_NOTIFY, seq_nb,
                         GATTM_GetHandle(DFU_DATA_VAL),
                         size, data_p);
    }
    else
    {
        App_Ble_DataCfm(link, seq_nb, APP_BLE_LINK_DOWN);
    }
}
