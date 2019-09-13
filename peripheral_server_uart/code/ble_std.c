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
 * ble_std.c
 * - Bluetooth standard functions and message handlers
 * ----------------------------------------------------------------------------
 * $Revision: 1.22 $
 * $Date: 2018/11/20 20:25:54 $
 * ------------------------------------------------------------------------- */

#include "app.h"

/* Bluetooth Environment Structure */
struct ble_env_tag ble_env;

/* List of functions used to create the database */
const appm_add_svc_func_t appm_add_svc_func_list[] = {
    SERVICE_ADD_FUNCTION_LIST, NULL
};

/* List of functions used to enable client services */
const appm_enable_svc_func_t appm_enable_svc_func_list[] = {
    SERVICE_ENABLE_FUNCTION_LIST, NULL
};

/* Bluetooth Device Address */
uint8_t bdaddr[BDADDR_LENGTH];
uint8_t bdaddr_type;

static struct gapm_set_dev_config_cmd *gapmConfigCmd;

/* ----------------------------------------------------------------------------
 * Function      : Device_Param_Prepare(struct app_device_param *param)
 * ----------------------------------------------------------------------------
 * Description   : Configure the device related stack parameters
 * Inputs        : - param    - Pointer to the parameters to be configured
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Device_Param_Prepare(app_device_param_t *param)
{
    param->device_param_src_type = FLASH_PROVIDED_or_DFLT;

    if (param->device_param_src_type == APP_PROVIDED)
    {
        uint8_t temp_bleAddress[6] = APP_PUBLIC_BDADDR;

        memcpy(param->bleAddress, temp_bleAddress, 6);
    }
}

/* ----------------------------------------------------------------------------
 * Standard Functions
 * ------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Function      : void BLE_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize the BLE baseband and application manager
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_Initialize(void)
{
    struct gapm_reset_cmd *cmd;
    uint8_t default_addr[BDADDR_LENGTH] = PRIVATE_BDADDR;

    /* Seed the random number generator */
    srand(1);

    /* Set radio clock accuracy in ppm */
    BLE_DeviceParam_Set_ClockAccuracy(RADIO_CLOCK_ACCURACY);

    /* Initialize the kernel and Bluetooth stack */
    Kernel_Init(0);
    BLE_InitNoTL(0);

    /* Enable the Bluetooth related interrupts needed */
    NVIC_EnableIRQ(BLE_EVENT_IRQn);
    NVIC_EnableIRQ(BLE_RX_IRQn);
    NVIC_EnableIRQ(BLE_CRYPT_IRQn);
    NVIC_EnableIRQ(BLE_ERROR_IRQn);
    NVIC_EnableIRQ(BLE_SW_IRQn);
    NVIC_EnableIRQ(BLE_GROSSTGTIM_IRQn);
    NVIC_EnableIRQ(BLE_FINETGTIM_IRQn);
    NVIC_EnableIRQ(BLE_CSCNT_IRQn);
    NVIC_EnableIRQ(BLE_SLP_IRQn);

    /* Reset the Bluetooth environment */
    memset(&ble_env, 0, sizeof(ble_env));

    /* Initialize task state */
    ble_env.state = APPM_INIT;

    /* Set Bluetooth device type and address: depending on the device address
     * type selected by the application, either a public or private address is
     * used:
     *     - In case public address type is selected: (i) If a public address is
     *       provided by the application, it is used; (ii) Otherwise the device
     *       public address available in DEVICE_INFO_BLUETOOTH_ADDR (located in
     *       NVR3) is used. If no public address is available in NVR3, a default
     *       public address pre-defined in the stack is used instead.
     *     - In case private address type is selected: the private address
     *       provided by the application is used. */
