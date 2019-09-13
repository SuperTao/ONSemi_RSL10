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
 * ble_diss.h
 * - Device information service server functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.7 $
 * $Date: 2018/02/27 15:42:17 $
 * ------------------------------------------------------------------------- */

#ifndef BLE_DISS_H
#define BLE_DISS_H

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

#include <diss.h>
/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/
/* List of message handlers that are used by the device information service server
 * application manager */
#define DISS_MESSAGE_HANDLER_LIST \
    DEFINE_MESSAGE_HANDLER(DISS_VALUE_REQ_IND, DeviceInfo_ValueReqInd)

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/
struct diss_support_env_tag
{
    /* The flag that indicates that service has been enabled */
    bool enable;
};

/* Support for the application manager and the application environment */
extern struct diss_support_env_tag diss_support_env;

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/

/* Bluetooth Device Information (DISS) support functions */
extern void Diss_Env_Initialize(void);

extern void DeviceInfo_ServiceAdd(void);

extern int DeviceInfo_ValueReqInd(ke_msg_id_t const msgid,
                                  struct diss_value_req_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* BLE_DISS_H */
