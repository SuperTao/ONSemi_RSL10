/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* drv_targ.c
* - The target driver initializes the system HW for the BootLoader.
* ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <rsl10.h>

#include "sys_boot.h"
#include "drv_targ.h"

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

static uint32_t volatile mod_sys_ticks;

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

void SysTick_Handler(void);

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
    Sys_NVIC_DisableAllInt();

    /* Reduce DCDC max current to avoid spikes */
    ACS_VCC_CTRL->ICH_TRIM_BYTE = VCC_ICHTRIM_80MA_BYTE;

    /* Enable or disable buck converter or LDO */
#if (CFG_BUCK_COV)
    ACS_VCC_CTRL->BUCK_ENABLE_ALIAS = VCC_BUCK_BITBAND;
#else    /* if (CFG_BUCK_COV) */
    ACS_VCC_CTRL->BUCK_ENABLE_ALIAS = VCC_LDO_BITBAND;
#endif    /* if (CFG_BUCK_COV) */

    /* Enable RF power switches */
    SYSCTRL_RF_POWER_CFG->RF_POWER_ALIAS = RF_POWER_ENABLE_BITBAND;

    /* Remove RF isolation */
    SYSCTRL_RF_ACCESS_CFG->RF_ACCESS_ALIAS = RF_ACCESS_ENABLE_BITBAND;

    /* Start the 48 MHz oscillator without changing the other register bits */
    RF->XTAL_CTRL = (RF->XTAL_CTRL & ~XTAL_CTRL_DISABLE_OSCILLATOR) |
                    XTAL_CTRL_REG_VALUE_SEL_INTERNAL;

    /* Enable 48 MHz oscillator divider at desired prescale value */
    RF_REG2F->CK_DIV_1_6_CK_DIV_1_6_BYTE = CK_DIV_1_6_PRESCALE_6_BYTE;

    /* Wait until 48 MHz oscillator is started */
    while (RF_REG39->ANALOG_INFO_CLK_DIG_READY_ALIAS !=
           ANALOG_INFO_CLK_DIG_READY_BITBAND);

    /* Switch to (divided 48 MHz) oscillator clock */
    Sys_Clocks_SystemClkConfig(SYSCLK_CLKSRC_RFCLK |
                               EXTCLK_PRESCALE_1   |
                               JTCK_PRESCALE_1);

    /* Enable CM3 loop cache. */
    SYSCTRL->CSS_LOOP_CACHE_CFG = CSS_LOOP_CACHE_ENABLE;

    /* Start 1ms system tick */
    mod_sys_ticks = 0;
    SysTick_Config(SystemCoreClock / 1000);

    /* Wait for Update activation go away */
    while (DIO_DATA->ALIAS[CFG_nUPDATE_DIO] == 0);

    /* Set LED indicator */
#ifdef CFG_LED_RED_DIO
    Sys_GPIO_Set_High(CFG_LED_RED_DIO);
#endif    /* ifdef CFG_LED_RED_DIO */

    __enable_irq();
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
 * Function      : uint_fast32_t Drv_Targ_GetTicks(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the accumulated system ticks.
 * Inputs        : None
 * Outputs       : return value     - number of 1ms ticks
 * Assumptions   :
 * ------------------------------------------------------------------------- */
uint_fast32_t Drv_Targ_GetTicks(void)
{
    return mod_sys_ticks;
}

/* ----------------------------------------------------------------------------
 * Function      : void SysTick_Handler(void);
 * ----------------------------------------------------------------------------
 * Description   : System tick interrupt handler.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    ++mod_sys_ticks;
}
