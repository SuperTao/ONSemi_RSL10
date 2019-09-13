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
 * fota_system.h
 * - Interworking layer for FOTA (visible to application)
 * ------------------------------------------------------------------------- */

#ifndef _FOTA_SYSTEM__H
#define _FOTA_SYSTEM__H

#include <stdint.h>
#include <stdlib.h>

#include <rsl10_protocol.h>
#include <rsl10_sys_power_modes.h>
#include <prf.h>

/* ----------------------------------------------------------------------------
 * Variables and types
 * --------------------------------------------------------------------------*/

typedef void srand_t (unsigned int seed);
typedef int rand_t (void);

typedef struct
{
    srand_t *srand;
    rand_t  *rand;
} fota_libc_wrapper_table;

typedef void Device_Param_Prepare_t (app_device_param_t *param);
typedef void Sys_PowerModes_Sleep_t (struct sleep_mode_env_tag *sleep_mode_env);
typedef void Sys_PowerModes_Standby_t (struct standby_mode_env_tag *standby_mode_env);

typedef struct
{
    Device_Param_Prepare_t    *Device_Param_Prepare;
    Sys_PowerModes_Sleep_t    *Sys_PowerModes_Sleep;
    Sys_PowerModes_Standby_t  *Sys_PowerModes_Standby;
} fota_sys_wrapper_table;

typedef const struct prf_task_cbs *fota_prf_wrapper_t (void);

typedef struct
{
    fota_prf_wrapper_t * am0_has_prf_itf_get;
    fota_prf_wrapper_t * anpc_prf_itf_get;
    fota_prf_wrapper_t * anps_prf_itf_get;
    fota_prf_wrapper_t * basc_prf_itf_get;
    fota_prf_wrapper_t * bass_prf_itf_get;
//    fota_prf_wrapper_t * bcsc_prf_itf_get;
//    fota_prf_wrapper_t * bcss_prf_itf_get;
    fota_prf_wrapper_t * blpc_prf_itf_get;
    fota_prf_wrapper_t * blps_prf_itf_get;
    fota_prf_wrapper_t * cppc_prf_itf_get;
    fota_prf_wrapper_t * cpps_prf_itf_get;
    fota_prf_wrapper_t * cscpc_prf_itf_get;
    fota_prf_wrapper_t * cscps_prf_itf_get;
    fota_prf_wrapper_t * disc_prf_itf_get;
    fota_prf_wrapper_t * diss_prf_itf_get;
//    fota_prf_wrapper_t * envc_prf_itf_get;
//    fota_prf_wrapper_t * envs_prf_itf_get;
    fota_prf_wrapper_t * findl_prf_itf_get;
    fota_prf_wrapper_t * findt_prf_itf_get;
    fota_prf_wrapper_t * glpc_prf_itf_get;
    fota_prf_wrapper_t * glps_prf_itf_get;
    fota_prf_wrapper_t * hogpbh_prf_itf_get;
    fota_prf_wrapper_t * hogpd_prf_itf_get;
    fota_prf_wrapper_t * hogprh_prf_itf_get;
    fota_prf_wrapper_t * hrpc_prf_itf_get;
    fota_prf_wrapper_t * hrps_prf_itf_get;
    fota_prf_wrapper_t * htpc_prf_itf_get;
    fota_prf_wrapper_t * htpt_prf_itf_get;
//    fota_prf_wrapper_t * ipsc_prf_itf_get;
//    fota_prf_wrapper_t * ipss_prf_itf_get;
    fota_prf_wrapper_t * lanc_prf_itf_get;
    fota_prf_wrapper_t * lans_prf_itf_get;
    fota_prf_wrapper_t * paspc_prf_itf_get;
    fota_prf_wrapper_t * pasps_prf_itf_get;
//    fota_prf_wrapper_t * plxc_prf_itf_get;
//    fota_prf_wrapper_t * plxs_prf_itf_get;
    fota_prf_wrapper_t * proxm_prf_itf_get;
    fota_prf_wrapper_t * proxr_prf_itf_get;
    fota_prf_wrapper_t * rscpc_prf_itf_get;
    fota_prf_wrapper_t * rscps_prf_itf_get;
    fota_prf_wrapper_t * scppc_prf_itf_get;
    fota_prf_wrapper_t * scpps_prf_itf_get;
    fota_prf_wrapper_t * tipc_prf_itf_get;
    fota_prf_wrapper_t * tips_prf_itf_get;
//    fota_prf_wrapper_t * udsc_prf_itf_get;
//    fota_prf_wrapper_t * udss_prf_itf_get;
    fota_prf_wrapper_t * wptc_prf_itf_get;
    fota_prf_wrapper_t * wpts_prf_itf_get;
//    fota_prf_wrapper_t * wscc_prf_itf_get;
//    fota_prf_wrapper_t * wscs_prf_itf_get;
} fota_prf_wrapper_table;

typedef struct
{
    fota_libc_wrapper_table libc;
    fota_sys_wrapper_table sys;
    fota_prf_wrapper_table prf;
} fota_config_t;

/* ----------------------------------------------------------------------------
 * Function      : void fota_init(const fota_config_t *config_p);
 * ----------------------------------------------------------------------------
 * Description   : Initializes the FOTA stack
 * Inputs        : config_p             - pointer to configuration
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void fota_init(const fota_config_t *config_p);

#endif    /* _FOTA_SYSTEM__H */
