/* ----------------------------------------------------------------------------
 * Copyright (c) 2015-2017 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * Copyright (C) RivieraWaves 2009-2016
 *
 * This module is derived in part from example code provided by RivieraWaves
 * and as such the underlying code is the property of RivieraWaves [a member
 * of the CEVA, Inc. group of companies], together with additional code which
 * is the property of ON Semiconductor. The code (in whole or any part) may not
 * be redistributed in any form without prior written permission from
 * ON Semiconductor.
 *
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * ble_gap.c
 * - BLE GAP layer abstraction source
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */
#include "config.h"
#include <ble_gap.h>
#include <ble_gatt.h>
#include <rsl10_protocol.h>
#include <rsl10_flash_rom.h>
#include <app_trace.h>

/* GAP Environment Structure */
static GAP_Env_t gap_env;

/* ----------------------------------------------------------------------------
 * Function      : void GAP_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize the GAP environment
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAP_Initialize(void)
{
    memset(&gap_env, 0, sizeof(GAP_Env_t));
    gap_env.gapmState = GAPM_STATE_INIT;

    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        gap_env.connection[i].conhdl = GAP_INVALID_CONHDL;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : const GAP_Env_t* GAP_GetEnv(void)
 * ----------------------------------------------------------------------------
 * Description   : Return a reference to the internal environment structure.
 * Inputs        : None
 * Outputs       : A constant pointer to GAP_Env_t
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const GAP_Env_t * GAP_GetEnv(void)
{
    return &gap_env;
}

/* ----------------------------------------------------------------------------
 * Function      : bool GAP_IsAddrPrivateResolvable(const uint8_t *addr,
 *                                                  uint8_t addrType)
 * ----------------------------------------------------------------------------
 * Description   : Check if address is private resolvable
 * Inputs        : None
 * Outputs       : true if address is private resolvable, false otherwise.
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool GAP_IsAddrPrivateResolvable(const uint8_t *addr, uint8_t addrType)
{
    return ((addrType == GAPM_GEN_RSLV_ADDR) &&
            ((addr[GAP_BD_ADDR_LEN - 1] & 0xc0) == GAP_RSLV_ADDR));
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_ResetCmd(void)
 * ----------------------------------------------------------------------------
 * Description   : Reset the GAP manager. Trigger a GAPM_CMP_EVT / GAPM_RESET
 *                 when finished.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_ResetCmd(void)
{
    struct gapm_reset_cmd *cmd;

    cmd = KE_MSG_ALLOC(GAPM_RESET_CMD, TASK_GAPM, TASK_APP, gapm_reset_cmd);
    cmd->operation = GAPM_RESET;
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_CancelCmd(void)
 * ----------------------------------------------------------------------------
 * Description   : Cancel an ongoing air operation such as scanning,
 *                 advertising or connecting. It has no impact on other
 *                 commands.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_CancelCmd(void)
{
    struct gapm_cancel_cmd *cmd;

    cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD, TASK_GAPM, TASK_APP, gapm_cancel_cmd);
    cmd->operation = GAPM_CANCEL;
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_SetDevConfigCmd(const struct
 *                                       gapm_set_dev_config_cmd *deviceConfig)
 * ----------------------------------------------------------------------------
 * Description   : Set device configuration. This command should be executed
 *                 after GAPM_Reset().
 * Inputs        : deviceConfig - Device configuration.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_SetDevConfigCmd(const struct gapm_set_dev_config_cmd *deviceConfig)
{
    struct gapm_set_dev_config_cmd *cmd;
    app_device_param_t param;

    cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD, TASK_GAPM, TASK_APP,
                       gapm_set_dev_config_cmd);

    /* Save a local copy of the configuration */
    memcpy(&gap_env.deviceConfig, deviceConfig, sizeof(struct gapm_set_dev_config_cmd));
    Device_Param_Prepare(&param);
    if (param.device_param_src_type == FLASH_PROVIDED_or_DFLT)
    {
        memcpy(gap_env.deviceConfig.irk.key, (uint8_t *)DEVICE_INFO_BLUETOOTH_IRK, KEY_LEN);

        /* If FLASH_PROVIDED_or_DFLT and address type is public, BLE stack will use the value
         * from flash. So, copy it in the local variable to be consistent */
        if (deviceConfig->addr_type == GAPM_CFG_ADDR_PUBLIC)
        {
            memcpy(gap_env.deviceConfig.addr.addr, (uint8_t *)DEVICE_INFO_BLUETOOTH_ADDR, BD_ADDR_LEN);
        }
    }

    /* Send configuration to stack */
    memcpy(cmd, &gap_env.deviceConfig, sizeof(struct gapm_set_dev_config_cmd));
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_StartConnectionCmd(const struct
 *                               gapm_start_connection_cmd *startConnectionCmd)
 * ----------------------------------------------------------------------------
 * Description   : Send a command to establish a connection with peer devices
 * Inputs        : startConnectionCmd - Pointer to command arguments.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_StartConnectionCmd(const struct gapm_start_connection_cmd
                             *startConnectionCmd)
{
    struct gapm_start_connection_cmd *cmd;

    gap_env.gapmState = GAPM_STATE_STARTING_CONNECTION;

    cmd = KE_MSG_ALLOC_DYN(GAPM_START_CONNECTION_CMD, TASK_GAPM, TASK_APP,
                           gapm_start_connection_cmd,
                           (startConnectionCmd->nb_peers * sizeof(struct gap_bdaddr)));

    memcpy(cmd, startConnectionCmd, (sizeof(struct gapm_start_connection_cmd) +
                                     (startConnectionCmd->nb_peers * sizeof(struct gap_bdaddr))));

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_StartAdvertiseCmd(const struct
 *                                       gapm_start_advertise_cmd *startAdvCmd)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPM_START_ADVERTISE_CMD to the stack
 * Inputs        : startAdvCmd     - Advertisement params
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_StartAdvertiseCmd(const struct gapm_start_advertise_cmd *startAdvCmd)
{
    struct gapm_start_advertise_cmd *cmd;
    gap_env.gapmState = GAPM_STATE_ADVERTISING;

    cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD, TASK_GAPM, TASK_APP,
                       gapm_start_advertise_cmd);

    memcpy(cmd, startAdvCmd, sizeof(struct gapm_start_advertise_cmd));

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_ProfileTaskAddCmd(uint8_t sec_lvl,
 *                      uint16_t prf_task_id, uint16_t app_task,
 *                      uint16_t start_hdl, uint8_t* param, uint32_t paramSize)
 * ----------------------------------------------------------------------------
 * Description   : Add profile to the stack.
 * Inputs        : sec_lvl       - Security level (for example,
 *                                                 PERM(SVC_AUTH, DISABLE))
 *                 prf_task_id   - Task id to be added (example: TASK_ID_BASS)
 *                 app_task      - APP_TASK
 *                 start_hdl     - Start handle
 *                 param         - Dynamic profile param
 *                 paramSize     - Param size
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_ProfileTaskAddCmd(uint8_t sec_lvl, uint16_t prf_task_id,
                            uint16_t app_task, uint16_t start_hdl,
                            uint8_t *param, uint32_t paramSize)
{
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(
        GAPM_PROFILE_TASK_ADD_CMD,
        TASK_GAPM,
        app_task,
        gapm_profile_task_add_cmd,
        paramSize);

    req->operation   = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl     = sec_lvl;
    req->prf_task_id = prf_task_id;
    req->app_task    = app_task;
    req->start_hdl   = start_hdl;

    if (param && paramSize)
    {
        memcpy(req->param, param, paramSize);
    }

    ke_msg_send(req);
}

#if CFG_BOND_LIST_IN_NVR2
/* ----------------------------------------------------------------------------
 * Function      : void GAPM_ResolvAddrCmd(uint8_t conidx,
 *                          const uint8_t *peerAddr, uint8_t irkListSize,
 *                          const struct gap_sec_key *irkList)
 * ----------------------------------------------------------------------------
 * Description   : Send a request to resolve a private random resolvable
 *                 address using a provided list of Identity Resolution Keys
 *                 (IRKs).
 *                   - GAPM_ADDR_SOLVED_IND: triggered if the address is
 *                                           correctly resolved
 *                   - GAPM_CMP_EVT: When operation completed
 * Inputs        : - conidx        - connection index.
 *                 - peerAddr      - resolvable private random address to be
 *                                   resolved.
 *                 - irkListSize   - number of items in provided IRK list
 *                                   (ignored if irkList == NULL).
 *                 - irkList       - if irkList == NULL, irkList is loaded from
 *                                   NVR2
 *                                 - if irkList != NULL, provided irkList is
 *                                   used
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_ResolvAddrCmd(uint8_t conidx, const uint8_t *peerAddr,
                        uint8_t irkListSize, const struct gap_sec_key *irkList)
{
    struct gapm_resolv_addr_cmd *cmd;

    if (irkList == NULL)
    {
        irkListSize = BondList_Size();
    }

    if (irkListSize == 0)
    {
        return;
    }

    cmd = KE_MSG_ALLOC_DYN(GAPM_RESOLV_ADDR_CMD,
                           TASK_GAPM, KE_BUILD_ID(TASK_APP, conidx),
                           gapm_resolv_addr_cmd,
                           sizeof(struct gap_sec_key) * irkListSize);

    if (irkList)    /* User has provided list of IRKs */
    {
        memcpy(cmd->irk, irkList, GAP_KEY_LEN * irkListSize);    /* Provided list of IRKs */
    }
    else    /* Read IRKs from NVR2 flash */
    {
        BondList_GetIRKs(cmd->irk);
    }

    cmd->operation = GAPM_RESOLV_ADDR;
    cmd->nb_key = irkListSize;
    memcpy(cmd->addr.addr, peerAddr, GAP_BD_ADDR_LEN);

    PRINTF("\n\rGAPM_ResolvAddrCmd:");
    PRINTF("\n\r  cmd->addr: ");
    for (uint i = 0; i < GAP_BD_ADDR_LEN; i++) PRINTF("%d ", cmd->addr.addr[i]);

    for (uint j = 0; j < cmd->nb_key; j++)
    {
        PRINTF("\n\r  cmd->IRK[%d]: ", j);
        for (uint i = 0; i < GAP_KEY_LEN; i++) PRINTF("%d ", cmd->irk[j].key[i]);
    }

    ke_msg_send(cmd);
}

