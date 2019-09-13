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
 * fota_system.c
 * - Interworking layer for FOTA (visible to application)
 * ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <rsl10.h>
#include <rsl10_protocol.h>

#include "fota_system.h"

/* ----------------------------------------------------------------------------
 * Defines and Macros
 * --------------------------------------------------------------------------*/

#define TABLE_ENTRY(e)                  .e = e

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

void SystemFotaInit(void);

const struct prf_task_cbs * am0_has_prf_itf_get(void);

const struct prf_task_cbs * anpc_prf_itf_get(void);

const struct prf_task_cbs * anps_prf_itf_get(void);

const struct prf_task_cbs * basc_prf_itf_get(void);

const struct prf_task_cbs * bass_prf_itf_get(void);

const struct prf_task_cbs * bcsc_prf_itf_get(void);

const struct prf_task_cbs * bcss_prf_itf_get(void);

const struct prf_task_cbs * blpc_prf_itf_get(void);

const struct prf_task_cbs * blps_prf_itf_get(void);

const struct prf_task_cbs * cppc_prf_itf_get(void);

const struct prf_task_cbs * cpps_prf_itf_get(void);

const struct prf_task_cbs * cscpc_prf_itf_get(void);

const struct prf_task_cbs * cscps_prf_itf_get(void);

const struct prf_task_cbs * disc_prf_itf_get(void);

const struct prf_task_cbs * diss_prf_itf_get(void);

const struct prf_task_cbs * envc_prf_itf_get(void);

const struct prf_task_cbs * envs_prf_itf_get(void);

const struct prf_task_cbs * findl_prf_itf_get(void);

const struct prf_task_cbs * findt_prf_itf_get(void);

const struct prf_task_cbs * glpc_prf_itf_get(void);

const struct prf_task_cbs * glps_prf_itf_get(void);

const struct prf_task_cbs * hogpbh_prf_itf_get(void);

const struct prf_task_cbs * hogpd_prf_itf_get(void);

const struct prf_task_cbs * hogprh_prf_itf_get(void);

const struct prf_task_cbs * hrpc_prf_itf_get(void);

const struct prf_task_cbs * hrps_prf_itf_get(void);

const struct prf_task_cbs * htpc_prf_itf_get(void);

const struct prf_task_cbs * htpt_prf_itf_get(void);

const struct prf_task_cbs * ipsc_prf_itf_get(void);

const struct prf_task_cbs * ipss_prf_itf_get(void);

const struct prf_task_cbs * lanc_prf_itf_get(void);

const struct prf_task_cbs * lans_prf_itf_get(void);

const struct prf_task_cbs * paspc_prf_itf_get(void);

const struct prf_task_cbs * pasps_prf_itf_get(void);

const struct prf_task_cbs * plxc_prf_itf_get(void);

const struct prf_task_cbs * plxs_prf_itf_get(void);

const struct prf_task_cbs * proxm_prf_itf_get(void);

const struct prf_task_cbs * proxr_prf_itf_get(void);

const struct prf_task_cbs * rscpc_prf_itf_get(void);

const struct prf_task_cbs * rscps_prf_itf_get(void);

const struct prf_task_cbs * scppc_prf_itf_get(void);

const struct prf_task_cbs * scpps_prf_itf_get(void);

const struct prf_task_cbs * tipc_prf_itf_get(void);

const struct prf_task_cbs * tips_prf_itf_get(void);

const struct prf_task_cbs * udsc_prf_itf_get(void);

const struct prf_task_cbs * udss_prf_itf_get(void);

const struct prf_task_cbs * wptc_prf_itf_get(void);

const struct prf_task_cbs * wpts_prf_itf_get(void);

const struct prf_task_cbs * wscc_prf_itf_get(void);

const struct prf_task_cbs * wscs_prf_itf_get(void);

/* ----------------------------------------------------------------------------
 * FOTA stack configuration table
 * --------------------------------------------------------------------------*/

