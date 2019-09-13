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
 * app.c
 * - Main application file
 * ----------------------------------------------------------------------------
 * $Revision: 1.24 $
 * $Date: 2017/12/04 22:53:32 $
 * ------------------------------------------------------------------------- */

#include "app.h"

int main(void)
{
    uint32_t length;
    uint8_t temp[BUFFER_SIZE];

    /* Initialize the system */
    App_Initialize();

    /* Main application loop:
     * - Run the kernel scheduler
     * - Send notifications for the battery voltage and RSSI values
     * - Refresh the watchdog and wait for an interrupt before continuing */
    while (1)
    {
        Kernel_Schedule();

        if (unhandled_packets != NULL)
        {
            if (UART_FillTXBuffer(unhandled_packets->length,
                                  unhandled_packets->data) !=
                UART_ERRNO_OVERFLOW)
            {
                unhandled_packets = removeNode(unhandled_packets);
            }
        }

        if (ble_env.state == APPM_CONNECTED)
        {
            if (app_env.send_batt_ntf && bass_support_env.enable)
            {
                app_env.send_batt_ntf = 0;
                Batt_LevelUpdateSend(0, app_env.batt_lvl, 0);
            }

            if (cs_env.sentSuccess)
            {
                /* Copy data from the UART RX buffer to the TX buffer */
                length = UART_EmptyRXBuffer(temp);
                if (length > 0)
                {
                    /* Split buffer into two packets when it's greater than
                     * packet size */
                    if (length > PACKET_SIZE)
                    {
                        CustomService_SendNotification(ble_env.conidx,
                                                       CS_IDX_TX_VALUE_VAL,
                                                       temp,
                                                       PACKET_SIZE);
                        CustomService_SendNotification(ble_env.conidx,
                                                       CS_IDX_TX_VALUE_VAL,
                                                       &temp[PACKET_SIZE],
                                                       length - PACKET_SIZE);
                    }
                    else
                    {
                        CustomService_SendNotification(ble_env.conidx,
                                                       CS_IDX_TX_VALUE_VAL,
                                                       temp,
                                                       length);
                    }
                }
            }
        }

        /* Refresh the watchdog timer */
        Sys_Watchdog_Refresh();

        /* Wait for an event before executing the scheduler again */
        SYS_WAIT_FOR_EVENT;
    }
}
