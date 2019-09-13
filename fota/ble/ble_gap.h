/* ----------------------------------------------------------------------------
 * Copyright (c) 2018 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * Copyright (C) RivieraWaves 2009-2018
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
 * ble_gap.h
 * - BLE GAP layer abstraction header
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */

#ifndef BLE_GAP_H
#define BLE_GAP_H

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
#include <rsl10.h>
#include <rsl10_ke.h>
#include <rsl10_map_nvr.h>
#include <gapm_task.h>
#include <gapc_task.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* GAPM default device configuration (GAPM_SET_DEV_CONFIG_CMD) */
#define GAPM_DEFAULT_RENEW_DUR                 15000
#define GAPM_DEFAULT_GAP_START_HDL             0
#define GAPM_DEFAULT_GATT_START_HDL            0
#define GAPM_DEFAULT_ATT_CFG                   GAPM_MASK_ATT_SVC_CHG_EN
#define GAPM_DEFAULT_TX_OCT_MAX                0x1b
#define GAPM_DEFAULT_TX_TIME_MAX               (14 * 8 + GAPM_DEFAULT_TX_OCT_MAX * 8)
#define GAPM_DEFAULT_MTU_MAX                   0x200
#define GAPM_DEFAULT_MPS_MAX                   0x200
#define GAPM_DEFAULT_MAX_NB_LECB               0
#define GAPM_DEFAULT_AUDIO_CFG                 0
#define GAPM_DEFAULT_ADV_DATA                  { 21, GAP_AD_TYPE_COMPLETE_NAME, 'O', 'N', \
                                                 ' ', 'S', 'e', 'm', 'i', 'c', 'o', 'n', 'd', 'u', 'c', 't', 'o', 'r', \
                                                 ' ', \
                                                 'B', 'L', 'E', 3, GAP_AD_TYPE_MANU_SPECIFIC_DATA, 0x62, 0x3 }
#define GAPM_DEFAULT_ADV_DATA_LEN              26
#define GAPM_DEFAULT_SCANRSP_DATA              { 3, GAP_AD_TYPE_MANU_SPECIFIC_DATA, 0x62, 0x3 }
#define GAPM_DEFAULT_SCANRSP_DATA_LEN          4

/* GAPM default StartConnectionCmd configuration (GAPM_START_CONNECTION_CMD) */
/* Defualt scan interval 62.5ms and scan window to 50% of the interval */
#define GAPM_DEFAULT_SCAN_INTERVAL             100
#define GAPM_DEFAULT_SCAN_WINDOW               50

/* Default connection interval = 20ms; slave latency = 0 */
#define GAPM_DEFAULT_CON_INTV_MIN              20
#define GAPM_DEFAULT_CON_INTV_MAX              20
#define GAPM_DEFAULT_CON_LATENCY               0

/* Default supervisory timeout = 3s */
#define GAPM_DEFAULT_SUPERV_TO                 300
#define GAPM_DEFAULT_CE_LEN_MIN                2 * GAPM_DEFAULT_CON_INTV_MIN
#define GAPM_DEFAULT_CE_LEN_MAX                2 * GAPM_DEFAULT_CON_INTV_MAX
#define GAPM_DEFAULT_NB_PEERS                  4

/* Advertisement default parameters */
/* Advertising min/max interval = 40ms (64*0.625ms) */
#define GAPM_DEFAULT_ADV_INTV_MIN              64
#define GAPM_DEFAULT_ADV_INTV_MAX              64

/* Advertising channel map - 37, 38, 39 */
#define GAPM_DEFAULT_ADV_CHMAP                 0x07

/* Check if a bondlist entry is valid */
#define VALID_BOND_INFO(state)          ((state > BOND_INFO_STATE_INVALID) && \
                                         (state <= SIZEOF_BONDLIST))
#ifndef CFG_BONDLIST_SIZE
#define CFG_BONDLIST_SIZE SIZEOF_BONDLIST
#endif /* ifndef CFG_BONDLIST_SIZE */

#define MS_TO_BLE_TICKS(ms) ((((uint32_t)ms) * 1000) / 625)

/* --------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    GAPM_STATE_INIT,               /* Initial state. Before the device is configured */
    GAPM_STATE_READY,              /* After device configuration is set */
    GAPM_STATE_ADVERTISING,        /* GAPM is advertising */
    GAPM_STATE_SCANNING,           /* GAPM is in scan mode */
    GAPM_STATE_STARTING_CONNECTION    /* GAPM is trying to initiate a connection (after a call to StartConnectionCmd) */
} GAPM_State_t;