#if (BD_ADDRESS_TYPE == BD_TYPE_PUBLIC)
    bdaddr_type = GAPM_CFG_ADDR_PUBLIC;
    if (Device_Param_Read(PARAM_ID_PUBLIC_BLE_ADDRESS, (uint8_t *)&bdaddr))
    {
        memcpy(bdaddr, default_addr, sizeof(uint8_t) * BDADDR_LENGTH);
    }
    else
    {
        memcpy(bdaddr, &co_default_bdaddr, sizeof(uint8_t) * BDADDR_LENGTH);
    }
#else    /* if (BD_ADDRESS_TYPE == BD_TYPE_PUBLIC) */
    memcpy(bdaddr, default_addr, sizeof(uint8_t) * BDADDR_LENGTH);
    bdaddr_type = GAPM_CFG_ADDR_PRIVATE;
#endif    /* if (BD_ADDRESS_TYPE == BD_TYPE_PUBLIC) */

    /* Initialize GAPM configuration command to initialize the stack */
    gapmConfigCmd =
        malloc(sizeof(struct gapm_set_dev_config_cmd));
    gapmConfigCmd->operation      = GAPM_SET_DEV_CONFIG;
    gapmConfigCmd->role = GAP_ROLE_PERIPHERAL;
    memcpy(gapmConfigCmd->addr.addr, bdaddr, BDADDR_LENGTH);
    gapmConfigCmd->addr_type = bdaddr_type;
    gapmConfigCmd->renew_dur = RENEW_DUR;
    memset(&gapmConfigCmd->irk.key[0], 0, KEY_LEN);
    gapmConfigCmd->pairing_mode = GAPM_PAIRING_DISABLE;
    gapmConfigCmd->gap_start_hdl = 0;
    gapmConfigCmd->gatt_start_hdl = 0;
    gapmConfigCmd->max_mtu = MTU_MAX;
    gapmConfigCmd->max_mps = MPS_MAX;
    gapmConfigCmd->att_and_ext_cfg = ATT_CFG;
    gapmConfigCmd->sugg_max_tx_octets = TX_OCT_MAX;
    gapmConfigCmd->sugg_max_tx_time = TX_TIME_MAX;
    gapmConfigCmd->tx_pref_rates = GAP_RATE_ANY;
    gapmConfigCmd->rx_pref_rates = GAP_RATE_ANY;
    gapmConfigCmd->max_nb_lecb = 0x0;
    gapmConfigCmd->audio_cfg = 0;

    /* Reset the stack */
    cmd = KE_MSG_ALLOC(GAPM_RESET_CMD, TASK_GAPM, TASK_APP, gapm_reset_cmd);
    cmd->operation = GAPM_RESET;

    /* Send the message */
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : bool Service_Add(void)
 * ----------------------------------------------------------------------------
 * Description   : Add the next service in the service list,
 *                 calling the appropriate add service function
 * Inputs        : None
 * Outputs       : return value - Indicates if any service has not yet been
 *                                added
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool Service_Add(void)
{
    /* Check if another should be added in the database */
    if (appm_add_svc_func_list[ble_env.next_svc] != NULL)
    {
        /* Call the function used to add the required service */
        appm_add_svc_func_list[ble_env.next_svc] ();

        /* Select the next service to add */
        ble_env.next_svc++;
        return (true);
    }

    return (false);
}

