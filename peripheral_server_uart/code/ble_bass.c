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
 * ble_bass.c
 * - Bluetooth battery service server functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.7 $
 * $Date: 2017/12/05 17:44:23 $
 * ------------------------------------------------------------------------- */

#include "app.h"

/* Global variable definition */
struct bass_support_env_tag bass_support_env;

/* ----------------------------------------------------------------------------
 * Function      : void Bass_Env_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize battery service server service environment
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Bass_Env_Initialize(void)
{
    bass_support_env.batt_ntf_cfg = 0;
}

/* ----------------------------------------------------------------------------
 * Function      : void Batt_ServiceAdd_Server(void)
 * ----------------------------------------------------------------------------
 * Description   : Send request to add battery profile in kernel and DB
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Batt_ServiceAdd_Server(void)
{
    struct bass_db_cfg *db_cfg;
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(
        GAPM_PROFILE_TASK_ADD_CMD,
        TASK_GAPM,
        TASK_APP,
        gapm_profile_task_add_cmd,
        sizeof(struct bass_db_cfg));

    /* Fill message */
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl = PERM(SVC_AUTH, DISABLE);
    req->prf_task_id = TASK_ID_BASS;
    req->app_task = TASK_APP;
    req->start_hdl = 0;

    /* Set parameters  */
    db_cfg = (struct bass_db_cfg *)req->param;
    db_cfg->bas_nb = 1;
    db_cfg->features[0] = BAS_BATT_LVL_NTF_SUP;
    db_cfg->batt_level_pres_format[0].description = 0;
    db_cfg->batt_level_pres_format[0].exponent = 0;
    db_cfg->batt_level_pres_format[0].format = 0x4;
    db_cfg->batt_level_pres_format[0].name_space = 1;
    db_cfg->batt_level_pres_format[0].unit = ATT_UNIT_PERCENTAGE;

    /* Send the message */
    ke_msg_send(req);
}

/* ----------------------------------------------------------------------------
 * Function      : void Batt_ServiceEnable_Server(uint8_t conidx)
 * ----------------------------------------------------------------------------
 * Description   : Send request for enabling battery service to battery server
 * Inputs        : connection index
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Batt_ServiceEnable_Server(uint8_t conidx)
{
    struct bass_enable_req *req;
    struct bass_env_tag *bass_env;
    bass_env = PRF_ENV_GET(BASS, bass);

    /* Allocate the message to enable battery service server */
    req = KE_MSG_ALLOC(BASS_ENABLE_REQ,
                       prf_src_task_get(&(bass_env->prf_env), conidx),
                       TASK_APP, bass_enable_req);

    /* Fill in the parameter structure, initializing the old battery levels
     * to 0 as there are no previous measurements */
    req->conidx = conidx;
    req->ntf_cfg = (0 | 1);
    req->old_batt_lvl[0] = 0;
    req->old_batt_lvl[1] = 0;

    /* Send the message */
    ke_msg_send(req);
}

/* ----------------------------------------------------------------------------
 * Function      : void Batt_LevelUpdateSend(uint8_t conidx, uint8_t batt_lvl,
 *                                           uint8_t bas_nb)
 * ----------------------------------------------------------------------------
 * Description   : Send request for sending battery level notification
 * Inputs        : - conidx     - connection index
 *               : - batt_lvl   - battery level
 *               : - bas_nb     - battery server instance number
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Batt_LevelUpdateSend(uint8_t conidx, uint8_t batt_lvl, uint8_t bas_nb)
{
    struct bass_env_tag *bass_env;
    struct bass_batt_level_upd_req *req;
    bass_env = PRF_ENV_GET(BASS, bass);

    /* Allocate the battery level update request message */
    req = KE_MSG_ALLOC(BASS_BATT_LEVEL_UPD_REQ,
                       prf_src_task_get(&(bass_env->prf_env),
                                        conidx), TASK_APP,
                       bass_batt_level_upd_req);

    /* Fill in the parameter structure */
    req->bas_instance = bas_nb;
    req->batt_level = batt_lvl;

    /* Send the message */
    ke_msg_send(req);
}

/* ----------------------------------------------------------------------------
 * Function      : int Batt_LevelNtfCfgInd(ke_msg_id_t const msg_id,
 *                                          struct bass_batt_level_ntf_cfg_ind
 *                                          const *param,
 *                                          ke_task_id_t const dest_id,
 *                                          ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle indication of configuration changes for battery level
 *                 notification
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct bass_batt_level_ntf_cfg_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int Batt_LevelNtfCfgInd(ke_msg_id_t const msg_id,
                        struct bass_batt_level_ntf_cfg_ind const *param,
                        ke_task_id_t const dest_id,
                        ke_task_id_t const src_id)
{
    /* Store the new notification configuration */
    bass_support_env.batt_ntf_cfg = param->ntf_cfg;

    return (KE_MSG_CONSUMED);
}

/* ----------------------------------------------------------------------------
 * Function      : int Batt_EnableRsp_Server(ke_msg_id_t const msg_id,
 *                                     struct bass_enable_rsp
 *                                     const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle battery service enable response from battery task
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct bass_enable_rsp
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int Batt_EnableRsp_Server(ke_msg_id_t const msg_id,
                          struct bass_enable_rsp const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    if (param->status == GAP_ERR_NO_ERROR)
    {
        bass_support_env.enable = true;
    }

    return (KE_MSG_CONSUMED);
}
