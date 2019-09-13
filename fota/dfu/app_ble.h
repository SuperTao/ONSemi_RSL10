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
 * app_ble.h
 * - Interface to the application specific BLE part.
 * ------------------------------------------------------------------------- */

#ifndef _APP_BLE_H    /* avoids multiple inclusion */
#define _APP_BLE_H

#include <stdbool.h>
#include <stdint.h>

#include <rsl10_ke.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define APP_BLE_STATES      \
    APP_BLE_ADVERTISING,    \
    APP_BLE_CONNECTED,      \
    APP_BLE_LINKUP,         \
    APP_BLE_DISCONNECTED,   \

#define APP_BLE_MSGS        \

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    APP_BLE_SUCCESS,
    APP_BLE_LINK_DOWN,
    APP_BLE_FAILURE
} App_Ble_status_t;

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Ble_Init(void);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_StartAdvertising(ke_task_id_t id)
 * ----------------------------------------------------------------------------
 * Description   : Starts BLE advertising.
 * Inputs        : id               - application task ID
 * Outputs       : None
 * Assumptions   : Only valid in state APP_BLE_DISCONNECTED
 * ------------------------------------------------------------------------- */
void App_Ble_StartAdvertising(ke_task_id_t id);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_ActivationInd(uint_fast8_t  link,
 *                                            uint_fast16_t max_size)
 * ----------------------------------------------------------------------------
 * Description   : Indicates link activation
 * Inputs        : link             - link ID
 * Inputs        : max_size         - max. data size for App_Ble_DataReq
 * Outputs       : None
 * Assumptions   : link is inactive
 * ------------------------------------------------------------------------- */
void App_Ble_ActivationInd(uint_fast8_t link, uint_fast16_t max_size);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DeactivationInd(uint_fast8_t  link)
 * ----------------------------------------------------------------------------
 * Description   : Indicates link deactivation
 * Inputs        : link             - link ID
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DeactivationInd(uint_fast8_t link);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DataReq(uint_fast8_t   link,
 *                                      uint_fast16_t  seq_nb,
 *                                      const uint8_t *data_p,
 *                                      uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Requests sending data
 * Inputs        : link             - link ID
 * Inputs        : seq_nb           - sequence number
 * Inputs        : data_p           - pointer to data
 * Inputs        : size             - size of data
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DataReq(uint_fast8_t link, uint_fast16_t seq_nb,
                     const uint8_t *data_p, uint_fast16_t size);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DataCfm(uint_fast8_t      link,
 *                                      uint_fast16_t     seq_nb,
 *                                      App_Hdlc_status_t status)
 * ----------------------------------------------------------------------------
 * Description   : Confirms processing App_Hdlc_DataReq
 * Inputs        : link             - link ID
 * Inputs        : seq_nb           - sequence number
 * Inputs        : status           - request status
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DataCfm(uint_fast8_t link, uint_fast16_t seq_nb,
                     App_Ble_status_t status);

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DataInd(uint_fast8_t   link,
 *                                      const uint8_t *data_p,
 *                                      uint_fast16_t  size)
 * ----------------------------------------------------------------------------
 * Description   : Indicates received data
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to data
 * Inputs        : size             - size of data
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DataInd(uint_fast8_t link,
                     const uint8_t *data_p, uint_fast16_t size);

#endif    /* _APP_BLE_H */
