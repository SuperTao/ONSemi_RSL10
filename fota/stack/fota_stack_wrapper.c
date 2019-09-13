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
 * fota_wrapper.c
 * - Wrapper functions for the FOTA stack callbacks
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <rsl10.h>
#include <rsl10_bb.h>
#include <rsl10_ble.h>

#include "sys_fota.h"
#include "fota_system.h"

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

static const fota_config_t *fota_config_p;

/* ----------------------------------------------------------------------------
 * Function      : void fota_init(const fota_config_t *config_p)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the FOTA stack
 * Inputs        : config_p             - pointer to configuration
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void fota_init(const fota_config_t *config_p)
{
    extern uint32_t __data_stack_init__;
    extern uint32_t __data_stack_start__;
    extern uint32_t __data_stack_end__;

    extern uint32_t __bss_stack_start__;
    extern uint32_t __bss_stack_end__;

    uint32_t *dst_p;
    uint32_t *src_p;

    /* Mask all interrupts */
    __set_PRIMASK(PRIMASK_DISABLE_INTERRUPTS);

    /* Initialize the data sections */
    dst_p = &__data_stack_start__;
    src_p = &__data_stack_init__;
    while (dst_p < &__data_stack_end__)
    {
        *dst_p++ = *src_p++;
    }

    /* Initialize the bss sections */
    dst_p = &__bss_stack_start__;
    while (dst_p < &__bss_stack_end__)
    {
        *dst_p++ = 0;
    }

    fota_config_p = config_p;
}

/* ----------------------------------------------------------------------------
 * Function      : void Sys_Fota_StartDfu(uint32_t mode)
 * ----------------------------------------------------------------------------
 * Description   : Starts the FOTA from the application
 * Inputs        : mode     0 = from stackless application
 *                          1 = from application with BLE stack
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Sys_Fota_StartDfu(uint32_t mode)
{
    /* Initialize system */
    if (mode != 0)
    {
      BLE_Reset();
	}
    Sys_Initialize();

    /* Start DFU */
    Sys_BootROM_StartApp((uint32_t *)APP_BASE_ADR);

    /* If DFU start failed -> wait for Reset */
    for (;;);
}

/* ----------------------------------------------------------------------------
 * libc wrappers
 * ------------------------------------------------------------------------- */

void __wrap_srand(unsigned int seed)
{
    fota_config_p->libc.srand(seed);
}

int __wrap_rand(void)
{
    return fota_config_p->libc.rand();
}

/* ----------------------------------------------------------------------------
 * sys wrappers
 * ------------------------------------------------------------------------- */

void __wrap_Device_Param_Prepare(app_device_param_t *param)
{
    return fota_config_p->sys.Device_Param_Prepare(param);
}

void __wrap_Sys_PowerModes_Sleep(struct sleep_mode_env_tag *sleep_mode_env)
{
    fota_config_p->sys.Sys_PowerModes_Sleep(sleep_mode_env);
}

void __wrap_Sys_PowerModes_Standby(struct standby_mode_env_tag *standby_mode_env)
{
    fota_config_p->sys.Sys_PowerModes_Standby(standby_mode_env);
}

/* ----------------------------------------------------------------------------
 * prf wrappers
 * ------------------------------------------------------------------------- */

const struct prf_task_cbs * __wrap_am0_has_prf_itf_get(void)
{
    return fota_config_p->prf.am0_has_prf_itf_get();
}

