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
 * ble_bass.h
 * - Bluetooth battery service header
 * ----------------------------------------------------------------------------
 * $Revision: 1.6 $
 * $Date: 2018/02/27 15:42:17 $
 * ------------------------------------------------------------------------- */

#ifndef BLE_BASS_H
#define BLE_BASS_H

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/
/* List of message handlers that are used by the battery service server
 * application manager */
#define BASS_MESSAGE_HANDLER_LIST                                   \
    DEFINE_MESSAGE_HANDLER(BASS_ENABLE_RSP, Batt_EnableRsp_Server), \
    DEFINE_MESSAGE_HANDLER(BASS_BATT_LEVEL_NTF_CFG_IND, Batt_LevelNtfCfgInd)

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/
struct bass_support_env_tag
{
    /* The current value of CCCD of battery value that has been set by
     * the client device */
    uint8_t batt_ntf_cfg;

    /* The flag that indicates that service has been enabled */
    bool enable;
};

/* Support for the application manager and the application environment */
extern struct bass_support_env_tag bass_support_env;

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/
extern void Bass_Env_Initialize(void);

extern void Batt_ServiceAdd_Server(void);

extern void Batt_ServiceEnable_Server(uint8_t conidx);

extern void Batt_LevelUpdateSend(uint8_t conidx, uint8_t batt_lvl,
                                 uint8_t bas_nb);

extern int Batt_EnableRsp_Server(ke_msg_id_t const msgid,
                                 struct bass_enable_rsp const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

extern int Batt_LevelNtfCfgInd(ke_msg_id_t const msgid,
                               struct bass_batt_level_ntf_cfg_ind const *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id);

extern int Batt_LevelUpdateRsp(ke_msg_id_t const msgid,
                               struct bass_batt_level_upd_rsp const *param,
                               ke_task_id_t const dest_id,
                               ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* BLE_BASS_H */
