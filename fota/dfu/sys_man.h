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
 * sys_man.h
 * - Interface to the System Management.
 * ------------------------------------------------------------------------- */

#ifndef _SYS_MAN_H    /* avoids multiple inclusion */
#define _SYS_MAN_H

#include <stdbool.h>
#include <rsl10.h>
#include <rsl10_ke.h>

#include "app_dfu.h"
#include "app_hdlc.h"
#include "app_ble.h"
#include "app_conf.h"

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    SYS_MAN_STATE_INIT,

    /* application states */
    APP_BLE_STATES
    APP_HDLC_STATES
    APP_DFU_STATES

    SYS_MAN_STATE_MAX
} Sys_Man_app_state_t;

typedef enum
{
    SYS_MAN_STATE_CHANGE_IND = TASK_FIRST_MSG(TASK_ID_APP),

    /* application messages */
    APP_BLE_MSGS
    APP_HDLC_MSGS
    APP_DFU_MSGS

    SYS_MAN_LAST_MSG
} Sys_Man_app_msg_t;

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : Sys_Man_app_state_t Sys_Man_GetAppState(ke_task_id_t id)
 * ----------------------------------------------------------------------------
 * Description   : Get the application state
 * Inputs        : id               - application task ID
 * Outputs       : return value     - current application state
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static inline Sys_Man_app_state_t Sys_Man_GetAppState(ke_task_id_t id)
{
    return (Sys_Man_app_state_t)ke_state_get(id);
}

/* ----------------------------------------------------------------------------
 * Function      : void Sys_Man_SetAppState(ke_task_id_t        id,
 *                                          Sys_Man_app_state_t state)
 * ----------------------------------------------------------------------------
 * Description   : Set the application state
 * Inputs        : id               - application task ID
 * Inputs        : state            - new application state
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static inline void Sys_Man_SetAppState(ke_task_id_t id,
                                       Sys_Man_app_state_t state)
{
    ke_state_set(id, (ke_state_t)state);
    ke_msg_send_basic(SYS_MAN_STATE_CHANGE_IND, id, id);
}

#endif    /* _SYS_MAN_H */
