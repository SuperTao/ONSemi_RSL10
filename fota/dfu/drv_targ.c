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
 * drv_targ.c
 * - The target driver initializes the system HW.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <rsl10.h>

#include "drv_targ.h"

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

static bool background_active;

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the basic target HW.
 *                 After the initialization all interrupts are disabled!
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Init(void)
{
    /* Disable all interrupts and clear any pending interrupts. */
    Sys_NVIC_DisableAllInt();
    Sys_NVIC_ClearAllPendingInt();

    /* Disable operation of DIO 13-15 in JTAG mode. */
    DIO_JTAG_SW_PAD_CFG->CM3_JTAG_DATA_EN_ALIAS = CM3_JTAG_DATA_DISABLED_BITBAND;
    DIO_JTAG_SW_PAD_CFG->CM3_JTAG_TRST_EN_ALIAS = CM3_JTAG_TRST_DISABLED_BITBAND;

    /* Configure the current trim settings for VCC, VDDA. */
    ACS_VCC_CTRL->ICH_TRIM_BYTE  = VCC_ICHTRIM_16MA_BYTE;
    ACS_VDDA_CP_CTRL->PTRIM_BYTE = VDDA_PTRIM_16MA_BYTE;

    /* Start and configure VDDRF. */
    ACS_VDDRF_CTRL->ENABLE_ALIAS = VDDRF_ENABLE_BITBAND;
    ACS_VDDRF_CTRL->CLAMP_ALIAS  = VDDRF_DISABLE_HIZ_BITBAND;

    /* Wait until VDDRF supply has powered up. */
    while (ACS_VDDRF_CTRL->READY_ALIAS != VDDRF_READY_BITBAND);

    /* Disable RF power amplifier. */
    ACS_VDDPA_CTRL->ENABLE_ALIAS = VDDPA_DISABLE_BITBAND;
    ACS_VDDPA_CTRL->VDDPA_SW_CTRL_ALIAS    = VDDPA_SW_VDDRF_BITBAND;

    /* Enable RF power switches. */
    SYSCTRL_RF_POWER_CFG->RF_POWER_ALIAS   = RF_POWER_ENABLE_BITBAND;

    /* Remove RF isolation. */
    SYSCTRL_RF_ACCESS_CFG->RF_ACCESS_ALIAS = RF_ACCESS_ENABLE_BITBAND;

    /* Set radio output power of RF */
    Sys_RFFE_SetTXPower(CFG_RF_POWER_DBM);

    /* Start the 48 MHz oscillator without changing the other register bits. */
    RF->XTAL_CTRL = (RF->XTAL_CTRL & ~XTAL_CTRL_DISABLE_OSCILLATOR)
                    | XTAL_CTRL_REG_VALUE_SEL_INTERNAL;

    /* Enable the 48 MHz oscillator divider using the desired pre-scale value. */
    RF_REG2F->CK_DIV_1_6_CK_DIV_1_6_BYTE = CK_DIV_1_6_PRESCALE_6_BYTE;

    /* Wait until 48 MHz oscillator is started. */
    while (RF_REG39->ANALOG_INFO_CLK_DIG_READY_ALIAS != ANALOG_INFO_CLK_DIG_READY_BITBAND);

    /* Switch to (divided 48 MHz) oscillator clock */
    Sys_Clocks_SystemClkConfig(SYSCLK_CLKSRC_RFCLK | EXTCLK_PRESCALE_1 | JTCK_PRESCALE_1);

    /* Configure clock dividers */
    Sys_Clocks_SystemClkPrescale0(SLOWCLK_PRESCALE_8 | BBCLK_PRESCALE_1 | USRCLK_PRESCALE_1);
    Sys_Clocks_SystemClkPrescale2(CPCLK_PRESCALE_8 | DCCLK_PRESCALE_2);

    /* Wake-up and apply clock to the BLE base-band interface. */
    BBIF->CTRL = (BB_CLK_ENABLE | BBCLK_DIVIDER_8 | BB_WAKEUP);

    /* Enable CM3 loop cache. */
    SYSCTRL->CSS_LOOP_CACHE_CFG = CSS_LOOP_CACHE_ENABLE;

    background_active = false;
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Poll(void);
 * ----------------------------------------------------------------------------
 * Description   : Polls the target driver.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Poll(void)
{
    Sys_Watchdog_Refresh();

    /* go to sleep mode only when no background task is active */
    if (!background_active)
    {
        /* activate sleep mode */
        SYS_WAIT_FOR_EVENT;
    }
    background_active = false;
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Reset(void);
 * ----------------------------------------------------------------------------
 * Description   : Performs a system reset.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Reset(void)
{
    NVIC_SystemReset();
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_SetBackgroundFlag(void);
 * ----------------------------------------------------------------------------
 * Description   : Sets the background task flag. This prevents the activation
 *                 of the CPU sleep mode in the current loop cycle.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_SetBackgroundFlag(void)
{
    background_active = true;
}
