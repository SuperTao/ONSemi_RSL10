/* ----------------------------------------------------------------------------
* Copyright (c) 2019 Semiconductor Components Industries, LLC (d/b/a ON
* Semiconductor). All Rights Reserved.
*
* This code is the property of ON Semiconductor and may not be redistributed
* in any form without prior written permission from ON Semiconductor.
* The terms of use and warranty for this code are covered by contractual
* agreements between ON Semiconductor and the licensee.
* ----------------------------------------------------------------------------
* drv_uart.h
* - Interface to the UART communication driver of the BootLoader.
* ------------------------------------------------------------------------- */

#ifndef _DRV_UART_H    /* avoids multiple inclusion */
#define _DRV_UART_H

#include <stdint.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define UART_WITH_FCS                   true
#define UART_WITHOUT_FCS                false

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef uint16_t Drv_Uart_fcs_t;

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the UART HW.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_Init(void);

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
void Drv_Uart_StartSend(void *msg_p, uint_fast16_t length, bool fcs_b);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_FinishSend(void)
 * ----------------------------------------------------------------------------
 * Description   : Waits for the complete transmission of a message.
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_FinishSend(void);

/* ----------------------------------------------------------------------------
 * Function      : void Drv_Uart_StartRecv(uint_fast16_t length)
 * ----------------------------------------------------------------------------
 * Description   : Starts receiving a message.
 * Inputs        : length           - length of message in octets
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void Drv_Uart_StartRecv(uint_fast16_t length);

/* ----------------------------------------------------------------------------
 * Function      : void * Drv_Uart_FinishRecv(void)
 * ----------------------------------------------------------------------------
 * Description   : Waits for the complete reception of a message
 *                 and returns it.
 * Inputs        : None
 * Outputs       : return value     - pointer to message
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void * Drv_Uart_FinishRecv(void);

#endif    /* _DRV_UART_H */
