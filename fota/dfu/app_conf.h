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
 * app_conf.h
 * - Interface to the DFU component configuration database.
 * ------------------------------------------------------------------------- */

#ifndef _APP_CONF_H    /* avoids multiple inclusion */
#define _APP_CONF_H

#include <stdint.h>
#include <gap.h>
#include "sys_boot.h"
#include "sys_fota.h"

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    APP_CONF_BOOT_VERSION,
    APP_CONF_STACK_VERSION,
    APP_CONF_APP_VERSION
} App_Conf_version_type_t;

typedef const Sys_Fota_uuid_t App_Conf_uuid_t;
typedef const Sys_Boot_app_version_t App_Conf_version_t;
typedef const uint8_t App_Conf_build_id_t[32];
typedef const uint8_t App_Conf_key_t[64];
typedef const struct
{
    uint16_t length;
    uint8_t value_a[29];
} App_Conf_dev_name_t;

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_uuid_t * App_Conf_GetDeviceID(void);
 * ----------------------------------------------------------------------------
 * Description   : Returns the device ID
 * Inputs        : None
 * Outputs       : return value     - pointer to device ID
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_uuid_t * App_Conf_GetDeviceID(void);

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_version_t * App_Conf_GetVersion(
 *                                              App_Conf_version_type_t type)
 * ----------------------------------------------------------------------------
 * Description   : Returns the requested version information
 * Inputs        : type             - type of version
 * Outputs       : return value     - pointer to version info
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_version_t * App_Conf_GetVersion(App_Conf_version_type_t type);

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_build_id_t * App_Conf_GetBuildID(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the BLE stack build ID
 * Inputs        : None
 * Outputs       : return value     - pointer to build ID
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_build_id_t * App_Conf_GetBuildID(void);

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_key_t * App_Conf_GetPublicKey(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the public key for signature verification
 * Inputs        : None
 * Outputs       : return value     - pointer to public key
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_key_t * App_Conf_GetPublicKey(void);

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_uuid_t * App_Conf_GetServiceId(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the service ID used in the BLE advertisement
 * Inputs        : None
 * Outputs       : return value     - pointer to service ID
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_uuid_t * App_Conf_GetServiceId(void);

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_dev_name_t * App_Conf_GetDeviceName(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the device name used in the BLE advertisement
 * Inputs        : None
 * Outputs       : return value     - pointer to device name
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_dev_name_t * App_Conf_GetDeviceName(void);

#endif    /* _APP_CONF_H */
