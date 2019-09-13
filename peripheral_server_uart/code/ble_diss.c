/* ----------------------------------------------------------------------------
 * Copyright (c) 2017 Semiconductor Components Industries, LLC (d/b/a
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
 *  This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * ble_diss.c
 * - Device information service server functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.8 $
 * $Date: 2017/12/05 15:41:23 $
 * ------------------------------------------------------------------------- */

#include "app.h"

/* Global variable definition */
struct diss_support_env_tag diss_support_env;

/* ----------------------------------------------------------------------------
 * Function      : void Diss_Env_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize device info service server service environment
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Diss_Env_Initialize(void)
{
    diss_support_env.enable = 0;
}

/* ----------------------------------------------------------------------------
 * Function      : int DevinceInfo_ValueReqInd(ke_msg_id_t const msg_id,
 *                                     struct diss_value_req_ind
 *                                     const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle the indication when peer device request a value
 *                 initialized by application
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gattm_add_svc_rsp
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int DeviceInfo_ValueReqInd(ke_msg_id_t const msgid,
                           struct diss_value_req_ind const *param,
                           ke_task_id_t const dest_id,
                           ke_task_id_t const src_id)
{
    /* Initialize length */
    uint8_t len = 0;

    /* Pointer to the data */
    uint8_t *data = NULL;

    /* Check requested value */
    switch (param->value)
    {
        case DIS_MANUFACTURER_NAME_CHAR:
        {
            /* Set information */
            len = APP_DIS_MANUFACTURER_NAME_LEN;
            data = (uint8_t *)APP_DIS_MANUFACTURER_NAME;
        } break;

        case DIS_MODEL_NB_STR_CHAR:
        {
            /* Set information */
            len = APP_DIS_MODEL_NB_STR_LEN;
            data = (uint8_t *)APP_DIS_MODEL_NB_STR;
        } break;

        case DIS_SYSTEM_ID_CHAR:
        {
            /* Set information */
            len = APP_DIS_SYSTEM_ID_LEN;
            data = (uint8_t *)APP_DIS_SYSTEM_ID;
        } break;

        case DIS_PNP_ID_CHAR:
        {
            /* Set information */
            len = APP_DIS_PNP_ID_LEN;
            data = (uint8_t *)APP_DIS_PNP_ID;
        } break;

        case DIS_SERIAL_NB_STR_CHAR:
        {
            /* Set information */
            len = APP_DIS_SERIAL_NB_STR_LEN;
            data = (uint8_t *)APP_DIS_SERIAL_NB_STR;
        } break;

        case DIS_HARD_REV_STR_CHAR:
        {
            /* Set information */
            len = APP_DIS_HARD_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_HARD_REV_STR;
        } break;

        case DIS_FIRM_REV_STR_CHAR:
        {
            /* Set information */
            len = APP_DIS_FIRM_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_FIRM_REV_STR;
        } break;

        case DIS_SW_REV_STR_CHAR:
        {
            /* Set information */
            len = APP_DIS_SW_REV_STR_LEN;
            data = (uint8_t *)APP_DIS_SW_REV_STR;
        } break;

        case DIS_IEEE_CHAR:
        {
            /* Set information */
            len = APP_DIS_IEEE_LEN;
            data = (uint8_t *)APP_DIS_IEEE;
        } break;

        default:
        {
        }
        break;
    }

    /* Allocate confirmation to send the value */
    struct diss_value_cfm *cfm_value = KE_MSG_ALLOC_DYN(DISS_VALUE_CFM,
                                                        src_id, dest_id,
                                                        diss_value_cfm,
                                                        len);

    /* Set parameters */
    cfm_value->value = param->value;
    cfm_value->length = len;
    if (len)
    {
        /* Copy data */
        memcpy(&cfm_value->data[0], data, len);
    }

    /* Send message */
    ke_msg_send(cfm_value);

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : void DeviceInfo_ServiceAdd(void)
 * ----------------------------------------------------------------------------
 * Description   : Send request to add device information in kernel and DB
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void DeviceInfo_ServiceAdd(void)
{
    struct diss_db_cfg *db_cfg;

    /* Allocate the DISS_CREATE_DB_REQ */
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(
        GAPM_PROFILE_TASK_ADD_CMD,
        TASK_GAPM,
        TASK_APP,
        gapm_profile_task_add_cmd,
        sizeof(struct
               diss_db_cfg));

    /* Fill message */
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = PERM(SVC_AUTH, DISABLE);
    req->prf_task_id = TASK_ID_DISS;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    /* Set Parameters */
    db_cfg = (struct diss_db_cfg *)req->param;
    db_cfg->features = APP_DIS_FEATURES;

    /* Send message */
    ke_msg_send(req);
}