static const fota_config_t configuration =
{
    .libc =
    {
        TABLE_ENTRY(srand),
        TABLE_ENTRY(rand)
    },
    .sys =
    {
        TABLE_ENTRY(Device_Param_Prepare),
        TABLE_ENTRY(Sys_PowerModes_Sleep),
        TABLE_ENTRY(Sys_PowerModes_Standby)
    },
    .prf =
    {
        TABLE_ENTRY(am0_has_prf_itf_get),
        TABLE_ENTRY(anpc_prf_itf_get),
        TABLE_ENTRY(anps_prf_itf_get),
        TABLE_ENTRY(basc_prf_itf_get),
        TABLE_ENTRY(bass_prf_itf_get),
//        TABLE_ENTRY(bcsc_prf_itf_get),
//        TABLE_ENTRY(bcss_prf_itf_get),
        TABLE_ENTRY(blpc_prf_itf_get),
        TABLE_ENTRY(blps_prf_itf_get),
        TABLE_ENTRY(cppc_prf_itf_get),
        TABLE_ENTRY(cpps_prf_itf_get),
        TABLE_ENTRY(cscpc_prf_itf_get),
        TABLE_ENTRY(cscps_prf_itf_get),
        TABLE_ENTRY(disc_prf_itf_get),
        TABLE_ENTRY(diss_prf_itf_get),
//        TABLE_ENTRY(envc_prf_itf_get),
//        TABLE_ENTRY(envs_prf_itf_get),
        TABLE_ENTRY(findl_prf_itf_get),
        TABLE_ENTRY(findt_prf_itf_get),
        TABLE_ENTRY(glpc_prf_itf_get),
        TABLE_ENTRY(glps_prf_itf_get),
        TABLE_ENTRY(hogpbh_prf_itf_get),
        TABLE_ENTRY(hogpd_prf_itf_get),
        TABLE_ENTRY(hogprh_prf_itf_get),
        TABLE_ENTRY(hrpc_prf_itf_get),
        TABLE_ENTRY(hrps_prf_itf_get),
        TABLE_ENTRY(htpc_prf_itf_get),
        TABLE_ENTRY(htpt_prf_itf_get),
//        TABLE_ENTRY(ipsc_prf_itf_get),
//        TABLE_ENTRY(ipss_prf_itf_get),
        TABLE_ENTRY(lanc_prf_itf_get),
        TABLE_ENTRY(lans_prf_itf_get),
        TABLE_ENTRY(paspc_prf_itf_get),
        TABLE_ENTRY(pasps_prf_itf_get),
//        TABLE_ENTRY(plxc_prf_itf_get),
//        TABLE_ENTRY(plxs_prf_itf_get),
        TABLE_ENTRY(proxm_prf_itf_get),
        TABLE_ENTRY(proxr_prf_itf_get),
        TABLE_ENTRY(rscpc_prf_itf_get),
        TABLE_ENTRY(rscps_prf_itf_get),
        TABLE_ENTRY(scppc_prf_itf_get),
        TABLE_ENTRY(scpps_prf_itf_get),
        TABLE_ENTRY(tipc_prf_itf_get),
        TABLE_ENTRY(tips_prf_itf_get),
//        TABLE_ENTRY(udsc_prf_itf_get),
//        TABLE_ENTRY(udss_prf_itf_get),
        TABLE_ENTRY(wptc_prf_itf_get),
        TABLE_ENTRY(wpts_prf_itf_get),
//        TABLE_ENTRY(wscc_prf_itf_get),
//        TABLE_ENTRY(wscs_prf_itf_get),
    }
};

/* ----------------------------------------------------------------------------
 * Function      : void SystemFotaInit(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize the FOTA stack.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void SystemFotaInit(void)
{
    fota_init(&configuration);
}

/* ----------------------------------------------------------------------------
 * Function      : Device_Param_Prepare(app_device_param_t *param)
 * ----------------------------------------------------------------------------
 * Description   : Weak definition of the function in case that application
 *                 doesn't define it
 * Inputs        : - param    - Parameter identifier
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void __attribute((weak)) Device_Param_Prepare(app_device_param_t * param)
{
    param->device_param_src_type = FLASH_PROVIDED_or_DFLT;
    param->chnlAsses_param_src_type = FLASH_PROVIDED_or_DFLT;
}
