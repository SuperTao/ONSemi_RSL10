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
 * app_conf.c
 * - DFU component configuration database.
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdlib.h>
#include <rsl10.h>
#include <rsl10_ke.h>

#include "app_conf.h"
#include "sys_fota.h"

/* ----------------------------------------------------------------------------
 * Application Version
 * ------------------------------------------------------------------------- */

typedef struct
{
    uint32_t length;
    App_Conf_key_t public_key;
    App_Conf_uuid_t uuid;
    App_Conf_dev_name_t dev_name;
} config_t;

typedef struct
{
    Sys_Fota_version_t version;
    config_t config;
} version_t;

__attribute__ ((section(".rodata.boot.version")))
const version_t Sys_Boot_app_version =
{
    .version =
    {
        { VER_ID, BOOT_VER_ENCODE(VER_MAJOR, VER_MINOR, VER_REVISION) }
    },
    .config =
    {
        .length     = sizeof(config_t),
        .uuid       = CFG_FOTA_SVC_UUID,
        .dev_name   = { sizeof(CFG_DEVICE_NAME) - 1, CFG_DEVICE_NAME }
    }
};

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_uuid_t * App_Conf_GetDeviceID(void);
 * ----------------------------------------------------------------------------
 * Description   : Returns the device ID
 * Inputs        : None
 * Outputs       : return value     - pointer to device ID
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_uuid_t * App_Conf_GetDeviceID(void)
{
    return &Sys_Boot_app_version.version.dev_id;
}

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_version_t * App_Conf_GetVersion(
 *                                              App_Conf_version_type_t type)
 * ----------------------------------------------------------------------------
 * Description   : Returns the requested version information
 * Inputs        : type             - type of version
 * Outputs       : return value     - pointer to version info
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_version_t * App_Conf_GetVersion(App_Conf_version_type_t type)
{
    switch (type)
    {
        case APP_CONF_BOOT_VERSION:
        {
            return Sys_Boot_GetVersion(BOOT_BASE_ADR);
        }

        case APP_CONF_STACK_VERSION:
        {
            return &Sys_Boot_app_version.version.app_id;
        }

        case APP_CONF_APP_VERSION:
        {
            return Sys_Boot_GetVersion(Sys_Boot_GetNextImage(APP_BASE_ADR));
        }
    }

    return NULL;
}

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_build_id_t * App_Conf_GetBuildID(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the BLE stack build ID
 * Inputs        : None
 * Outputs       : return value     - pointer to build ID
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_build_id_t * App_Conf_GetBuildID(void)
{
    const Sys_Boot_descriptor_t *dscr_p = Sys_Boot_GetDscr(APP_BASE_ADR);
    return (App_Conf_build_id_t *)(dscr_p->build_id_a);
}

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_key_t * App_Conf_GetPublicKey(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the public key for signature verification
 * Inputs        : None
 * Outputs       : return value     - pointer to public key
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_key_t * App_Conf_GetPublicKey(void)
{
    return &Sys_Boot_app_version.config.public_key;
}

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_uuid_t * App_Conf_GetServiceId(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the UUID used in the BLE advertisement
 * Inputs        : None
 * Outputs       : return value     - pointer to UUIDy
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_uuid_t * App_Conf_GetServiceId(void)
{
    return &Sys_Boot_app_version.config.uuid;
}

/* ----------------------------------------------------------------------------
 * Function      : App_Conf_dev_name_t * App_Conf_GetDeviceName(void)
 * ----------------------------------------------------------------------------
 * Description   : Returns the device name used in the BLE advertisement
 * Inputs        : None
 * Outputs       : return value     - pointer to device name
 * Assumptions   :
 * ------------------------------------------------------------------------- */
App_Conf_dev_name_t * App_Conf_GetDeviceName(void)
{
    return &Sys_Boot_app_version.config.dev_name;
}
