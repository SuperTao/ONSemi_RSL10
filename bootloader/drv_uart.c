/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* drv_uart.c
* - The UART driver implements the lower layers of the communication protocol
*   of the BootLoader.
* ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <rsl10.h>
#include <rsl10_flash.h>

#include "drv_uart.h"
#include "drv_targ.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define CRC_CCITT_SIZE                  sizeof(Drv_Uart_fcs_t)
#define CRC_CCITT_GOOD                  0xF0B8
#define CRC_CONFIG                      (CRC_CCITT                      | \
                                         CRC_LITTLE_ENDIAN              | \
                                         CRC_BIT_ORDER_NON_STANDARD     | \
                                         CRC_FINAL_REVERSE_NON_STANDARD)

#define NUM_RX_BUF                      2
#define RX_BUF_SIZE                     (FLASH_SECTOR_SIZE + CRC_CCITT_SIZE)
#define MAX_CHAR_DELAY                  20      /* in milliseconds */

#define DIV_CEIL(n, d)                  (((n) + (d) - 1) / (d))

/* DMA channel numbers for TX and RX */
#define DMA_TX_CH                       0
#define DMA_RX_CH                       1

/* DMA config for TX */
#define DMA_TX_CONFIG                   (DMA_LITTLE_ENDIAN        | \
                                         DMA_DISABLE              | \
                                         DMA_DISABLE_INT_DISABLE  | \
                                         DMA_ERROR_INT_DISABLE    | \
                                         DMA_COMPLETE_INT_DISABLE | \
                                         DMA_COUNTER_INT_DISABLE  | \
                                         DMA_START_INT_DISABLE    | \
                                         DMA_DEST_WORD_SIZE_8     | \
                                         DMA_SRC_WORD_SIZE_32     | \
                                         DMA_DEST_UART            | \
                                         DMA_PRIORITY_0           | \
                                         DMA_TRANSFER_M_TO_P      | \
                                         DMA_DEST_ADDR_STATIC     | \
                                         DMA_SRC_ADDR_INC         | \
                                         DMA_ADDR_LIN)

/* DMA config for RX */
#define DMA_RX_CONFIG                   (DMA_LITTLE_ENDIAN        | \
                                         DMA_DISABLE              | \
                                         DMA_DISABLE_INT_DISABLE  | \
                                         DMA_ERROR_INT_DISABLE    | \
                                         DMA_COMPLETE_INT_DISABLE | \
                                         DMA_COUNTER_INT_DISABLE  | \
                                         DMA_START_INT_DISABLE    | \
                                         DMA_DEST_WORD_SIZE_32    | \
                                         DMA_SRC_WORD_SIZE_8      | \
                                         DMA_SRC_UART             | \
                                         DMA_PRIORITY_0           | \
                                         DMA_TRANSFER_P_TO_M      | \
                                         DMA_DEST_ADDR_INC        | \
                                         DMA_SRC_ADDR_STATIC      | \
                                         DMA_ADDR_LIN)

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef struct
{
    uint32_t active;
    uint32_t data_a[NUM_RX_BUF][DIV_CEIL(RX_BUF_SIZE, sizeof(uint32_t))];
} rx_buffer_t;

