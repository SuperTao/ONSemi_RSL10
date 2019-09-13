/* ----------------------------------------------------------------------------
 * Copyright (c) 2017 Semiconductor Components Industries, LLC (d/b/a
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
 * app_trace.c
 * - Trace application functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */

#include "app_trace.h"
#include <rsl10.h>

#if (RSL10_DEBUG == DBG_UART)
char UARTTXBuffer[MAX_SIZE_STR];
volatile uint8_t tx_busy = 0;
#endif    /* if (RSL10_DEBUG == DBG_UART) */

/* ----------------------------------------------------------------------------
 * Function      : void trace_init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize trace application. Only used for UART debug port.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : The UART port, two DIO and one DMA should be reserved.
 * ------------------------------------------------------------------------- */
void trace_init(void)
{
#if (RSL10_DEBUG == DBG_UART)
    Sys_UART_DIOConfig(DIO_6X_DRIVE | DIO_WEAK_PULL_UP | DIO_LPF_ENABLE,
                       UART_TX, UART_RX);
    Sys_UART_Enable(SystemCoreClock, UART_BAUD_RATE, UART_DMA_MODE_ENABLE);

    Sys_DMA_ChannelConfig(DMA_UART_TX, UART_TX_CFG, 1, 0,
                          (uint32_t)UARTTXBuffer, (uint32_t)&UART->TX_DATA);
    NVIC_SetPriority(DMA_IRQn(DMA_UART_TX), 0x4);
    NVIC_EnableIRQ(DMA_IRQn(DMA_UART_TX));
#elif (RSL10_DEBUG == DBG_RTT)
#endif    /* if (RSL10_DEBUG == DBG_UART) */
}

/* ----------------------------------------------------------------------------
 * Function      : void assert_error (const char *file, const int line,
 *                                      const char *msg)
 * ----------------------------------------------------------------------------
 * Description   : Assertion function. Resets the application in non-debug
 *                 mode. Stay in a dead loop in debug mode.
 * Inputs        : - file - name of the file that calls the function
 *                 - line - line number that the assert happened
 *                 - msg  - the source of error
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void assert_error (const char *file, const int line, const char *msg)
{
    PRINTF("%s: %d: %s\r\n", file, line, msg);
#if (RSL10_DEBUG == DBG_NO)
    NVIC_SystemReset();
#else    /* if (RSL10_DEBUG == DBG_NO) */
    __disable_irq();
    while (1)
    {
        Sys_Watchdog_Refresh();
    }
#endif    /* if (RSL10_DEBUG == DBG_NO) */
}

#if (RSL10_DEBUG == DBG_UART)
/* ----------------------------------------------------------------------------
 * Function      : void DMA_IRQ_FUNC(DMA_UART_TX)(void)
 * ----------------------------------------------------------------------------
 * Description   : DMA interrupt service routing to clear busy flag.
 * Inputs        : None
 * Outputs       : None
 * ------------------------------------------------------------------------- */
void DMA_IRQ_FUNC(DMA_UART_TX)(void)
{
    tx_busy = 0;
    Sys_DMA_ClearChannelStatus(DMA_UART_TX);
}

/* ----------------------------------------------------------------------------
 * Function      : int UART_printf(int narg, const char *sFormat, ...)
 * ----------------------------------------------------------------------------
 * Description   : Printf function for the UART debug port
 * Inputs        : - narg    - Number of argument to print
 *                 - sFormat - Pointer to the argument list
 * Outputs       : None
 * Assumptions   : The UART port and the DMA has been initialized.
 * ------------------------------------------------------------------------- */
int UART_printf(const int narg, const char *sFormat, ...)
{
    va_list arg;
    int done = 0;

    /* Check until the DMA is not busy */
    while (tx_busy == 1)
    {
        Sys_Watchdog_Refresh();
    }

    if (narg > 1)
    {
        /* Use sprint if there is more than one argument */
        va_start(arg, sFormat);
        done = vsprintf(UARTTXBuffer, sFormat, arg);
        va_end(arg);
        DMA_CTRL1[DMA_UART_TX].TRANSFER_LENGTH_SHORT = strlen((const char *)UARTTXBuffer);
    }
    else
    {
        /* Use simple memcpy if there is one argument */
        DMA_CTRL1[DMA_UART_TX].TRANSFER_LENGTH_SHORT = strlen(sFormat);
        memcpy(UARTTXBuffer, sFormat, DMA_CTRL1[DMA_UART_TX].TRANSFER_LENGTH_SHORT);
    }

    /* The DMA is busy */
    tx_busy = 1;
    Sys_DMA_ChannelEnable(DMA_UART_TX);

    return done;
}

#endif    /* if (RSL10_DEBUG == DBG_UART) */
