/* ----------------------------------------------------------------------------
 * Copyright (c) 2015-2017 Semiconductor Components Industries, LLC (d/b/a ON
 * Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * rsl10_protocol.c
 * - Provide Bluetooth protocol related parameters
 * ----------------------------------------------------------------------------
 * $Revision: 1.4 $
 * $Date: 2019/05/01 15:29:24 $
 * ------------------------------------------------------------------------- */

#include <rsl10.h>
#include <rsl10_protocol.h>

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

static const uint8_t all_ff_bytes[32] = {  \
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
static const uint8_t all_00_bytes[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/
ble_deviceParam_t ble_deviceParam = { 0, 0, 20, 3, 0, 0 };

/* ----------------------------------------------------------------------------
 * Function prototypes
 * ------------------------------------------------------------------------- */

extern void __wrap_Device_Param_Prepare(app_device_param_t *param);

/* ----------------------------------------------------------------------------
 * Function      : Device_Param_Read(uint8_t requestedId, uint8_t *buf)
 * ----------------------------------------------------------------------------
 * Description   : Read Bluetooth low energy parameters, security keys, and
 *                 channel assessment parameters that are provided by the
 *                 application or NVR3
 * Inputs        : - requestedId    - Parameter identifier
 *                 - buf            - Pointer to the returned value
 * Outputs       : - Return value   - Indicate if requested parameter exists
 *                                    in flash memory
 * Assumptions   : Application has declared Device_Param_Prepare function
 * ------------------------------------------------------------------------- */
uint8_t Device_Param_Read(uint8_t requestedId, uint8_t *buf)
{
    uint8_t valueExist = 1;

    uint8_t *ptr;
    uint8_t *ptr2;

    app_device_param_t param;

    param.device_param_src_type = FLASH_PROVIDED_or_DFLT;
    param.chnlAsses_param_src_type = FLASH_PROVIDED_or_DFLT;

    __wrap_Device_Param_Prepare(&param);

    switch (requestedId)
    {
        case PARAM_ID_PUBLIC_BLE_ADDRESS:
        {
            if (param.device_param_src_type == APP_PROVIDED)
            {
                memcpy(buf, param.bleAddress, sizeof(param.bleAddress));
            }
            else
            {
                uint8_t *ptr = (uint8_t *)DEVICE_INFO_BLUETOOTH_ADDR;
                if (memcmp(all_ff_bytes, ptr, sizeof(param.bleAddress)) == 0 ||
                    memcmp(all_00_bytes, ptr, sizeof(param.bleAddress)) == 0)
                {
                    /* If Bluetooth address is not available */
                    valueExist = 0;
                }
                else
                {
                    memcpy(buf, ptr, sizeof(param.bleAddress));
                }
            }
        }
        break;

        case PARAM_ID_IRK:
        {
            if (param.device_param_src_type == APP_PROVIDED)
            {
                memcpy(buf, param.irk, sizeof(param.irk));
            }
            else
            {
                uint8_t *ptr = (uint8_t *)DEVICE_INFO_BLUETOOTH_IRK;
                if (memcmp(all_ff_bytes, ptr, sizeof(param.irk)) == 0)
                {
                    /* If IRK is not available */
                    valueExist = 0;
                }
                else
                {
                    memcpy(buf, ptr, sizeof(param.irk));
                }
            }
        }
        break;

        case PARAM_ID_CSRK:
        {
            if (param.device_param_src_type == APP_PROVIDED)
            {
                memcpy(buf, param.csrk, sizeof(param.csrk));
            }
            else
            {
                uint8_t *ptr = (uint8_t *)DEVICE_INFO_BLUETOOTH_CSRK;
                if (memcmp(all_ff_bytes, ptr, sizeof(param.csrk)) == 0)
                {
                    /* If CSRK is not available */
                    valueExist = 0;
                }
                else
                {
                    memcpy(buf, ptr, sizeof(param.csrk));
                }
            }
        }
        break;

        case PARAM_ID_PRIVATE_KEY:
        {
            if (param.device_param_src_type == APP_PROVIDED)
            {
                memcpy(buf, param.privateKey, sizeof(param.privateKey));
            }
            else
            {
                uint8_t *ptr = (uint8_t *)DEVICE_INFO_ECDH_PRIVATE;
                if (memcmp(all_ff_bytes, ptr, sizeof(param.privateKey)) == 0)
                {
                    /* If Private key is not available */
                    valueExist = 0;
                }
                else
                {
                    memcpy(buf, ptr, sizeof(param.privateKey));
                }
            }
        }
        break;

        case PARAM_ID_PUBLIC_KEY:
        {
            if (param.device_param_src_type == APP_PROVIDED)
            {
                memcpy(buf +  0, param.publicKey_x, sizeof(param.publicKey_x));
                memcpy(buf + 32, param.publicKey_y, sizeof(param.publicKey_y));
            }
            else
            {
                ptr  = (uint8_t *)DEVICE_INFO_ECDH_PUBLIC_X;
                ptr2 = (uint8_t *)DEVICE_INFO_ECDH_PUBLIC_Y;
                if (memcmp(all_ff_bytes, ptr,  sizeof(param.publicKey_x)) == 0 ||
                    memcmp(all_ff_bytes, ptr2, sizeof(param.publicKey_y)) == 0)
                {
                    /* If public key is not available */
                    valueExist = 0;
                }
                else
                {
                    memcpy(buf +  0, ptr,  sizeof(param.publicKey_x));
                    memcpy(buf + 32, ptr2, sizeof(param.publicKey_y));
                }
            }
        }
        break;

        case PARAM_ID_BLE_CA_TIMER_DUR:
        {
            if (param.chnlAsses_param_src_type == APP_PROVIDED)
            {
                memcpy(buf, &param.chnlAsses_timer_duration,
                       sizeof(param.chnlAsses_timer_duration));
            }
            else
            {
                valueExist = 0;
            }
        }
        break;

        case PARAM_ID_BLE_CRA_TIMER_CNT:
        {
            if (param.chnlAsses_param_src_type == APP_PROVIDED)
            {
                *buf = param.chnlAsses_timer_cnt;
            }
            else
            {
                valueExist = 0;
            }
        }
        break;

        case PARAM_ID_BLE_CA_MIN_THR:
        {
            if (param.chnlAsses_param_src_type == APP_PROVIDED)
            {
                *buf = param.chnlAsses_min_thr;
            }
            else
            {
                valueExist = 0;
            }
        }
        break;

        case PARAM_ID_BLE_CA_MAX_THR:
        {
            if (param.chnlAsses_param_src_type == APP_PROVIDED)
            {
                *buf = param.chnlAsses_max_thr;
            }
            else
            {
                valueExist = 0;
            }
        }
        break;

        case PARAM_ID_BLE_CA_NOISE_THR:
        {
            if (param.chnlAsses_param_src_type == APP_PROVIDED)
            {
                *buf = param.chnlAsses_noise_thr;
            }
            else
            {
                valueExist = 0;
            }
        }
        break;

        default:
        {
            valueExist = 0;
        }
        break;
    }

    return valueExist;
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_ADV_IFS(uint32_t adv_ifs)
 * ----------------------------------------------------------------------------
 * Description   : Definition of the function to set advertisement inter-frame
 *                 space
 * Inputs        : - adv_ifs    - A inter-frame space in us
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_ADV_IFS(uint32_t adv_ifs)
{
    ble_deviceParam.adv_ifs = adv_ifs;
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_ClockAccuracy(uint16_t clockAccuracy)
 * ----------------------------------------------------------------------------
 * Description   : Definition of the function to set clock accuracy according to
 *                 XTAL 48 MHz or low power clock accuracy for sleep applications
 * Inputs        : - clockAccuracy    - Clock accuracy in ppm
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_ClockAccuracy(uint16_t clockAccuracy)
{
    if (clockAccuracy > 0 && clockAccuracy < 501)
    {
        ble_deviceParam.clockAccuracy = clockAccuracy;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_ForcedClockAccuracy(uint32_t forcedClockAccuracy)
 * ----------------------------------------------------------------------------
 * Description   : Definition of the function to set The sum of clock accuracy of
 *                   master and slave devices
 * Inputs        : - forcedClockAccuracy    - The sum of clock accuracy of
 *                                            devices in ppm
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_ForcedClockAccuracy(uint32_t forcedClockAccuracy)
{
    if (forcedClockAccuracy > 0 && forcedClockAccuracy < 10001)
    {
        ble_deviceParam.forcedClockAccuracy =  forcedClockAccuracy;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_MaxNumRAL(uint8_t maxNumRAL)
 * ----------------------------------------------------------------------------
 * Description   : The size of resolving address list
 * Inputs        : - maxNumRAL    - Maximum number of devices that can be
 *                                  set for RAL, by default is 3, for base-band
 *                                  clock equal or bigger than 16 MHz can be set
 *                                  up to 6
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_MaxNumRAL(uint8_t maxNumRAL)
{
    if ((maxNumRAL > 3) && (maxNumRAL < 7) &&
        ((BBIF->CTRL & BBIF_CTRL_CLK_SEL_Mask) != BBCLK_DIVIDER_8))
    {
        ble_deviceParam.maxNumRAL = maxNumRAL;
    }
    else
    {
        ble_deviceParam.maxNumRAL = 3;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_DLE(uint8_t maxRxOctet, uint16_t maxRxTime)
 * ----------------------------------------------------------------------------
 * Description   : Setting of default data length parameters
 * Inputs        : - maxRxOctet    - Supported maximum number of bytes for RX
 *               : - maxRxTime     - Supported maximum time in microsecond for RX
 * Outputs       : return value    - Returns zero if parameters are set successfully
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint8_t BLE_DeviceParam_Set_MaxRxOctet(uint8_t maxRxOctet, uint16_t maxRxTime)
{
    uint8_t error = 0;

    if ((maxRxOctet < 27) || (maxRxOctet > 251) ||
        (maxRxTime < 328) || (maxRxTime > 2120))
    {
        ble_deviceParam.max_rx_octets = 0;
        ble_deviceParam.max_rx_time = 0;
        error = 0xff;
    }
    else
    {
        ble_deviceParam.max_rx_octets = maxRxOctet;
        ble_deviceParam.max_rx_time = maxRxTime;
    }

    return error;
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_AdvDelay(uint8_t fixedDelayEnable)
 * ----------------------------------------------------------------------------
 * Description   : Enables a fixed value for advertisement intervals by
 *                 setting advDelay to zero. If enabled, this feature will
 *                 violate the Bluetooth Low Energy specification
 * Inputs        : - fixedDelayEnable    - Set to non-zero to enable a zero
 * 										   random AdvDelay value
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_AdvDelay(uint8_t fixedDelayEnable)
{
    if(fixedDelayEnable)
    {
       ble_deviceParam.fixedAdvIntervalDelayEnable = 1;
    }
    else
    {
        ble_deviceParam.fixedAdvIntervalDelayEnable = 0;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : BLE_DeviceParam_Set_SlaveLatencyDelay(uint8_t latencyDelay)
 * ----------------------------------------------------------------------------
 * Description   : Sets a delay to the instant that slave latency is applies
 *                 Slave latency is delayed by the number of interval equals to
 *                 the argument of latencyDelay
 * Inputs        : - fixedDelayEnable    - The desired number of interval that
 *                                         slave latency is delayed
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void BLE_DeviceParam_Set_SlaveLatencyDelay(uint8_t latencyDelay)
{
    ble_deviceParam.slaveLatencyDelay = latencyDelay;

}
