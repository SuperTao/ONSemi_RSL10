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
 * app_dfu.h
 * - Interface to the DFU component.
 * ------------------------------------------------------------------------- */

#ifndef _APP_DFU_H    /* avoids multiple inclusion */
#define _APP_DFU_H

#include <stdbool.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define APP_DFU_STATES          \

#define APP_DFU_MSGS            \
    APP_DFU_SUPERVISOR_TIMER,   \

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void App_Dfu_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Dfu_Init(void);

/* ----------------------------------------------------------------------------
 * Function      : void App_Dfu_Poll(void)
 * ----------------------------------------------------------------------------
 * Description   : Poll routine for the event loop
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Dfu_Poll(void);

#endif    /* _APP_DFU_H */