typedef struct
{
    GAPM_State_t gapmState;
    uint16_t profileAddedCount;
    uint8_t connectionCount;
    struct gapm_set_dev_config_cmd deviceConfig;
    struct gapc_connection_req_ind connection[BLE_CONNECTION_MAX];

#if CFG_BOND_LIST_IN_NVR2
    BondInfo_Type bondInfo[BLE_CONNECTION_MAX];
#endif /* if CFG_BOND_LIST_IN_NVR2 */
} GAP_Env_t;

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/
void GAP_Initialize(void);

const GAP_Env_t * GAP_GetEnv(void);

bool GAP_IsAddrPrivateResolvable(const uint8_t *addr, uint8_t addrType);

/* GAPM Functions */
void GAPM_ResetCmd(void);

void GAPM_CancelCmd(void);

void GAPM_SetDevConfigCmd(const struct gapm_set_dev_config_cmd *deviceConfig);

void GAPM_StartConnectionCmd(const struct gapm_start_connection_cmd *startConnectionCmd);

void GAPM_StartAdvertiseCmd(const struct gapm_start_advertise_cmd *startAdvCmd);

void GAPM_ProfileTaskAddCmd(uint8_t sec_lvl, uint16_t prf_task_id, uint16_t app_task,
                            uint16_t start_hdl, uint8_t *param, uint32_t paramSize);

#if CFG_BOND_LIST_IN_NVR2
void GAPM_ResolvAddrCmd(uint8_t conidx, const uint8_t *peerAddr,
                        uint8_t irkListSize, const struct gap_sec_key *irkList);

#endif    /* CFG_BOND_LIST_IN_NVR2 */

uint16_t GAPM_GetProfileAddedCount(void);

const struct gapm_set_dev_config_cmd * GAPM_GetDeviceConfig(void);

bool GAPM_AddAdvData(enum gap_ad_type newDataFlag, uint8_t const *newData,
                     uint8_t newDataLen, uint8_t *resultAdvData,
                     uint8_t *resultAdvDataLen);

void GAPM_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                     ke_task_id_t const dest_id, ke_task_id_t const src_id);

/* GAPC Functions */
void GAPC_Initialize(void);

void GAPC_ParamUpdateCfm(uint8_t conidx, bool accept, uint16_t ce_len_min,
                         uint16_t ce_len_max);

void GAPC_ConnectionCfm(uint8_t conidx, struct gapc_connection_cfm const *param);

void GAPC_DisconnectCmd(uint8_t conidx, uint8_t reason);

void GAPC_DisconnectAll(uint8_t reason);

uint8_t GAPC_GetConnectionCount(void);

const struct gapc_connection_req_ind * GAPC_GetConnectionInfo(uint8_t conidx);

bool GAPC_IsConnectionActive(uint8_t conidx);

void GAPC_GetDevInfoCfm_Name(uint8_t conidx, const char *devName);

void GAPC_GetDevInfoCfm_Appearance(uint8_t conidx, uint16_t appearance);

void GAPC_GetDevInfoCfm_SlvPrefParams(uint8_t conidx, uint16_t con_intv_min,
                                      uint16_t con_intv_max, uint16_t slave_latency,
                                      uint16_t conn_timeout);

void GAPC_GetDevInfoCfm(uint8_t conidx, uint8_t req,
                        const union gapc_dev_info_val *dat);

void GAPC_SetDevInfoCfm(uint8_t conidx,
                        const struct gapc_set_dev_info_req_ind *param,
                        union gapc_dev_info_val *dat,
                        size_t name_max_length);

void GAPC_BondCfm(uint8_t conidx, enum gapc_bond request, bool accept,
                  const union gapc_bond_cfm_data *data);

void GAPC_EncryptCmd(uint8_t conidx, uint16_t ediv, const uint8_t *randnb,
                     const uint8_t *ltk, uint8_t key_size);

void GAPC_EncryptCfm(uint8_t conidx, bool found, const uint8_t *ltk);

void GAPC_BondCmd(uint8_t conidx, const struct gapc_pairing *pairing);

void GAPC_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                     ke_task_id_t const dest_id, ke_task_id_t const src_id);

#if CFG_BOND_LIST_IN_NVR2
bool GAPC_IsBonded(uint8_t conidx);

const BondInfo_Type * GAPC_GetBondInfo(uint8_t conidx);

/* Bluetooth bonding support functions */
bool BondList_Add(BondInfo_Type *bond_info);

const BondInfo_Type * BondList_FindByAddr(const uint8_t *addr,
                                          uint8_t addrType);

const BondInfo_Type * BondList_FindByIRK(const uint8_t *irk);

uint8_t BondList_GetIRKs(struct gap_sec_key *irkList);

bool BondList_Remove(uint8_t bondStateIndex);

bool BondList_RemoveAll(void);

uint8_t BondList_Size(void);

void NVR2_WriteEnable(bool enable);

#endif /* if CFG_BOND_LIST_IN_NVR2 */

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* BLE_GAP_H */