rx_buffer_t Drv_Uart_rx_buffer;
static uint16_t mod_start_dma_cnt;

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the UART HW.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_Init(void)
{
    /* Initialize flow-control pins */
#ifdef CFG_UART_RTS_DIO
    Sys_DIO_Config(CFG_UART_RTS_DIO, DIO_MODE_GPIO_OUT_1 | DIO_2X_DRIVE);
#endif    /* ifdef CFG_UART_RTS_DIO */
#ifdef CFG_UART_CTS_DIO
    Sys_DIO_Config(CFG_UART_CTS_DIO,
                   DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_ENABLE);
#endif    /* ifdef CFG_UART_CTS_DIO */

    /* Configure TX and RX pin with correct current values */
    Sys_UART_DIOConfig(DIO_2X_DRIVE | DIO_LPF_DISABLE,
                       CFG_UART_TXD_DIO, CFG_UART_RXD_DIO);

    /* Enable device UART port */
    Sys_UART_Enable(SystemCoreClock, CFG_UART_BAUD_RATE, UART_DMA_MODE_ENABLE);

    /* Initialize DMA for TX channel */
    Sys_DMA_ChannelConfig(DMA_TX_CH, DMA_TX_CONFIG, 0, 0,
                          0, (uint32_t)&UART->TX_DATA);

    Drv_Uart_rx_buffer.active = 0;
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_StartSend(void *msg_p, uint_fast16_t length,
 *                                         bool fcs_b)
 * ----------------------------------------------------------------------------
 * Description   : Starts sending a message.
 * Inputs        : msg_p            - pointer to message
 *                                    (must have an alignment of 4)
 *                 length           - length of message in octets
 *                                    (including optional FCS)
 *                 fcs_b            - UART_WITH_FCS
 *                                    appends a FCS to the message
 *                                  - UART_WITHOUT_FCS
 *                                    no FCS is appended
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_StartSend(void *msg_p, uint_fast16_t length, bool fcs_b)
{
    uint_fast16_t cnt;
    uint_fast16_t crc;
    uint8_t      *data_p;

    if (fcs_b)
    {
        /* Select correct CRC algorithm for FCS */
        Sys_CRC_Set_Config(CRC_CONFIG | CRC_FINAL_XOR_NON_STANDARD);
        CRC->VALUE = CRC_CCITT_INIT_VALUE;
        for (data_p = msg_p, cnt = length - CRC_CCITT_SIZE; cnt > 0; cnt--)
        {
            CRC->ADD_8 = *data_p++;
        }
        crc = CRC->FINAL;
        *data_p++ = (crc >> 0);
        *data_p   = (crc >> 8);
    }

    /* Wait for completion of previous transfer */
    while (DMA_CTRL0[DMA_TX_CH].ENABLE_ALIAS);

    /* Start new transfer */
    DMA_CTRL1[DMA_TX_CH].TRANSFER_LENGTH_SHORT = length;
    Sys_DMA_Set_ChannelSourceAddress(DMA_TX_CH, (uint32_t)msg_p);
    Sys_DMA_ChannelEnable(DMA_TX_CH);
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_FinishSend(void)
 * ----------------------------------------------------------------------------
 * Description   : Waits for the complete transmission of a message.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_FinishSend(void)
{
    uint32_t cycles = 20 * SystemCoreClock / CFG_UART_BAUD_RATE;

    /* Wait for completion of the DMA transfer */
    while (DMA_CTRL0[DMA_TX_CH].ENABLE_ALIAS);

    /* Wait for completely shifted out the last character */
    Sys_Delay_ProgramROM(cycles);
}

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_StartRecv(uint_fast16_t length)
 * ----------------------------------------------------------------------------
 * Description   : Starts receiving a message.
 * Inputs        : length           - length of message in octets
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_StartRecv(uint_fast16_t length)
{
    length += CRC_CCITT_SIZE;

    /* Start new transfer */
    Sys_DMA_ChannelConfig(DMA_RX_CH, DMA_RX_CONFIG,
                          length, 0,
                          (uint32_t)&UART->RX_DATA,
                          (uint32_t)Drv_Uart_rx_buffer.data_a[Drv_Uart_rx_buffer.active]);
    mod_start_dma_cnt = DMA->WORD_CNT[DMA_RX_CH];
    Sys_DMA_ChannelEnable(DMA_RX_CH);
#ifdef CFG_UART_RTS_DIO
    Sys_GPIO_Set_Low(CFG_UART_RTS_DIO);
#endif    /* ifdef CFG_UART_RTS_DIO */
}

/* ----------------------------------------------------------------------------
 * Function      : void * Drv_Uart_FinishRecv(void)
 * ----------------------------------------------------------------------------
 * Description   : Waits for the complete reception of a message and returns
 *                 it.
 * Inputs        : None
 * Outputs       : return value     - pointer to message
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void * Drv_Uart_FinishRecv(void)
{
    uint_fast32_t tick_cnt;
    uint_fast16_t dma_cnt;
    uint_fast16_t length;
    void         *result_p;
    uint8_t      *data_p;

    length   = DMA_CTRL1[DMA_RX_CH].TRANSFER_LENGTH_SHORT;
    result_p = Drv_Uart_rx_buffer.data_a[Drv_Uart_rx_buffer.active];

    /* Wait for completion of current transfer with character timeout */
    tick_cnt = Drv_Targ_GetTicks();
    dma_cnt  = mod_start_dma_cnt;
    while (DMA_CTRL0[DMA_RX_CH].ENABLE_ALIAS)
    {
        if (dma_cnt != DMA->WORD_CNT[DMA_RX_CH])
        {
            dma_cnt  = DMA->WORD_CNT[DMA_RX_CH];
            tick_cnt = Drv_Targ_GetTicks();
        }
        else if (Drv_Targ_GetTicks() - tick_cnt > MAX_CHAR_DELAY)
        {
            Sys_DMA_ChannelDisable(DMA_RX_CH);
            return NULL;
        }
    }

#ifdef CFG_UART_RTS_DIO
    Sys_GPIO_Set_High(CFG_UART_RTS_DIO);
#endif    /* ifdef CFG_UART_RTS_DIO */

    /* Check FCS of received message */
    Sys_CRC_Set_Config(CRC_CONFIG);
    CRC->VALUE = CRC_CCITT_INIT_VALUE;
    for (data_p  = result_p;
         length >= sizeof(uint32_t);
         length -= sizeof(uint32_t))
    {
        CRC->ADD_32 = *(uint32_t *)data_p;
        data_p += sizeof(uint32_t);
    }
    if (length >= sizeof(uint16_t))
    {
        CRC->ADD_16 = *(uint16_t *)data_p;
        data_p += sizeof(uint16_t);
        length -= sizeof(uint16_t);
    }
    if (length > 0)
    {
        CRC->ADD_8 = *data_p;
    }
    if (CRC->FINAL != CRC_CCITT_GOOD)
    {
        return NULL;
    }

    Drv_Uart_rx_buffer.active = (Drv_Uart_rx_buffer.active + 1) % NUM_RX_BUF;
    return result_p;
}
