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
 * app_trace.h
 * - Trace application functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */

#ifndef APP_TRACE_H
#define APP_TRACE_H

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "config.h"

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

#define DBG_NO              0
#define DBG_UART            1    /* Note: if application uses UART, debugging over UART is not possible */
#define DBG_RTT             2    /* Note: for RTT debugging, please add SEGGER RTT files into your application */

#ifndef RSL10_DEBUG
#define RSL10_DEBUG         DBG_NO
#endif    /* RSL10_DEBUG */

#if (RSL10_DEBUG == DBG_RTT)

/* Include the Segger RTT files into the project */
#include <SEGGER_RTT.h>
#endif    /* if (RSL10_DEBUG == DBG_RTT) */

#define DMA_UART_TX         7
#define UART_TX             DBG_UART_TXD_DIO
#define UART_RX             DBG_UART_RXD_DIO
#define UART_BAUD_RATE      DBG_UART_BAUD_RATE
#define MAX_SIZE_STR        100

#define UART_TX_CFG                     (DMA_DISABLE                | \
                                         DMA_ADDR_LIN               | \
                                         DMA_TRANSFER_M_TO_P        | \
                                         DMA_PRIORITY_0             | \
                                         DMA_DISABLE_INT_DISABLE    | \
                                         DMA_ERROR_INT_DISABLE      | \
                                         DMA_COMPLETE_INT_ENABLE    | \
                                         DMA_COUNTER_INT_DISABLE    | \
                                         DMA_START_INT_DISABLE      | \
                                         DMA_LITTLE_ENDIAN          | \
                                         DMA_SRC_ADDR_INC           | \
                                         DMA_SRC_WORD_SIZE_32       | \
                                         DMA_SRC_ADDR_STEP_SIZE_1   | \
                                         DMA_DEST_ADDR_STATIC       | \
                                         DMA_DEST_UART              | \
                                         DMA_DEST_WORD_SIZE_8)

#define THREE_BLOCK_APPN(x, y, z)       x##y##z
#define DMA_IRQn(x)                     THREE_BLOCK_APPN(DMA, x, _IRQn)
#define DMA_IRQ_FUNC(x)                 THREE_BLOCK_APPN(DMA, x, _IRQHandler)

/* Assertions for critical errors */
#define ASSERT(msg)                                     \
    do                                                  \
    {                                                   \
        if (!(msg))                                     \
        {                                               \
            assert_error(__MODULE__, __LINE__, #msg);   \
        }                                               \
    } while (0)

#if (RSL10_DEBUG == DBG_NO)
#define TRACE_INIT()
#define PRINTF(...)
#else    /* if (RSL10_DEBUG == DBG_NO) */

/* Initialize trace port */
#define TRACE_INIT()            trace_init()
#if (RSL10_DEBUG == DBG_RTT)
#define PRINTF(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#elif (RSL10_DEBUG == DBG_UART)

#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, \
                      ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
/* #define PRINTF(...) { UART_printf(3, "%s: %d", __FILE__, __LINE__);\
 *  UART_printf(VA_NARGS(__VA_ARGS__), __VA_ARGS__);} */
#define PRINTF(...) UART_printf(VA_NARGS(__VA_ARGS__), __VA_ARGS__);

#endif    /* if (RSL10_DEBUG == DBG_RTT) */

#endif    /* if (RSL10_DEBUG == DBG_NO) */

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/
void assert_error (const char *file, const int line, const char *msg);

void trace_init (void);

int UART_printf(const int num, const char *sFormat, ...);

void DMA_IRQ_FUNC(DMA_UART_TX)(void);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_TRACE_H */