const struct prf_task_cbs * __wrap_anpc_prf_itf_get(void)
{
    return fota_config_p->prf.anpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_anps_prf_itf_get(void)
{
    return fota_config_p->prf.anps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_basc_prf_itf_get(void)
{
    return fota_config_p->prf.basc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_bass_prf_itf_get(void)
{
    return fota_config_p->prf.bass_prf_itf_get();
}

const struct prf_task_cbs * __wrap_blpc_prf_itf_get(void)
{
    return fota_config_p->prf.blpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_blps_prf_itf_get(void)
{
    return fota_config_p->prf.blps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_cppc_prf_itf_get(void)
{
    return fota_config_p->prf.cppc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_cpps_prf_itf_get(void)
{
    return fota_config_p->prf.cpps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_cscpc_prf_itf_get(void)
{
    return fota_config_p->prf.cscpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_cscps_prf_itf_get(void)
{
    return fota_config_p->prf.cscps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_disc_prf_itf_get(void)
{
    return fota_config_p->prf.disc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_diss_prf_itf_get(void)
{
    return fota_config_p->prf.diss_prf_itf_get();
}

const struct prf_task_cbs * __wrap_findl_prf_itf_get(void)
{
    return fota_config_p->prf.findl_prf_itf_get();
}

const struct prf_task_cbs * __wrap_findt_prf_itf_get(void)
{
    return fota_config_p->prf.findt_prf_itf_get();
}

const struct prf_task_cbs * __wrap_glpc_prf_itf_get(void)
{
    return fota_config_p->prf.glpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_glps_prf_itf_get(void)
{
    return fota_config_p->prf.glps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_hogpbh_prf_itf_get(void)
{
    return fota_config_p->prf.hogpbh_prf_itf_get();
}

const struct prf_task_cbs * __wrap_hogpd_prf_itf_get(void)
{
    return fota_config_p->prf.hogpd_prf_itf_get();
}

const struct prf_task_cbs * __wrap_hogprh_prf_itf_get(void)
{
    return fota_config_p->prf.hogprh_prf_itf_get();
}

const struct prf_task_cbs * __wrap_hrpc_prf_itf_get(void)
{
    return fota_config_p->prf.hrpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_hrps_prf_itf_get(void)
{
    return fota_config_p->prf.hrps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_htpc_prf_itf_get(void)
{
    return fota_config_p->prf.htpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_htpt_prf_itf_get(void)
{
    return fota_config_p->prf.htpt_prf_itf_get();
}

const struct prf_task_cbs * __wrap_lanc_prf_itf_get(void)
{
    return fota_config_p->prf.lanc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_lans_prf_itf_get(void)
{
    return fota_config_p->prf.lans_prf_itf_get();
}

const struct prf_task_cbs * __wrap_paspc_prf_itf_get(void)
{
    return fota_config_p->prf.paspc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_pasps_prf_itf_get(void)
{
    return fota_config_p->prf.pasps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_proxm_prf_itf_get(void)
{
    return fota_config_p->prf.proxm_prf_itf_get();
}

const struct prf_task_cbs * __wrap_proxr_prf_itf_get(void)
{
    return fota_config_p->prf.proxr_prf_itf_get();
}

const struct prf_task_cbs * __wrap_rscpc_prf_itf_get(void)
{
    return fota_config_p->prf.rscpc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_rscps_prf_itf_get(void)
{
    return fota_config_p->prf.rscps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_scppc_prf_itf_get(void)
{
    return fota_config_p->prf.scppc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_scpps_prf_itf_get(void)
{
    return fota_config_p->prf.scpps_prf_itf_get();
}

const struct prf_task_cbs * __wrap_tipc_prf_itf_get(void)
{
    return fota_config_p->prf.tipc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_tips_prf_itf_get(void)
{
    return fota_config_p->prf.tips_prf_itf_get();
}

const struct prf_task_cbs * __wrap_wptc_prf_itf_get(void)
{
    return fota_config_p->prf.wptc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_wpts_prf_itf_get(void)
{
    return fota_config_p->prf.wpts_prf_itf_get();
}

#if 0
const struct prf_task_cbs * __wrap_bcsc_prf_itf_get(void)
{
    return fota_config_p->prf.bcsc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_bcss_prf_itf_get(void)
{
    return fota_config_p->prf.bcss_prf_itf_get();
}

const struct prf_task_cbs * __wrap_envc_prf_itf_get(void)
{
    return fota_config_p->prf.envc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_envs_prf_itf_get(void)
{
    return fota_config_p->prf.envs_prf_itf_get();
}

const struct prf_task_cbs * __wrap_ipsc_prf_itf_get(void)
{
    return fota_config_p->prf.ipsc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_ipss_prf_itf_get(void)
{
    return fota_config_p->prf.ipss_prf_itf_get();
}

const struct prf_task_cbs * __wrap_plxc_prf_itf_get(void)
{
    return fota_config_p->prf.plxc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_plxs_prf_itf_get(void)
{
    return fota_config_p->prf.plxs_prf_itf_get();
}

const struct prf_task_cbs * __wrap_udsc_prf_itf_get(void)
{
    return fota_config_p->prf.udsc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_udss_prf_itf_get(void)
{
    return fota_config_p->prf.udss_prf_itf_get();
}

const struct prf_task_cbs * __wrap_wscc_prf_itf_get(void)
{
    return fota_config_p->prf.wscc_prf_itf_get();
}

const struct prf_task_cbs * __wrap_wscs_prf_itf_get(void)
{
    return fota_config_p->prf.wscs_prf_itf_get();
}

#endif /* if 0 */
