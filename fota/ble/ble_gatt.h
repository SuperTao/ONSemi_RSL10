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
 * ble_gatt.h
 * - BLE GATT layer abstraction header
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */

#ifndef BLE_GATTM_H
#define BLE_GATTM_H

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
#include <rsl10_ke.h>
#include <gattm_task.h>
#include <gattc_task.h>

/* ----------------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------------- */
#define GATTC_DEFAULT_START_HDL         0x0001
#define GATTC_DEFAULT_END_HDL           0xFFFF

/* Macros to declare a (custom) service with 16, 32 and 128 bit UUID
 *   - srvidx: Service index
 *   - uuid: Service UUID */
#define CS_SERVICE_UUID_16(srvidx, uuid) \
    { srvidx, { uuid, PERM(SVC_UUID_LEN, UUID_16), 0, 0 }, true, 0, NULL, NULL }
#define CS_SERVICE_UUID_32(srvidx, uuid) \
    { srvidx, { uuid, PERM(SVC_UUID_LEN, UUID_32), 0, 0 }, true, 0, NULL, NULL }
#define CS_SERVICE_UUID_128(srvidx, uuid) \
    { srvidx, { uuid, PERM(SVC_UUID_LEN, UUID_128), 0, 0 }, true, 0, NULL, NULL }

/* Standard declaration/description UUIDs in 16-byte format */
#define CS_ATT_SERVICE_128            { 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }
#define CS_ATT_CHARACTERISTIC_128     { 0x03, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }
#define CS_ATT_CLIENT_CHAR_CFG_128    { 0x02, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }
#define CS_ATT_CHAR_USER_DESC_128     { 0x01, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }

/* Macros to define characteristics with 16, 32 and 128 bit UUID
 *   - attidx_char: Characteristic attribute index
 *   - attidx_val: Value attribute index
 *   - uuid: UUID
 *   - perm: Permissions (see gattm_att_desc)
 *   - length: Value max length (in bytes)
 *   - data: Pointer to the data structure in the application
 *   - callback: Function to transfer the data between the application and the GATTM */
#define CS_CHAR_UUID_16(attidx_char, attidx_val, uuid, perm, length, data, callback) \
    { attidx_char, { CS_ATT_CHARACTERISTIC_128, PERM(RD, ENABLE), 0, 0 }, false, 0, NULL, NULL }, \
    { attidx_val, { uuid, perm, length, PERM(RI, ENABLE) | PERM(UUID_LEN, UUID_16) }, false, length, data, callback }
#define CS_CHAR_UUID_32(attidx_char, attidx_val, uuid, perm, length, data, callback) \
    { attidx_char, { CS_ATT_CHARACTERISTIC_128, PERM(RD, ENABLE), 0, 0 }, false, 0, NULL, NULL }, \
    { attidx_val, { uuid, perm, length, PERM(RI, ENABLE) | PERM(UUID_LEN, UUID_32) }, false, length, data, callback }
#define CS_CHAR_UUID_128(attidx_char, attidx_val, uuid, perm, length, data, callback) \
    { attidx_char, { CS_ATT_CHARACTERISTIC_128, PERM(RD, ENABLE), 0, 0 }, false, 0, NULL, NULL }, \
    { attidx_val, { uuid, perm, length, PERM(RI, ENABLE) | PERM(UUID_LEN, UUID_128) }, false, length, data, callback }

/* Macro to add to the characteristic a CCC
 *   - attidx: CCC attribute index
 *   - data: Pointer to the 2-byte CCC data value in the application
 *   - callback: Function to transfer the CCC data between the application and the GATTM */
#define CS_CHAR_CCC(attidx, data, callback) \
    { attidx, { CS_ATT_CLIENT_CHAR_CFG_128, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), 0, PERM(RI, ENABLE) }, \
      false, 2, data, callback }

/* Macro to add to the characteristic a user description
 *   - attidx: Description attribute index
 *   - data: Pointer to the description string (constant) */
#define CS_CHAR_USER_DESC(attidx, length, data, callback) \
    {attidx, {CS_ATT_CHAR_USER_DESC_128, PERM(RD, ENABLE), length, PERM(RI, ENABLE) }, false, length, data, callback }
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* custom service attribute definitions */
struct att_db_desc
{
    uint16_t att_idx;
    struct gattm_att_desc att;
    bool is_service;
    uint16_t length;
    void *data;    /* convert into uint8_t */
    uint8_t (*callback)(uint8_t conidx, uint16_t attidx, uint16_t handle,
                        uint8_t *toData, const uint8_t *fromData, uint16_t lenData,
                        uint16_t operation);
};

typedef struct
{
    uint16_t addedSvcCount;
    uint16_t discSvcCount[BLE_CONNECTION_MAX];

    uint16_t start_hdl;    /* Start handle of custom services in the stack's attribute database */
    const struct att_db_desc *att_db;
    uint16_t att_db_len;
} GATT_Env_t;

void GATT_Initialize(void);

const GATT_Env_t * GATT_GetEnv(void);

/* ----------------------------------------------------------------------------
 * GATTM Functions
 * --------------------------------------------------------------------------*/
uint16_t GATTM_GetServiceAddedCount(void);

void GATTM_AddAttributeDatabase(const struct att_db_desc *att_db, uint16_t att_db_len);

uint16_t GATTM_GetHandle(uint16_t attidx);

void GATTM_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * GATTC Functions
 * --------------------------------------------------------------------------*/
void GATTC_Initialize(void);

void GATTC_DiscByUUIDSvc(uint8_t conidx, uint8_t uuid[], uint8_t uuid_len,
                         uint16_t start_hdl, uint16_t end_hdl);

void GATTC_DiscAllSvc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

void GATTC_DiscAllChar(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

void GATTC_SendEvtCmd(uint8_t conidx, uint8_t operation, uint16_t seq_num,
                      uint16_t handle, uint16_t length, uint8_t const *value);

void GATTC_ReadReqInd(ke_msg_id_t const msg_id,
                      struct gattc_read_req_ind const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id);

void GATTC_WriteReqInd(ke_msg_id_t const msg_id,
                       struct gattc_write_req_ind const *param,
                       ke_task_id_t const dest_id,
                       ke_task_id_t const src_id);

void GATTC_AttInfoReqInd(ke_msg_id_t const msg_id,
                         struct gattc_read_req_ind const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id);

void GATTC_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* GATT_H */
