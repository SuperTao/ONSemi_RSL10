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
 * sys_man.c
 * - This is the system management module. It contains the system
 *   initialization and the event loop.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdlib.h>
#include <rsl10.h>
#include <rsl10_ke.h>

#include "sys_man.h"
#include "app_dfu.h"
#include "app_hdlc.h"
#include "app_ble.h"
#include "app_trace.h"
#include "drv_targ.h"
#include "msg_handler.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

static const struct ke_msg_handler default_state[] =
{
    { KE_MSG_DEFAULT_HANDLER, (ke_msg_func_t)MsgHandler_Notify }
};

/* Use the state and event handler definition for all states. */
static const struct ke_state_handler default_handler
    = KE_STATE_HANDLER(default_state);

/* Defines a place holder for all task instance's state */
static ke_state_t state_a[CFG_HDLC_NB_LINKS];

static const struct ke_task_desc TASK_DESC_APP =
{
    .state_handler = NULL,
    .default_handler = &default_handler,
    .state = state_a,
    .state_max = SYS_MAN_STATE_MAX,
    .idx_max = CFG_HDLC_NB_LINKS
};

/* ----------------------------------------------------------------------------
 * Function      : static void Init(void)
 * ----------------------------------------------------------------------------
 * Description   : System Initialization.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void Init(void)
{
    /* Debug/trace initialization. In order to enable UART or RTT trace,
     * configure the 'RSL10_DEBUG' macro in config.h */
    TRACE_INIT();

    /* Seed the random number generator */
    srand(1);

    /* Initialize HW drivers */
    Drv_Targ_Init();

    /* Initialize the kernel scheduler */
    Kernel_Init(0);

    /* Create the application task handler */
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    /* Initialize modules */
    App_Dfu_Init();
    App_Hdlc_Init();
    App_Ble_Init();
}

/* ----------------------------------------------------------------------------
 * Function      : int main(void)
 * ----------------------------------------------------------------------------
 * Description   : Main routine of the Updater.
 * Inputs        : None
 * Outputs       : Returns always 0
 * Assumptions   :
 * ------------------------------------------------------------------------- */
int main(void)
{
    Init();

    /* kernel scheduler loop */
    for (;;)
    {
        /* Handle the kernel scheduler */
        Kernel_Schedule();

        /* Polling the modules */
        App_Dfu_Poll();
        Drv_Targ_Poll();
    }

    /* never get here */
    return 0;
}
