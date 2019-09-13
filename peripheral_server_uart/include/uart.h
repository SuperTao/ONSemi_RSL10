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
 * app_uart.h
 * - Main application header
 * ----------------------------------------------------------------------------
 * $Revision: 1.16 $
 * $Date: 2018/02/27 15:42:17 $
 * ------------------------------------------------------------------------- */

#ifndef UART_H_
#define UART_H_

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * Globals
 * --------------------------------------------------------------------------*/
extern uint32_t gNextData;
extern uint32_t *gUARTTXData;
extern struct overflow_packet *unhandled_packets;
extern uint8_t timerFlag;

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* UART Parameters */
#define CFG_SYS_CLK                     SystemCoreClock
#define CFG_DIO_RXD_UART                4
#define CFG_DIO_TXD_UART                5
#define CFG_UART_BAUD_RATE              115200
#define BUFFER_SIZE                     (PACKET_SIZE * 2)
#define POLL_SIZE                       25

#define DMA_RX_NUM                      1
#define DMA_RX_CFG                      (DMA_ENABLE                 | \
                                         DMA_ADDR_CIRC              | \
                                         DMA_TRANSFER_P_TO_M        | \
                                         DMA_PRIORITY_0             | \
                                         DMA_DISABLE_INT_DISABLE    | \
                                         DMA_ERROR_INT_DISABLE      | \
                                         DMA_COMPLETE_INT_ENABLE    | \
                                         DMA_COUNTER_INT_ENABLE     | \
                                         DMA_START_INT_DISABLE      | \
                                         DMA_LITTLE_ENDIAN          | \
                                         DMA_SRC_ADDR_STATIC        | \
                                         DMA_SRC_WORD_SIZE_32       | \
                                         DMA_SRC_UART               | \
                                         DMA_DEST_ADDR_INC          | \
                                         DMA_DEST_ADDR_STEP_SIZE_1  | \
                                         DMA_DEST_WORD_SIZE_32)

#define DMA_TX_NUM                      0
#define DMA_TX_CFG                      (DMA_ENABLE                 | \
                                         DMA_ADDR_LIN               | \
                                         DMA_TRANSFER_M_TO_P        | \
                                         DMA_PRIORITY_0             | \
                                         DMA_DISABLE_INT_DISABLE    | \
                                         DMA_ERROR_INT_DISABLE      | \
                                         DMA_COMPLETE_INT_DISABLE   | \
                                         DMA_COUNTER_INT_DISABLE    | \
                                         DMA_START_INT_DISABLE      | \
                                         DMA_LITTLE_ENDIAN          | \
                                         DMA_SRC_ADDR_INC           | \
                                         DMA_SRC_WORD_SIZE_32       | \
                                         DMA_SRC_ADDR_STEP_SIZE_1   | \
                                         DMA_DEST_ADDR_STATIC       | \
                                         DMA_DEST_UART              | \
                                         DMA_DEST_WORD_SIZE_32)

/* List of message handlers that are used by the different profiles/services */
#define UART_MESSAGE_HANDLER_LIST \
    DEFINE_MESSAGE_HANDLER(UART_TEST_TIMER, UART_Timer)

/* Set timer to 20 ms (2 times the 10 ms kernel timer resolution) */
#define TIMER_20MS_SETTING             2

/* UART Task messages */
enum uart_msg
{
    /* Timer used to have a tick periodically for UART */
    UART_TEST_TIMER,
};

/* Define error codes */
#define UART_ERRNO_NONE                 0
#define UART_ERRNO_OVERFLOW             1

typedef struct overflow_packet
{
    struct overflow_packet *next;
    uint8_t length;
    uint8_t data[];
} overflow_packet_t;

/* ----------------------------------------------------------------------------
 * Function Prototypes
 * --------------------------------------------------------------------------*/
extern void DMA1_IRQHandler(void);

extern void UART_Initialize(void);

/* UART support functions */
extern uint32_t UART_EmptyRXBuffer(uint8_t *data);

extern unsigned int UART_FillTXBuffer(uint32_t txSize, uint8_t *data);

extern overflow_packet_t * createNode(uint8_t length, uint8_t *data);

extern overflow_packet_t * removeNode(overflow_packet_t *head);

extern int UART_Timer(ke_msg_id_t const msg_id, void const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_UART_H_ */