#endif    /* CFG_BOND_LIST_IN_NVR2 */

/* ----------------------------------------------------------------------------
 * Function      : uint16_t GAPM_GetProfileAddedCount(void)
 * ----------------------------------------------------------------------------
 * Description   : Return number of profile addded to the stack.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint16_t GAPM_GetProfileAddedCount(void)
{
    return gap_env.profileAddedCount;
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_GetDeviceConfig(void)
 * ----------------------------------------------------------------------------
 * Description   : Provide the current device configuration set via
 *                 GAPM_SetDevConfigCmd.
 * Inputs        : None
 * Outputs       : A pointer to const struct gapm_set_dev_config_cmd
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const struct gapm_set_dev_config_cmd * GAPM_GetDeviceConfig(void)
{
    return &gap_env.deviceConfig;
}

/* ----------------------------------------------------------------------------
 * Function      : bool GAPM_AddAdvData(enum gap_ad_type newDataFlag,
 *                           uint8_t const *newData, uint8_t newDataLen,
 *                           uint8_t *resultAdvData, uint8_t* resultAdvDataLen)
 * ----------------------------------------------------------------------------
 * Description   : Utility function to add advertising data into the result
 *                 buffer.
 *                 function
 * Inputs        : newDataFlag      - GAP Advertising flag (see
 *                                    enum gap_ad_type)
 *                 newData          - Data to be added to resultAdvData
 *                 newDataLen       - Length of the data being added (excluding
 *                                    flag)
 * Outputs       : resultAdvData    - Resulting advertising data
 *                 resultAdvDataLen - Full advertising data length
 *                                    (including flags and field length bytes)
 * Assumptions   : resultAdvDataLen must be initialized to 0 before adding data
 * ------------------------------------------------------------------------- */