/* ----------------------------------------------------------------------------
 * Function      : void Advertising_Start(void)
 * ----------------------------------------------------------------------------
 * Description   : Send a start advertising
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Advertising_Start(void)
{
    uint8_t device_name_length;
    uint8_t device_name_avail_space;
    uint8_t scan_rsp[SCAN_RSP_DATA_LEN] = APP_SCNRSP_DATA;
    uint8_t company_id[APP_COMPANY_ID_DATA_LEN] = APP_COMPANY_ID_DATA;

    /* Prepare the GAPM_START_ADVERTISE_CMD message */
    struct gapm_start_advertise_cmd *cmd;

    /* If the application is ready, start advertising */
    if (ble_env.state == APPM_READY)
    {
        /* Prepare the start advertisment command message */
        cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD, TASK_GAPM, TASK_APP,
                           gapm_start_advertise_cmd);
        cmd->op.addr_src = GAPM_STATIC_ADDR;
        cmd->channel_map = APP_ADV_CHMAP;

        cmd->intv_min = APP_ADV_INT_MIN;
        cmd->intv_max = APP_ADV_INT_MAX;

        cmd->op.code = GAPM_ADV_UNDIRECT;
        cmd->op.state = 0;
        cmd->info.host.mode = GAP_GEN_DISCOVERABLE;
        cmd->info.host.adv_filt_policy = 0;

        /* Set the scan response data */
        cmd->info.host.scan_rsp_data_len = APP_SCNRSP_DATA_LEN;
        memcpy(&cmd->info.host.scan_rsp_data[0],
               scan_rsp, cmd->info.host.scan_rsp_data_len);

        /* Get remaining space in the advertising data -
         * 2 bytes are used for name length/flag */
        cmd->info.host.adv_data_len = 0;
        device_name_avail_space = (ADV_DATA_LEN - 3) - 2;

        /* Check if data can be added to the advertising data */
        if (device_name_avail_space > 0)
        {
            /* Add as much of the device name as possible */
            device_name_length = strlen(APP_DFLT_DEVICE_NAME);
            if (device_name_length > 0)
            {
                /* Check available space */
                device_name_length = co_min(device_name_length,
                                            device_name_avail_space);
                cmd->info.host.adv_data[cmd->info.host.adv_data_len] =
                    device_name_length + 1;

                /* Fill device name flag */
                cmd->info.host.adv_data[cmd->info.host.adv_data_len + 1] =
                    APP_DEVICE_NAME_FLAG;

                /* Copy device name */
                memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len +
                                                2],
                       APP_DFLT_DEVICE_NAME, device_name_length);

                /* Update advertising data length */
                cmd->info.host.adv_data_len += (device_name_length + 2);
            }

            /* If there is still space, add the company ID */
            if (((ADV_DATA_LEN - 3) - cmd->info.host.adv_data_len - 2) >=
                APP_COMPANY_ID_DATA_LEN)
            {
                memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len],
                       company_id, APP_COMPANY_ID_DATA_LEN);
                cmd->info.host.adv_data_len += APP_COMPANY_ID_DATA_LEN;
            }
        }

        /* Send the message */
        ke_msg_send(cmd);

        /* Set the state of the task to APPM_ADVERTISING  */
        ble_env.state = APPM_ADVERTISING;
    }
}

