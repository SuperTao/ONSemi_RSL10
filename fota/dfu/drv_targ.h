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
 * drv_targ.h
 * - Interface to the target driver.
 * ------------------------------------------------------------------------- */

#ifndef _DRV_TARG_H    /* avoids multiple inclusion */
#define _DRV_TARG_H

#include <stdbool.h>

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the basic target HW.
 *                 After the initialization all interrupts are disabled!
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Init(void);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Poll(void);
 * ----------------------------------------------------------------------------
 * Description   : Polls the target driver.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Poll(void);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_Reset(void);
 * ----------------------------------------------------------------------------
 * Description   : Performs a system reset.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_Reset(void);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Targ_SetBackgroundFlag(void);
 * ----------------------------------------------------------------------------
 * Description   : Sets the background task flag. This prevents the activation
 *                 of the CPU sleep mode in the current loop cycle.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Targ_SetBackgroundFlag(void);

#endif    /* _DRV_TARG_H */
