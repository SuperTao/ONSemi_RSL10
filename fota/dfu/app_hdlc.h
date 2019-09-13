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
 * app_hdlc.h
 * - Interface to the DFU data link protocol.
 * ------------------------------------------------------------------------- */

#ifndef _APP_HDLC_H    /* avoids multiple inclusion */
#define _APP_HDLC_H

#include <stdbool.h>
#include <stdint.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define APP_HDLC_STATES \

#define APP_HDLC_MSGS   \
    APP_HDLC_ACKPEND,   \
    APP_HDLC_T200,      \

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void App_Hdlc_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Hdlc_Init(void);

/* ----------------------------------------------------------------------------
 * Function      : bool App_Hdlc_DataReq(const uint8_t *data_p,
 *                                       uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Requests sending data
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to SDU
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  peer receiver is ready
 *                                      (data was accepted)
 * Outputs       : return value     - false peer receiver is busy
 *                                      (data was not accepted)
 * Assumptions   : link is up
 * ------------------------------------------------------------------------- */
bool App_Hdlc_DataReq(uint_fast8_t link,
                      const uint8_t *data_p, uint_fast16_t size);

/* ----------------------------------------------------------------------------
 * Function      : void App_Hdlc_DataCfm(uint_fast8_t   link,
 *                                       const uint8_t *data_p)
 * ----------------------------------------------------------------------------
 * Description   : Confirms sending data and acknowledging it by peer
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to data
 * Outputs       : None
 * Assumptions   : link is up
 * ------------------------------------------------------------------------- */
void App_Hdlc_DataCfm(uint_fast8_t link, const uint8_t *data_p);

/* ----------------------------------------------------------------------------
 * Function      : bool App_Hdlc_DataInd(const uint8_t *data_p,
 *                                       uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Indicates received data
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to SDU
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  receiver is ready for more data
 * Outputs       : return value     - false receiver is now busy
 * Assumptions   : link is up
 * ------------------------------------------------------------------------- */
bool App_Hdlc_DataInd(uint_fast8_t link,
                      const uint8_t *data_p, uint_fast16_t size);

#endif    /* _APP_HDLC_H */