bool GAPM_AddAdvData(enum gap_ad_type newDataFlag, uint8_t const *newData,
                     uint8_t newDataLen, uint8_t *resultAdvData,
                     uint8_t *resultAdvDataLen)
{
    /* If new data element cannot be added to current accumulated data */
    if ((ADV_DATA_LEN - *resultAdvDataLen) < (newDataLen + 2))
    {
        return false;
    }

    /* Else, add new data element into current accumulated data */
    resultAdvData[*resultAdvDataLen] = newDataLen + 1;
    resultAdvData[*resultAdvDataLen + 1] = newDataFlag;
    memcpy(resultAdvData + *resultAdvDataLen + 2, newData, newDataLen);

    *resultAdvDataLen += newDataLen + 2;
    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPM_MsgHandler(ke_msg_id_t const msg_id,
 *                                     void const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle any GAPM message received from kernel
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter (unused)
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPM_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                     ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (msg_id)
    {
        case GAPM_CMP_EVT:
        {
            const struct gapm_cmp_evt *p = param;

            PRINTF("\n\rGAPM_CMP_EVT: operation=0x%02x, status=0x%02x\r\n", p->operation, p->status);

            if (p->operation == GAPM_RESET)
            {
                GAP_Initialize();
                GATT_Initialize();
            }
            else if (((p->operation == GAPM_SET_DEV_CONFIG || p->operation == GAPM_CANCEL)
                      && p->status == GAP_ERR_NO_ERROR) ||
                     (p->operation >= GAPM_ADV_NON_CONN && p->operation <= GAPM_ADV_DIRECT_LDC) ||
                     (p->operation >= GAPM_CONNECTION_DIRECT && p->operation <= GAPM_CONNECTION_NAME_REQUEST) ||
                     (p->operation == GAPM_CONNECTION_GENERAL))
            {
                gap_env.gapmState = GAPM_STATE_READY;
            }
        }
        break;

        case GAPM_PROFILE_ADDED_IND:
        {
            gap_env.profileAddedCount++;
        }
        break;

#if CFG_BOND_LIST_IN_NVR2
        case GAPM_ADDR_SOLVED_IND:    /* Event triggered when private address resolution was successful */
        {
            const struct gapm_addr_solved_ind *p = param;
            const BondInfo_Type *bondInfo = BondList_FindByIRK(p->irk.key);
            uint8_t conidx = KE_IDX_GET(dest_id);
            if (bondInfo)    /* Found bond info. Copy it into environment variable */
            {
                memcpy(&gap_env.bondInfo[conidx], bondInfo, sizeof(BondInfo_Type));
            }
        }
        break;
#endif /* if CFG_BOND_LIST_IN_NVR2 */
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_ParamUpdateCfm(uint8_t conidx, bool accept,
 *                                    uint16_t ce_len_min, uint16_t ce_len_max)
 * ----------------------------------------------------------------------------
 * Description   : Send parameter update confirmation
 * Inputs        : conidx       - Connection index
 *                 accept       - Accept update request, true or false
 *                 ce_len_min   - Connection event minimum length
 *                 ce_len_max   - Connection event maximum length
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_ParamUpdateCfm(uint8_t conidx, bool accept, uint16_t ce_len_min,
                         uint16_t ce_len_max)
{
    struct gapc_param_update_cfm *cfm;

    cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM, KE_BUILD_ID(TASK_GAPC, conidx),
                       TASK_APP, gapc_param_update_cfm);

    cfm->accept = accept;
    cfm->ce_len_min = ce_len_min;
    cfm->ce_len_max = ce_len_max;

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_ConnectionCfm(uint8_t conidx,
 *                                  struct gapc_connection_cfm const *param)
 * ----------------------------------------------------------------------------
 * Description   : Send parameter update confirmation
 * Inputs        : conidx       - Connection index
 *                 param        - Connection confirmation parameters.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_ConnectionCfm(uint8_t conidx, struct gapc_connection_cfm const *param)
{
    struct gapc_connection_cfm *cfm;

    cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
                       KE_BUILD_ID(TASK_GAPC, conidx),
                       TASK_APP, gapc_connection_cfm);

    memcpy(cfm, param, sizeof(struct gapc_connection_cfm));

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_DisconnectCmd(uint8_t conidx, uint8_t reason)
 * ----------------------------------------------------------------------------
 * Description   : Disconnect a given connected peer device
 *                 Notes: followed by responses
 *                   - GAPC_DISCONNECT_IND: Event triggered when disconnection
 *                                          is finished
 *                   - GAPC_CMP_EVT:        When operation completed
 * Inputs        : conidx - connection index
 *                 reason - disconnect reason. See GAPC_DISCONNECT_CMD documentation
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
/**
 * @brief Disconnect a given connected peer device
 *
 * @param[in] conidx Connection index of the peer to be disconnected
 * @param[in] reason disconnect reason. See GAPC_DISCONNECT_CMD documentation
 */
void GAPC_DisconnectCmd(uint8_t conidx, uint8_t reason)
{
    struct gapc_disconnect_cmd *cmd;

    /* Allocate a disconnection command message */
    cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD, KE_BUILD_ID(TASK_GAPC, conidx),
                       TASK_APP, gapc_disconnect_cmd);
    cmd->operation = GAPC_DISCONNECT;
    cmd->reason = reason;

    ke_msg_send(cmd);    /* Send the message */
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_DisconnectAll(uint8_t reason)
 * ----------------------------------------------------------------------------
 * Description   : Disconnect all connected peer devices
 *                 Notes: followed by responses
 *                   - GAPC_DISCONNECT_IND: Event triggered when disconnection
 *                                          is finished
 *                   - GAPC_CMP_EVT:        When operation completed
 * Inputs        : reason - disconnect reason. See GAPC_DISCONNECT_CMD documentation
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
/**
 * @brief Disconnect all connected peer devices
 *
 * @param[in] reason disconnect reason. See GAPC_DISCONNECT_CMD documentation
 */
void GAPC_DisconnectAll(uint8_t reason)
{
    for (uint8_t i = 0; i < BLE_CONNECTION_MAX; i++)
    {
        if (GAPC_IsConnectionActive(i) != false)
        {
            GAPC_DisconnectCmd(i, reason);
        }
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetConnectionCount(void)
 * ----------------------------------------------------------------------------
 * Description   : Return the current number of established connections
 * Inputs        : None
 * Outputs       : A number [0, BLE_CONNECTION_MAX]
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint8_t GAPC_GetConnectionCount(void)
{
    return gap_env.connectionCount;
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetConnectionInfo(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Return the connection information associated with the
 *                 connection index
 * Inputs        : conidx      - Connection index
 * Outputs       : A pointer to a struct gapc_connection_req_ind
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const struct gapc_connection_req_ind * GAPC_GetConnectionInfo(uint8_t conidx)
{
    if (conidx < BLE_CONNECTION_MAX)
    {
        return &gap_env.connection[conidx];
    }
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetConnectionInfo(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Check the current state of the connection
 * Inputs        : conidx      - Connection index
 * Outputs       : true if is connected, false otherwise
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool GAPC_IsConnectionActive(uint8_t conidx)
{
    if (conidx < BLE_CONNECTION_MAX)
    {
        return (gap_env.connection[conidx].conhdl != GAP_INVALID_CONHDL);
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetDevInfoCfm_Name(uint8_t conidx, const char
 *                                              *devName)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_GET_DEV_INFO_CFM message in response to a name
 *                 request made via GetDevInfoReq.
 * Inputs        : conidx      - Connection index
 *                 devName     - Device name
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_GetDevInfoCfm_Name(uint8_t conidx, const char *devName)
{
    uint8_t len = strlen(devName);
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                         KE_BUILD_ID(TASK_GAPC, conidx),
                                                         TASK_APP, gapc_get_dev_info_cfm,
                                                         len);
    cfm->req = GAPC_DEV_NAME;
    memcpy(&cfm->info.name.value[0], devName, len);
    cfm->info.name.length = len;

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetDevInfoCfm_Appearance(uint8_t conidx,
 *                                                    uint16_t appearance)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_GET_DEV_INFO_CFM message in response to an
 *                 Appearance request made via GetDevInfoReq.
 * Inputs        : conidx      - Connection index
 *                 appearance  - Device appearance
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_GetDevInfoCfm_Appearance(uint8_t conidx, uint16_t appearance)
{
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                         KE_BUILD_ID(TASK_GAPC, conidx),
                                                         TASK_APP, gapc_get_dev_info_cfm, 0);
    cfm->req = GAPC_DEV_APPEARANCE;
    cfm->info.appearance = appearance;

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetDevInfoCfm_SlvPrefParams(
 *                               uint8_t conidx, uint16_t con_intv_min,
 *                               uint16_t con_intv_max, uint16_t slave_latency,
 *                               uint16_t conn_timeout)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_GET_DEV_INFO_CFM message in response to a
 *                 Slave Preferred Parameters request, made via GetDevInfoReq.
 * Inputs        : conidx        - Connection index
 *                 con_intv_min  - Minimum connection interval
 *                 con_intv_max  - Maximum connection interval
 *                 slave_latency - Slave latency
 *                 conn_timeout  - Connection timeout
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_GetDevInfoCfm_SlvPrefParams(uint8_t conidx, uint16_t con_intv_min,
                                      uint16_t con_intv_max, uint16_t slave_latency,
                                      uint16_t conn_timeout)
{
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                         KE_BUILD_ID(TASK_GAPC, conidx),
                                                         TASK_APP, gapc_get_dev_info_cfm, 0);
    cfm->req = GAPC_DEV_SLV_PREF_PARAMS;
    cfm->info.slv_params.con_intv_min  = con_intv_min;
    cfm->info.slv_params.con_intv_max  = con_intv_max;
    cfm->info.slv_params.slave_latency = slave_latency;
    cfm->info.slv_params.conn_timeout  = conn_timeout;

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetDevInfoCfm(uint8_t conidx, uint8_t req,
 *                                         const union gapc_dev_info_val* dat)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_GET_DEV_INFO_CFM message in response to a
 *                 GAPC_GET_DEV_INFO_REQ_IND.
 * Inputs        : conidx      - Connection index.
 *                 req         - GAPC_DEV_NAME: Device Name
 *                             - GAPC_DEV_APPEARANCE: Device Appearance Icon
 *                             - GAPC_DEV_SLV_PREF_PARAMS: Device Slave
 *                               preferred parameters.
 *                 dat        - Confirmation data. See gapc_dev_info_val.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_GetDevInfoCfm(uint8_t conidx, uint8_t req,
                        const union gapc_dev_info_val *dat)
{
    struct gapc_get_dev_info_cfm *cfm = NULL;
    size_t sz;

    /* can't use regular memcpy+sizeof because of the flexible
     * array member in gap_dev_name */
    if (req == GAPC_DEV_NAME)
    {
        sz = offsetof(struct gapc_get_dev_info_cfm, info.name.value)
             + dat->name.length;
    }
    else
    {
        sz = sizeof(struct gapc_get_dev_info_cfm);
    }

    cfm = ke_msg_alloc(
        GAPC_GET_DEV_INFO_CFM,
        KE_BUILD_ID(TASK_GAPC, conidx),
        KE_BUILD_ID(TASK_APP, conidx),
        sz);

    if (dat != NULL)
    {
        cfm->req = req;
        memcpy(&cfm->info, dat, sz - offsetof(struct gapc_get_dev_info_cfm, info));
        ke_msg_send(cfm);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_SetDevInfoCfm(uint8_t conidx, uint8_t req,
 *                                         const union gapc_dev_info_val* dat)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_SET_DEV_INFO_CFM message in response to a
 *                 GAPC_SET_DEV_INFO_REQ_IND.
 * Inputs        : conidx      - Connection index.
 *                 req         - GAPC_DEV_NAME: Device Name
 *                             - GAPC_DEV_APPEARANCE: Device Appearance Icon
 *                             - GAPC_DEV_SLV_PREF_PARAMS: Device Slave
 *                               preferred parameters.
 *                 dat        - Confirmation data. See gapc_dev_info_val.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_SetDevInfoCfm(uint8_t conidx,
                        const struct gapc_set_dev_info_req_ind *param,
                        union gapc_dev_info_val *dat,
                        size_t name_max_length)
{
    struct gapc_set_dev_info_cfm *cfm;

    /* Status code used to know if requested has been accepted or not */
    uint8_t status = GAP_ERR_REJECTED;

    if (dat != NULL)
    {
        switch (param->req)
        {
            case GAPC_DEV_NAME:
            {
                size_t sz = (param->info.name.length < name_max_length ?
                             param->info.name.length : name_max_length);
                memcpy(dat->name.value, param->info.name.value, sz);
                dat->name.length = sz;
                status = GAP_ERR_NO_ERROR;
            }
            break;

            case GAPC_DEV_APPEARANCE:
            {
                status = GAP_ERR_REJECTED;
            }
            break;
        }
    }

    cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM,
                       KE_BUILD_ID(TASK_GAPC, conidx),
                       TASK_APP,
                       gapc_set_dev_info_cfm);

    cfm->req = param->req;
    cfm->status = status;

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_BondCfm(uint8_t conidx, uint8_t request,
 *                                   uint8_t accept,
 *                                   union gapc_bond_cfm_data *data)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_BOND_CFM message in response to a
 *                 GAPC_BOND_REQ_IND.
 * Inputs        : conidx      - Connection index.
 *                 request     - See gapc_bond.
 *                 accept      - true or false to accept or reject the bond
 *                               request.
 *                 data        - Confirmation data. See gapc_bond_cfm_data.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_BondCfm(uint8_t conidx, enum gapc_bond request, bool accept,
                  const union gapc_bond_cfm_data *data)
{
    struct gapc_bond_cfm *cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                             KE_BUILD_ID(TASK_GAPC, conidx),
                                             KE_BUILD_ID(TASK_APP, conidx),
                                             gapc_bond_cfm);
    cfm->request = request;
    cfm->accept  = accept;
    memcpy(&cfm->data, data, sizeof(union gapc_bond_cfm_data));

#if CFG_BOND_LIST_IN_NVR2
    /* Slave side only: store generated LTK temporarily in gap_env.bondInfo.
     * It is written in NVR2 once GAPC_PAIRING_SUCCEED in received. */
    if (request == GAPC_LTK_EXCH)
    {
        memcpy(gap_env.bondInfo[conidx].LTK, data->ltk.ltk.key, GAP_KEY_LEN);
        memcpy(gap_env.bondInfo[conidx].RAND, data->ltk.randnb.nb, GAP_RAND_NB_LEN);
        gap_env.bondInfo[conidx].EDIV = data->ltk.ediv;
    }
#endif /* if CFG_BOND_LIST_IN_NVR2 */

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_EncryptCmd(uint8_t conidx, uint16_t ediv,
 *                                      const uint8_t* randnb,
 *                                      const uint8_t *ltk, uint8_t key_size)
 * ----------------------------------------------------------------------------
 * Description   : Operation triggered by the master in order to initiate the
 *                 encryption procedure with a connected and bonded peer.
 *                 It contains the Long Term Key that should be used during the
 *                 encryption.
 * Inputs        : conidx      - Connection index
 *                 ediv        - EDIV (Encryption Diversifier)
 *                 randnb      - Random number
 *                 ltk         - Long Term Key
 *                 key_size    - Key size (7 to 16)
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_EncryptCmd(uint8_t conidx, uint16_t ediv, const uint8_t *randnb,
                     const uint8_t *ltk, uint8_t key_size)
{
    struct gapc_encrypt_cmd *cmd = KE_MSG_ALLOC(GAPC_ENCRYPT_CMD,
                                                KE_BUILD_ID(TASK_GAPC, conidx),
                                                TASK_APP, gapc_encrypt_cmd);
    cmd->operation = GAPC_ENCRYPT;
    cmd->ltk.ediv = ediv;
    cmd->ltk.key_size = key_size;
    memcpy(cmd->ltk.ltk.key, ltk, key_size);
    memcpy(cmd->ltk.randnb.nb, randnb, GAP_RAND_NB_LEN);
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_EncryptCfm(uint8_t conidx, bool found,
 *                                      const uint8_t *ltk)
 * ----------------------------------------------------------------------------
 * Description   : Send a GAPC_ENCRYPT_CFM message in response to a
 *                 GAPC_ENCRYPT_REQ_IND.
 * Inputs        : conidx      - Connection index.
 *                 found       - Indicate if an LTK has been found for the peer
 *                               device
 *                 ltk         - Long Term Key
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_EncryptCfm(uint8_t conidx, bool found, const uint8_t *ltk)
{
    /* Prepare the GAPC_ENCRYPT_CFM message to be sent to SMPC */
    struct gapc_encrypt_cfm *cfm;
    cfm = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM, KE_BUILD_ID(TASK_GAPC, conidx),
                       KE_BUILD_ID(TASK_APP, conidx),
                       gapc_encrypt_cfm);
    cfm->found = found;

    if (found)
    {
        memcpy(cfm->ltk.key, ltk, GAP_KEY_LEN);
    }

    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_BondCmd(uint8_t conidx,
 *                                   const struct gapc_pairing* pairing)
 * ----------------------------------------------------------------------------
 * Description   : Used by a master to send a GAPC_BOND_CMD message to initiate
 *                 the pairing procedure with a connected slave peer.
 * Inputs        : conidx      - Connection index.
 *                 pairing     - Pairing features.
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_BondCmd(uint8_t conidx, const struct gapc_pairing *pairing)
{
    struct gapc_bond_cmd *cmd = KE_MSG_ALLOC(GAPC_BOND_CMD,
                                             KE_BUILD_ID(TASK_GAPC, conidx),
                                             TASK_APP, gapc_bond_cmd);
    cmd->operation = GAPC_BOND;

    memcpy(&cmd->pairing, pairing, sizeof(struct gapc_pairing));
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_MsgHandler(ke_msg_id_t const msg_id,
 *                                     void const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle any GAPC message received from kernel
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter (unused)
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GAPC_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                     ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    switch (msg_id)
    {
        case GAPC_CONNECTION_REQ_IND:
        {
            const struct gapc_connection_req_ind *p = param;
            gap_env.connectionCount++;
            memcpy(&gap_env.connection[conidx], p,
                   sizeof(struct gapc_connection_req_ind));
#if CFG_BOND_LIST_IN_NVR2
            gap_env.bondInfo[conidx].STATE = BOND_INFO_STATE_INVALID;
            if (!GAP_IsAddrPrivateResolvable(p->peer_addr.addr, p->peer_addr_type))
            {
                const BondInfo_Type *bondInfo = BondList_FindByAddr(p->peer_addr.addr, p->peer_addr_type);
                if (bondInfo)    /* Found bond info. Copy it into environment variable */
                {
                    memcpy(&gap_env.bondInfo[conidx], bondInfo, sizeof(BondInfo_Type));
                }
            }
#endif /* if CFG_BOND_LIST_IN_NVR2 */
        }
        break;

        case GAPC_DISCONNECT_IND:
        {
#if CFG_BOND_LIST_IN_NVR2
            /* If device is bonded and disconnection is due to MIC failure, it's likely that we tried
             * to encrypt with outdated bond information. So, let's remove it. */
            if (GAPC_IsBonded(conidx) &&
                (((struct gapc_disconnect_ind *)param)->reason == CO_ERROR_TERMINATED_MIC_FAILURE))
            {
                BondList_Remove(gap_env.bondInfo[conidx].STATE);
            }
#endif /* if CFG_BOND_LIST_IN_NVR2 */
            gap_env.connectionCount--;
            gap_env.connection[conidx].conhdl = GAP_INVALID_CONHDL;
        }
        break;

        case GAPC_CMP_EVT:
        {
            /* see gapc_operation for the types of operations */
            PRINTF("    \n\rGAPC_CMP_EVT: op=%d status=%d\n\r", ((struct gapc_cmp_evt *)param)->operation,
                   ((struct gapc_cmp_evt *)param)->status);
        }
        break;

        case GAPC_PARAM_UPDATED_IND:
        {
            struct gapc_param_updated_ind const *p = param;
            gap_env.connection[conidx].con_interval = p->con_interval;
            gap_env.connection[conidx].con_latency  = p->con_latency;
            gap_env.connection[conidx].sup_to       = p->sup_to;
        }
        break;

#if CFG_BOND_LIST_IN_NVR2
        case GAPC_BOND_REQ_IND:
        {
            if (((struct gapc_bond_req_ind *)param)->request == GAPC_PAIRING_REQ)
            {
                /* Remove old information from BondList (if bonded before) */
                if (GAPC_IsBonded(conidx))
                {
                    BondList_Remove(GAPC_GetBondInfo(conidx)->STATE);
                }
                gap_env.bondInfo[conidx].STATE = BOND_INFO_STATE_INVALID;
                memcpy(gap_env.bondInfo[conidx].ADDR, GAPC_GetConnectionInfo(conidx)->peer_addr.addr, GAP_BD_ADDR_LEN);
                gap_env.bondInfo[conidx].ADDR_TYPE = GAPC_GetConnectionInfo(conidx)->peer_addr_type;
            }
        }
        break;

        case GAPC_BOND_IND:
        {
            const struct gapc_bond_ind *p = param;
            switch (p->info)
            {
                case GAPC_PAIRING_SUCCEED:
                {
                    PRINTF("\n\rGAPC_BOND_IND / GAPC_PAIRING_SUCCEED");
                    BondList_Add(&gap_env.bondInfo[conidx]);
                }
                break;

                case GAPC_IRK_EXCH:
                {
                    PRINTF("\n\rGAPC_BOND_IND / GAPC_IRK_EXCH:");
                    PRINTF("\n\r  IRK: ");
                    for (uint i = 0; i < GAP_KEY_LEN; i++) PRINTF("%d ", p->data.irk.irk.key[i]);

                    PRINTF("\n\r  ADDR_TYPE: %d", p->data.irk.addr.addr_type);

                    PRINTF("\n\r  ADDR:");
                    for (uint i = 0; i < GAP_BD_ADDR_LEN; i++) PRINTF("%d ", p->data.irk.addr.addr.addr[i]);

                    memcpy(gap_env.bondInfo[conidx].IRK, p->data.irk.irk.key, GAP_KEY_LEN);
                    memcpy(gap_env.bondInfo[conidx].ADDR, p->data.irk.addr.addr.addr, BD_ADDR_LEN);
                    gap_env.bondInfo[conidx].ADDR_TYPE = p->data.irk.addr.addr_type;
                }
                break;

                case GAPC_CSRK_EXCH:
                {
                    PRINTF("\n\rGAPC_BOND_IND / GAPC_CSRK_EXCH");
                    memcpy(gap_env.bondInfo[conidx].CSRK, p->data.csrk.key, GAP_KEY_LEN);
                }
                break;

                case GAPC_LTK_EXCH:
                {
                    PRINTF("\n\rGAPC_BOND_IND / GAPC_LTK_EXCH");
                    PRINTF("\n\r  LTK: %d %d %d %d",
                           p->data.ltk.ltk.key[0], p->data.ltk.ltk.key[1],
                           p->data.ltk.ltk.key[2], p->data.ltk.ltk.key[3]);
                    PRINTF("\n\r  RAND: %d %d %d %d",
                           p->data.ltk.randnb.nb[0], p->data.ltk.randnb.nb[1],
                           p->data.ltk.randnb.nb[2], p->data.ltk.randnb.nb[3]);
                    PRINTF("\n\r  EDIV: %d", p->data.ltk.ediv);
                    memcpy(gap_env.bondInfo[conidx].LTK, p->data.ltk.ltk.key, GAP_KEY_LEN);
                    memcpy(gap_env.bondInfo[conidx].RAND, p->data.ltk.randnb.nb, GAP_RAND_NB_LEN);
                    gap_env.bondInfo[conidx].EDIV = p->data.ltk.ediv;
                }
                break;

                case GAPC_PAIRING_FAILED:
                {
                    PRINTF("\n\rGAPC_PAIRING_FAILED! Reason = %d", p->data.reason);
                }
                break;
            }
        }
        break;
#endif /* if CFG_BOND_LIST_IN_NVR2 */
    }
}

#if CFG_BOND_LIST_IN_NVR2

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_IsBonded(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Check the current state of the bonding information
 * Inputs        : conidx      - Connection index
 * Outputs       : true if is connected, false otherwise
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool GAPC_IsBonded(uint8_t conidx)
{
    return (GAPC_IsConnectionActive(conidx) &&
            VALID_BOND_INFO(gap_env.bondInfo[conidx].STATE));
}

/* ----------------------------------------------------------------------------
 * Function      : void GAPC_GetBondInfo(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Current Bond information for the specified connection.
 * Inputs        : None
 * Outputs       : A pointer to the current BondInfo. Use GAPC_IsBonded(conidx)
 *                 to check if it's valid.
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const BondInfo_Type * GAPC_GetBondInfo(uint8_t conidx)
{
    return &gap_env.bondInfo[conidx];
}

/* ----------------------------------------------------------------------------
 * Function      : bool BondList_Add(BondInfo_Type* bond_info)
 * ----------------------------------------------------------------------------
 * Description   : Add a new entry into the BondList. If NVR2 is full and there
 *                  are invalid entries in the list, the NVR2 is refreshed.
 * Inputs        : - bond_info      - pointer to a BondInfo_Type variable
 * Outputs       : - true if it was able to write the new entry into flash.
 *                 - false if list is full or if an error occurred while
 *                   writing to flash memory.
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool BondList_Add(BondInfo_Type *bond_info)
{
    /* If there is space in the BondList */
    if (BondList_Size() < CFG_BONDLIST_SIZE)
    {
        uint8_t *state;
        unsigned int data_size;
        unsigned int *data_to_write;
        uint32_t *start_addr = (uint32_t *)FLASH_NVR2_BASE;

        /* Check if NVR2 is full due to invalid entries. To do so, we only need
         * to check the state of last possible entry (SIZEOF_BONDLIST-1) */
        bool NVR2_full =
            *((uint8_t *)(FLASH_NVR2_BASE + (SIZEOF_BONDLIST - 1) *
                          sizeof(BondInfo_Type))) != BOND_INFO_STATE_EMPTY;

        /* If NVR2 is full, we will need to erase the whole sector and squeeze
         * the valid entries side-by-side. This will create space for the new
         * entry. */
        if (NVR2_full)
        {
            BondInfo_Type list[SIZEOF_BONDLIST];

            /* Copy valid entries to RAM */

            uint8_t copied = 0;

            for (uint8_t i = 0; i < SIZEOF_BONDLIST; i++)
            {
                state = (uint8_t *)(FLASH_NVR2_BASE + i *
                                    sizeof(BondInfo_Type));
                if (VALID_BOND_INFO(*state))
                {
                    /* Read bond_info entry from NVR2 */
                    memcpy(&list[copied],
                           (uint8_t *)(FLASH_NVR2_BASE +
                                       i * sizeof(BondInfo_Type)),
                           sizeof(BondInfo_Type));

                    /* Update STATE field with new index */
                    list[copied].STATE = copied + 1;
                    copied++;
                }
            }

            /* Add new bond_info to the end of the array */
            memcpy(&list[copied], bond_info, sizeof(BondInfo_Type));

            /* Update STATE field with new index */
            list[copied].STATE = copied + 1;
            bond_info->STATE = list[copied].STATE;
            copied++;

            /* Erase NVR2 */
            if (!BondList_RemoveAll())
            {
                return (false);
            }

            /* NVR2 sector was erased, will write everything to the base
             * address */
            start_addr = (uint32_t *)FLASH_NVR2_BASE;

            /* data_size is the current size + 1 (the new entry) */
            data_size = copied * sizeof(BondInfo_Type);

            /* The data_to_write is the list */
            data_to_write = (unsigned int *)list;
        }

        /* NVR2 has space available, we can write the new entry directly. */
        else
        {
            /* Search for the first available position in NVR2 */
            for (uint8_t i = 0; i < SIZEOF_BONDLIST; i++)
            {
                /* Read state of each bond_info entry in NVR2 */
                state = (uint8_t *)(FLASH_NVR2_BASE + i *
                                    sizeof(BondInfo_Type));
                if (*state == BOND_INFO_STATE_EMPTY)
                {
                    /* start_addr will be the first available index "i" */
                    start_addr = (uint32_t *)(FLASH_NVR2_BASE + i *
                                              sizeof(BondInfo_Type));

                    /* Update STATE field with new index */
                    bond_info->STATE = i + 1;
                    break;
                }
            }

            /* The data_to_write is directly the bond_info passed as argument */
            data_to_write = (unsigned int *)bond_info;

            /* We are going to write just 1 entry */
            data_size = sizeof(BondInfo_Type);
        }

        /* Write new data to flash */
        NVR2_WriteEnable(true);
        unsigned int result = Flash_WriteBuffer((unsigned int)start_addr,
                                                data_size / 4, data_to_write);
        NVR2_WriteEnable(false);

        return (result == FLASH_ERR_NONE);
    }
    return (false);
}

/* ----------------------------------------------------------------------------
 * Function      : const BondInfo_Type* BondList_FindByAddr(
 *                                               const uint8_t *addr,
 *                                               uint8_t addrType)
 * ----------------------------------------------------------------------------
 * Description   : Search for the bonding information for a peer device in
 *                 NVR2 with matching address and address type
 * Inputs        : - addr      - address of the peer device to search
 *                 - addrType  - address type of the peer device to search
 * Outputs       : - If found an entry in NVR2, return its address as a pointer
 *                   to a const BondInfo_Type* element
 *                 - If not found, return NULL
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const BondInfo_Type * BondList_FindByAddr(const uint8_t *addr, uint8_t addrType)
{
    const BondInfo_Type *tmp;
    for (uint8_t i = 0; i < SIZEOF_BONDLIST; i++)
    {
        /* Read bond_info entry from NVR2 */
        tmp = (const BondInfo_Type *)(FLASH_NVR2_BASE + i *
                                      sizeof(BondInfo_Type));

        /* If bond info is valid and address match */
        if ((VALID_BOND_INFO(tmp->STATE)) &&
            (tmp->ADDR_TYPE == addrType) &&
            (memcmp(tmp->ADDR, addr, BD_ADDR_LEN) == 0))
        {
            return tmp;    /* return const reference to BondInfo in NVR2 */
        }
    }
    return NULL;
}

const BondInfo_Type * BondList_FindByIRK(const uint8_t *irk)
{
    const BondInfo_Type *tmp;
    for (uint8_t i = 0; i < SIZEOF_BONDLIST; i++)
    {
        /* Read bond_info entry from NVR2 */
        tmp = (const BondInfo_Type *)(FLASH_NVR2_BASE + i *
                                      sizeof(BondInfo_Type));

        /* If bond info is valid and address match */
        if ((VALID_BOND_INFO(tmp->STATE)) &&
            (memcmp(tmp->IRK, irk, GAP_KEY_LEN) == 0))
        {
            return tmp;    /* return const reference to BondInfo in NVR2 */
        }
    }
    return NULL;
}

/* ----------------------------------------------------------------------------
 * Function      : bool BondList_Remove(uint8_t bondStateIndex)
 * ----------------------------------------------------------------------------
 * Description   : Invalidates the bonding information associated to a peer
 *                 device located at FLASH_NVR2_BASE + (bondStateIndex - 1) *
 *                 sizeof(BondInfo_Type).
 * Inputs        : - bondStateIndex - bond information index. This index is
 *                   stored in the STATE field of a BondInfo_Type entry.
 * Outputs       : - Return true if removed successfully or false otherwise.
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool BondList_Remove(uint8_t bondStateIndex)
{
    bool result = false;
    if (bondStateIndex <= SIZEOF_BONDLIST)
    {
        unsigned int data = BOND_INFO_STATE_INVALID;

        /* NVR2 address where the bonding information will be invalidated */
        uint32_t *addr = (uint32_t *)(FLASH_NVR2_BASE +
                                      (bondStateIndex - 1) * sizeof(BondInfo_Type));

        /* Invalidate the state for the old bonding information stored in
         * NVR2 by clearing the first two words. */
        NVR2_WriteEnable(true);
        result = (Flash_WriteWordPair((unsigned int)addr, data, data) ==
                  FLASH_ERR_NONE);
        NVR2_WriteEnable(false);
    }
    return result;
}

/* ----------------------------------------------------------------------------
 * Function      : void BondList_RemoveAll(void)
 * ----------------------------------------------------------------------------
 * Description   : Clear BondList in flash by erasing NVR2 sector.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : Reset the board in case it cannot enable write on NVR2
 * ------------------------------------------------------------------------- */
bool BondList_RemoveAll(void)
{
    /* Erase NVR2 sector */
    NVR2_WriteEnable(true);
    unsigned int result = Flash_EraseSector(FLASH_NVR2_BASE);
    NVR2_WriteEnable(false);

    return (result == FLASH_ERR_NONE);
}

/* ----------------------------------------------------------------------------
 * Function      : uint8_t BondList_Size(void)
 * ----------------------------------------------------------------------------
 * Description   : Calculates the number of entries in the BondList
 *                 information
 * Inputs        : None
 * Outputs       : Number of entries in the BondList
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint8_t BondList_Size(void)
{
    uint8_t size = 0;
    uint8_t *state;

    for (unsigned int i = 0; i < SIZEOF_BONDLIST; i++)
    {
        /* Read state of each bond_info entry from NVR2 */
        state = (uint8_t *)(FLASH_NVR2_BASE + i * sizeof(BondInfo_Type));
        if (VALID_BOND_INFO(*state))
        {
            size++;
        }
    }

    return (size);
}

/* ----------------------------------------------------------------------------
 * Function      : uint8_t BondList_GetIRKs(const struct gap_sec_key *irkList)
 * ----------------------------------------------------------------------------
 * Description   : Returns the number of IRK keys along with the keys
 * Inputs        : irkList      - Pointer to an empty array that will hold the
 *                                result
 * Outputs       : - The number of IRK keys present in the BondList
 *                 - irkList array filled with all IRK keys in the BondList
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint8_t BondList_GetIRKs(struct gap_sec_key *irkList)
{
    uint8_t numKeys = 0;
    BondInfo_Type tmp;

    for (unsigned int i = 0; i < SIZEOF_BONDLIST; i++)
    {
        /* Read bond_info entry from NVR2 */
        memcpy(&tmp, (uint8_t *)(FLASH_NVR2_BASE + i * sizeof(BondInfo_Type)),
               sizeof(BondInfo_Type));

        /* If this bond_info entry is valid and the addresses match */
        if (VALID_BOND_INFO(tmp.STATE))
        {
            memcpy(&irkList[numKeys], tmp.IRK, sizeof(tmp.IRK));
            numKeys++;
        }
    }

    return (numKeys);
}

/* ----------------------------------------------------------------------------
 * Function      : unsigned NVR2_WriteEnable(bool enable)
 * ----------------------------------------------------------------------------
 * Description   : Enable/Disable writing to NVR2
 * Inputs        : - enable    - Enable writing when true, disable otherwise
 * Outputs       : None
 * Assumptions   : Function should cause a watchdog reset in case of error
 * ------------------------------------------------------------------------- */
void NVR2_WriteEnable(bool enable)
{
    /* Lock or unlock NVR2 region */
    FLASH->NVR_CTRL = NVR2_WRITE_ENABLE;
    FLASH->NVR_WRITE_UNLOCK = enable ? FLASH_NVR_KEY : 0;

    /* Check the lock/unlock operation result */
    bool NVR2_unlocked = (FLASH->IF_STATUS & 0x3FF) == 0x20;

    /* Error checking: this application needs to write in flash. If an error
     * occurred while locking/unlocking NVR2, wait for a watchdog reset */
    if (NVR2_unlocked != enable)
    {
        /* Disable all interrupts and clear any pending interrupts */
        Sys_NVIC_DisableAllInt();
        Sys_NVIC_ClearAllPendingInt();
        while (1)
        {
            /* Wait for Watchdog Reset to Occur */
            asm ("nop");
        }
    }
}

#endif    /* CFG_BOND_LIST_IN_NVR2 */