/* ----------------------------------------------------------------------------
 * Standard Message Handlers
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : int GAPM_ProfileAddedInd(ke_msg_id_t const msg_id,
 *                                          struct gapm_profile_added_ind
 *                                          const *param,
 *                                          ke_task_id_t const dest_id,
 *                                          ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle the received result of adding a profile to the
 *                 attribute database
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapm_profile_added_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPM_ProfileAddedInd(ke_msg_id_t const msg_id,
                         struct gapm_profile_added_ind const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id)
{
    /* If the application is creating its attribute database, continue to add
     * services; otherwise do nothing. */
    if (ble_env.state == APPM_CREATE_DB)
    {
        /* Add the next requested service */
        if (!Service_Add())
        {
            /* If there are no more services to add, go to the ready state */
            ble_env.state = APPM_READY;

            /* No more services to add, start advertising */
            Advertising_Start();
        }
    }

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int GAPM_CmpEvt(ke_msg_id_t const msg_id,
 *                                 struct gapm_cmp_evt
 *                                 const *param,
 *                                 ke_task_id_t const dest_id,
 *                                 ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle the reception of a GAPM complete event
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapm_cmp_evt
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPM_CmpEvt(ke_msg_id_t const msg_id,
                struct gapm_cmp_evt const *param,
                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    struct gapm_set_dev_config_cmd *cmd;

    switch (param->operation)
    {
        /* A reset has occurred, configure the device and
         * start a kernel timer for the application */
        case (GAPM_RESET):
        {
            if (param->status == GAP_ERR_NO_ERROR)
            {
                /* Set the device configuration */
                cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD, TASK_GAPM, TASK_APP,
                                   gapm_set_dev_config_cmd);
                memcpy(cmd, gapmConfigCmd, sizeof(struct
                                                  gapm_set_dev_config_cmd));
                free(gapmConfigCmd);

                /* Send message */
                ke_msg_send(cmd);

                /* Start a timer to be used as a periodic tick timer for
                 * application */
                ke_timer_set(APP_TEST_TIMER, TASK_APP, TIMER_200MS_SETTING);
                ke_timer_set(UART_TEST_TIMER, TASK_APP, TIMER_20MS_SETTING);

                /* Start LED timer */
                ke_timer_set(LED_TIMER, TASK_APP, TIMER_200MS_SETTING);
            }
        }
        break;

        /* Device configuration updated */
        case (GAPM_SET_DEV_CONFIG):
        {
            /* Start creating the GATT database */
            ble_env.state = APPM_CREATE_DB;

            /* Add the first required service in the database */
            if (!Service_Add())
            {
                /* Go to the ready state */
                ble_env.state = APPM_READY;

                /* Start advertising since there are no services to add
                 * to the attribute database */
                Advertising_Start();
            }
        }
        break;

        default:
        {
            /* No action required for other operations */
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int GAPC_GetDevInfoReqInd(ke_msg_id_t const msg_id,
 *                                           struct gapc_get_dev_info_req_ind
 *                                           const *param,
 *                                           ke_task_id_t const dest_id,
 *                                           ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle message device info request received from GAP
 * controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapc_get_dev_info_req_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPC_GetDevInfoReqInd(ke_msg_id_t const msg_id,
                          struct gapc_get_dev_info_req_ind const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    uint8_t len = strlen(APP_DFLT_DEVICE_NAME);

    /* Allocate message */
    struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                         src_id, dest_id,
                                                         gapc_get_dev_info_cfm,
                                                         len);

    switch (param->req)
    {
        case GAPC_DEV_NAME:
        {
            cfm->req = GAPC_DEV_NAME;
            memcpy(&cfm->info.name.value[0], APP_DFLT_DEVICE_NAME, len);
            cfm->info.name.length = len;
        }
        break;

        case GAPC_DEV_APPEARANCE:
        {
            /* Set the device appearance (no appearance) */
            cfm->info.appearance = 0;
            cfm->req = GAPC_DEV_APPEARANCE;
            cfm->info.appearance = GAPM_WRITE_DISABLE;
        }
        break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            /* Slave preferred connection interval (minimum) */
            cfm->info.slv_params.con_intv_min = PREF_SLV_MIN_CON_INTERVAL;

            /* Slave preferred connection interval (maximum) */
            cfm->info.slv_params.con_intv_max = PREF_SLV_MAX_CON_INTERVAL;

            /* Slave preferred connection latency */
            cfm->info.slv_params.slave_latency = PREF_SLV_LATENCY;

            /* Slave preferred link supervision timeout */
            cfm->info.slv_params.conn_timeout = PREF_SLV_SUP_TIMEOUT;

            cfm->req = GAPC_DEV_SLV_PREF_PARAMS;
        }
        break;

        default:
        {
            /* No action required for other requests */
        }
        break;
    }

    /* Send message */
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int GAPC_ConnectionReqInd(ke_msg_idd_t const msg_id,
 *                                           struct gapc_connection_req_ind
 *                                           const *param,
 *                                           ke_task_id_t const dest_id,
 *                                           ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle connection indication message received from GAP
 * controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapc_connection_req_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPC_ConnectionReqInd(ke_msg_id_t const msg_id,
                          struct gapc_connection_req_ind const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    struct gapc_connection_cfm *cfm;

    ble_env.conidx = KE_IDX_GET(src_id);

    /* Check if the received connection handle was valid */
    if (ble_env.conidx != GAP_INVALID_CONIDX)
    {
        ble_env.state = APPM_CONNECTED;

        /* Retrieve the connection info from the parameters */
        ble_env.conidx = param->conhdl;

        /* Save the connection parameters */
        ble_env.con_interval = param->con_interval;
        ble_env.con_latency = param->con_latency;
        ble_env.time_out = param->sup_to;

        /* Send connection confirmation */
        cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
                           KE_BUILD_ID(TASK_GAPC, ble_env.conidx), TASK_APP,
                           gapc_connection_cfm);

        cfm->auth = GAP_AUTH_REQ_NO_MITM_NO_BOND;

        cfm->svc_changed_ind_enable = 0;

        /* Send the message */
        ke_msg_send(cfm);

        /* Increasing MTU Capability and throughput rate */
        Connection_SetPktLength(ble_env.conidx, MTU_SIZE);
        Connection_ExchangeMTUCmd(ble_env.conidx);
        Connection_SetPHY(GAP_RATE_LE_2MBPS, GAP_RATE_LE_2MBPS);

        BLE_SetServiceState(true, ble_env.conidx);
    }
    else
    {
        Advertising_Start();
    }

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : unsigned int GAPC_CmpEvt(ke_msg_id_t const msg_id,
 *                                 struct gapc_cmp_evt
 *                                 const *param,
 *                                 ke_task_id_t const dest_id,
 *                                 ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle received GAPC complete event
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapc_cmp_evt
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPC_CmpEvt(ke_msg_id_t const msg_id,
                struct gapc_cmp_evt const *param,
                ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    /* No operations in this application use this event */
    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int GAPC_DisconnectInd(ke_msg_id_t const msg_id,
 *                                        struct gapc_disconnect_ind
 *                                        const *param,
 *                                        ke_task_id_t const dest_id,
 *                                        ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle disconnect indication message from GAP controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapc_disconnect_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPC_DisconnectInd(ke_msg_id_t const msg_id,
                       struct gapc_disconnect_ind const *param,
                       ke_task_id_t const dest_id,
                       ke_task_id_t const src_id)
{
    /* Go to the ready state */
    ble_env.state = APPM_READY;

    BLE_SetServiceState(false, ble_env.conidx);

    Advertising_Start();

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int GAPC_ParamUpdatedInd(ke_msg_id_t const msg_id,
 *                                          struct gapc_param_updated_ind
 *                                          const *param,
 *                                          ke_task_id_t const dest_id,
 *                                          ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle message parameter updated indication received from GAP
 *                 controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gapc_param_updated_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int GAPC_ParamUpdatedInd(ke_msg_id_t const msg_id,
                         struct gapc_param_updated_ind const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id)
{
    ble_env.updated_con_interval = param->con_interval;
    ble_env.updated_latency = param->con_latency;
    ble_env.updated_suo_to = param->sup_to;

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : unsigned int GAPC_ParamUpdateReqInd(ke_msg_id_t const msg_id,
 *                         struct gapc_param_update_req_ind const *param,
 *                         ke_task_id_t const dest_id,
 *                         ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   :
 * ------------------------------------------------------------------------- */
int GAPC_ParamUpdateReqInd(ke_msg_id_t const msg_id,
                           struct gapc_param_update_req_ind const *param,
                           ke_task_id_t const dest_id,
                           ke_task_id_t const src_id)
{
    struct gapc_param_update_cfm *cfm;

    cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM,
                       KE_BUILD_ID(TASK_GAPC, ble_env.conidx),
                       TASK_APP,
                       gapc_param_update_cfm);
    cfm->accept = 1;
    cfm->ce_len_max = 0xffff;
    cfm->ce_len_min = 0xffff;

    /* Send message */
    ke_msg_send(cfm);
    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : void BLE_SetServiceState(bool enable, uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Set Bluetooth application environment state to enabled
 * Inputs        : - enable    - Indicates that enable request should be sent
 *                               for all services/profiles or their status
 *                               should be set to disabled
 *                               enabled or disabled
 *                 - conidx    - Connection index
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_SetServiceState(bool enable, uint8_t conidx)
{
    /* All standard services should be send enable request to the stack,
     * for custom services, application should decide if it would want
     * to do a specific action
     * all services should be disabled once the link is lost
     */
    if (enable == true)
    {
        /* To enable standard Bluetooth services, an enable request should be
         * sent to the stack (for related profile) and at receiving the
         * response the enable flag can be set. For custom service it is
         * application implementation dependent. Here, it starts service
         * discovery, and if the service UUID and characteristics UUID are
         * discovered, then it goes to an state that is equivalent to the
         * enable flag of standard profiles
         */
        ble_env.next_svc_enable = 0;
        Service_Enable(conidx);
    }
    else
    {
        bass_support_env.enable = false;
        cs_env.state = CS_INIT;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : bool Service_Enable(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Enable the next service in the service list,
 *                 calling the appropriate enable service function
 * Inputs        : - conidx     - Connection index
 * Outputs       : return value - Indicates if any service has not yet been
 *                                added
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool Service_Enable(uint8_t conidx)
{
    /* Check if another should be added in the database */
    if (appm_enable_svc_func_list[ble_env.next_svc_enable] != NULL)
    {
        /* Call the function used to enable the required service */
        appm_enable_svc_func_list[ble_env.next_svc_enable] (conidx);

        /* Select the next service to enable */
        ble_env.next_svc_enable++;
        return (true);
    }

    return (false);
}

/* ----------------------------------------------------------------------------
 * Function      : void Connection_SetPktLength(uint8_t conidx, uint16_t length)
 * ----------------------------------------------------------------------------
 * Description   : Defining the connection packet length
 * Inputs        : - conidx       - connection index
 *                 - length       - length of value
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Connection_SetPktLength(uint8_t conidx, uint16_t length)
{
    struct gapc_set_le_pkt_size_cmd *cmd = KE_MSG_ALLOC(
        GAPC_SET_LE_PKT_SIZE_CMD,
        KE_BUILD_ID(TASK_GAPC,
                    conidx), TASK_APP, gapc_set_le_pkt_size_cmd);

    cmd->operation = GAPC_SET_LE_PKT_SIZE;
    cmd->tx_octets = length;
    cmd->tx_time = ((length * 8) + 112);

    /* Send the message */
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void Connection_ExchangeMTUCmd(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Configuring maximum MTU value due to MTU negotiation over ATT
 * Inputs        : conidx       - connection index
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Connection_ExchangeMTUCmd(uint8_t conidx)
{
    struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
                                                 KE_BUILD_ID(TASK_GATTC,
                                                             conidx), TASK_APP,
                                                 gattc_exc_mtu_cmd);
    cmd->operation = GATTC_MTU_EXCH;
    cmd->seq_num = 0;

    /* Send the message */
    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void Connection_SETPhy(uint8_t rx_rate, uint8_t tx_rate)
 * ----------------------------------------------------------------------------
 * Description   : Configuring RX and TX rate
 * Inputs        : -rx_rate       - RX rate
 *                 -tx_rate       - TX rate
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Connection_SetPHY(uint8_t rx_rate, uint8_t tx_rate)
{
    struct gapc_set_phy_cmd *cmd = KE_MSG_ALLOC(GAPC_SET_PHY_CMD,
                                                KE_BUILD_ID(TASK_GAPC,
                                                            ble_env.conidx),
                                                TASK_APP,
                                                gapc_set_phy_cmd);
    cmd->operation = GAPC_SET_PHY;
    cmd->rx_rates = rx_rate;
    cmd->tx_rates = tx_rate;
    ke_msg_send(cmd);
}
