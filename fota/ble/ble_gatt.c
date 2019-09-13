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
 * ble_gatt.c
 * - BLE GATT layer abstraction source
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */
#include "config.h"
#include <ble_gatt.h>
#include <gapm_task.h>
#include <gattc_task.h>
#include <app_trace.h>

/* GATT Environment Structure */
static GATT_Env_t gatt_env;

/* ----------------------------------------------------------------------------
 * Function      : void GATT_Initialize(void)
 * ----------------------------------------------------------------------------
 * Description   : Initialize the GATT environment
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATT_Initialize(void)
{
    memset(&gatt_env, 0, sizeof(GATT_Env_t));
}

/* ----------------------------------------------------------------------------
 * Function      : void GATT_GetEnv(void)
 * ----------------------------------------------------------------------------
 * Description   : Return a reference to the internal environment structure.
 * Inputs        : None
 * Outputs       : A constant pointer to GATT_Env_t
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
const GATT_Env_t * GATT_GetEnv(void)
{
    return &gatt_env;
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTM_GetEnv(void)
 * ----------------------------------------------------------------------------
 * Description   : Return a reference to the internal environment structure.
 * Inputs        : None
 * Outputs       : A constant pointer to GATTM_Env_t
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
uint16_t GATTM_GetServiceAddedCount(void)
{
    return gatt_env.addedSvcCount;
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTM_AddAttributeDatabase(
 *                      const struct att_db_desc *att_db, uint16_t att_db_len)
 * ----------------------------------------------------------------------------
 * Description   : Add multiple services/characteristic into the Bluetooth
 *                 stack database.
 * Inputs        : att_db     - Attribute database description (array)
 *                 att_db_len - Number of elements in the att_db array
 * Outputs       : None       -  Trigger a GATTM_ADD_SVC_RSP for every service
 *                               added.
 * Assumptions   : This function should be called only once, to construct the
 *                 whole custom services/attributes database.
 * ------------------------------------------------------------------------- */
void GATTM_AddAttributeDatabase(const struct att_db_desc *att_db,
                                uint16_t att_db_len)
{
    struct gattm_add_svc_req *req;
    uint8_t nb_att;
    uint8_t cs_nb_att = 0;
    uint8_t att_idx = 1;

    gatt_env.att_db = att_db;
    gatt_env.att_db_len = att_db_len;

    PRINTF("\r\nGATTM_AddAttributeDatabase: ");

    while (att_idx < att_db_len)
    {
        /* Count the number of attributes in the current service */
        for (nb_att = 0; att_idx < att_db_len && !att_db[att_idx].is_service; nb_att++, att_idx++);

        PRINTF(" nb_att = %d \r\n", nb_att);

        req = KE_MSG_ALLOC_DYN(GATTM_ADD_SVC_REQ,
                               TASK_GATTM, TASK_APP, gattm_add_svc_req,
                               nb_att * sizeof(struct gattm_att_desc));

        /* Fill the GATTM_ADD_SVC_REQ message to add a new custom service */
        req->svc_desc.start_hdl = 0;
        req->svc_desc.task_id = TASK_APP;
        req->svc_desc.perm = att_db[cs_nb_att].att.perm;
        req->svc_desc.nb_att = nb_att;
        memcpy(&req->svc_desc.uuid[0], &att_db[cs_nb_att++].att.uuid, ATT_UUID_128_LEN);

        for (uint8_t i = 0; i < nb_att; i++)
        {
            memcpy(&req->svc_desc.atts[i], &att_db[cs_nb_att++].att,
                   sizeof(struct gattm_att_desc));
        }

        ke_msg_send(req);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTM_GetHandle(uint16_t attidx)
 * ----------------------------------------------------------------------------
 * Description   : Return the stack database handle for a certain attribute
 *                 index.
 * Inputs        : attidx  - Attribute index
 * Outputs       : Handle value, in case of success or 0, in case the attidx or
 *                 the start handle is invalid.
 * Assumptions   : The user has used GATT_AddAttributeDatabase() to construct
 *                 the database and the stack has already finished adding
 *                 services to the databse.
 * ------------------------------------------------------------------------- */
uint16_t GATTM_GetHandle(uint16_t attidx)
{
    if (gatt_env.start_hdl && attidx < gatt_env.att_db_len)
    {
        return gatt_env.start_hdl + attidx;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTM_MsgHandler(ke_msg_id_t const msg_id,
 *                                     void const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle GATTM messages
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter (unused)
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTM_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (msg_id)
    {
        case GATTM_ADD_SVC_RSP:
        {
            const struct gattm_add_svc_rsp *p = param;
            if (p->status == ATT_ERR_NO_ERROR)
            {
                gatt_env.addedSvcCount++;
                if (gatt_env.start_hdl == 0)
                {
                    gatt_env.start_hdl = p->start_hdl;
                }
            }
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_DiscByUUIDSvc(uint8_t conidx, uint8_t uuid[],
 *                      uint8_t uuid_len, uint16_t start_hdl, uint16_t end_hdl)
 * ----------------------------------------------------------------------------
 * Description   : Start a service discovery by UUID (GATTC_DISC_BY_UUID_SVC)
 *                 in a certain handle range.
 * Inputs        : conidx      - Connection index
 *                 uuid        - Service UUID
 *                 uuid_len    - UUID length (ATT_UUID_16_LEN,
 *                                            ATT_UUID_32_LEN,
 *                                            ATT_UUID_128_LEN)
 *                 start_hdl   - Start handle of discovery range
 *                 end_hdl     - End handle of discovery range
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_DiscByUUIDSvc(uint8_t conidx, uint8_t uuid[], uint8_t uuid_len,
                         uint16_t start_hdl, uint16_t end_hdl)
{
    struct gattc_disc_cmd *cmd;

    gatt_env.discSvcCount[conidx] = 0;

    cmd = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
                           TASK_APP, gattc_disc_cmd, uuid_len * sizeof(uint8_t));

    cmd->operation = GATTC_DISC_BY_UUID_SVC;
    cmd->seq_num   = 0;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl   = end_hdl;
    cmd->uuid_len  = uuid_len;
    memcpy(cmd->uuid, uuid, uuid_len);

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_DiscAllSvc(uint8_t conidx, uint16_t start_hdl,
 *                                       uint16_t end_hdl)
 * ----------------------------------------------------------------------------
 * Description   : Start a service discovery for all services in a certain
 *                 handle range.
 * Inputs        : conidx      - Connection index
 *                 start_hdl   - Start handle of discovery range
 *                 end_hdl     - End handle of discovery range
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_DiscAllSvc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    struct gattc_disc_cmd *cmd;

    gatt_env.discSvcCount[conidx] = 0;

    cmd = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
                           TASK_APP, gattc_disc_cmd, 2 * sizeof(uint8_t));

    cmd->operation = GATTC_DISC_ALL_SVC;
    cmd->seq_num   = 0;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl   = end_hdl;
    cmd->uuid_len  = 2;
    cmd->uuid[0]   = 0;
    cmd->uuid[1]   = 0;

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_DiscAllChar(uint8_t conidx, uint16_t start_hdl,
 *                                        uint16_t end_hdl)
 * ----------------------------------------------------------------------------
 * Description   : Start a characteristic discovery for all characteristics in
 *                 a certain handle range.
 * Inputs        : conidx      - Connection index
 *                 start_hdl   - Start handle of discovery range
 *                 end_hdl     - End handle of discovery range
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_DiscAllChar(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    struct gattc_disc_cmd *cmd;

    cmd = KE_MSG_ALLOC_DYN(GATTC_DISC_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
                           TASK_APP, gattc_disc_cmd, 2 * sizeof(uint8_t));

    cmd->operation = GATTC_DISC_ALL_CHAR;
    cmd->seq_num   = 0;
    cmd->start_hdl = start_hdl;
    cmd->end_hdl   = end_hdl;
    cmd->uuid_len  = 2;
    cmd->uuid[0]   = 0;
    cmd->uuid[1]   = 0;

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_SendEvtCmd(uint8_t conidx, uint8_t operation,
 *                                       uint16_t seq_num, uint16_t handle,
 *                                       uint16_t length, uint8_t const *value)
 * ----------------------------------------------------------------------------
 * Description   : Send GATTC characteristic notification or indication
 * Inputs        : conidx      - Connection index
 *                 operation   - GATTC_NOTIFY or GATTC_INDICATE
 *                 seq_num     - Operation sequence number
 *                 handle      - Characteristic handle
 *                 length      - Length of attribute
 *                 value       - Attribute data value
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_SendEvtCmd(uint8_t conidx, uint8_t operation, uint16_t seq_num,
                      uint16_t handle, uint16_t length, uint8_t const *value)
{
    struct gattc_send_evt_cmd *cmd;

    cmd = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD, KE_BUILD_ID(TASK_GATTC, conidx),
                           TASK_APP, gattc_send_evt_cmd,
                           length * sizeof(uint8_t));
    cmd->handle = handle;
    cmd->length = length;
    cmd->operation = operation;
    cmd->seq_num = seq_num;
    memcpy(cmd->value, value, length);

    ke_msg_send(cmd);
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_MsgHandler(ke_msg_id_t const msg_id,
 *                                     void const *param,
 *                                     ke_task_id_t const dest_id,
 *                                     ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle any GATTC message received from kernel
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameter (unused)
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_MsgHandler(ke_msg_id_t const msg_id, void const *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);

    switch (msg_id)
    {
        case GATTC_DISC_SVC_IND:
        {
            gatt_env.discSvcCount[conidx]++;
        }
        break;

        case GATTC_READ_REQ_IND:
        {
            if (gatt_env.att_db)
            {
                GATTC_ReadReqInd(msg_id, param, dest_id, src_id);
            }
        }
        break;

        case GATTC_WRITE_REQ_IND:
        {
            if (gatt_env.att_db)
            {
                GATTC_WriteReqInd(msg_id, param, dest_id, src_id);
            }
        }
        break;

        case GATTC_ATT_INFO_REQ_IND:
        {
            if (gatt_env.att_db)
            {
                GATTC_AttInfoReqInd(msg_id, param, dest_id, src_id);
            }
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_ReadReqInd(ke_msg_id_t const msg_id,
 *                                       struct gattc_read_req_ind
 *                                       const *param,
 *                                       ke_task_id_t const dest_id,
 *                                       ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle received read request indication
 *                 from a GATT Controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gattc_read_req_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_ReadReqInd(ke_msg_id_t const msg_id,
                      struct gattc_read_req_ind const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id)
{
    /* Retrieve the index of environment structure representing peer device */
    signed int conidx = KE_IDX_GET(src_id);
    struct gattc_read_cfm *cfm;
    uint8_t length = 0;
    uint8_t status = GAP_ERR_NO_ERROR;
    uint16_t attnum;

    attnum = (param->handle - gatt_env.start_hdl);
    PRINTF("GATTC_ReadReqInd\r\n");

    if (param->handle <= gatt_env.start_hdl)
    {
        status = ATT_ERR_INVALID_HANDLE;
    }
    else if ((attnum >= gatt_env.att_db_len) ||
             !(gatt_env.att_db[attnum].att.perm & (PERM(RD, ENABLE) | PERM(NTF, ENABLE))))
    {
        status = ATT_ERR_READ_NOT_PERMITTED;
    }
    else
    {
        length = gatt_env.att_db[attnum].length;
    }

    /* Allocate and build message */
    cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM,
                           KE_BUILD_ID(TASK_GATTC, conidx),
                           TASK_APP, gattc_read_cfm, length);

    /* If there is no error, copy the requested attribute value, using the
     * callback function */
    if (status == GAP_ERR_NO_ERROR)
    {
        if (gatt_env.att_db[attnum].callback != NULL)
        {
            status = gatt_env.att_db[attnum].callback(conidx, attnum, param->handle, cfm->value,
                                                      gatt_env.att_db[attnum].data, length, GATTC_READ_REQ_IND);
        }
        else    /* No callback function has been set for this attribute, just do a memcpy */
        {
            memcpy(cfm->value, gatt_env.att_db[attnum].data, length);
        }
    }

    cfm->handle = param->handle;
    cfm->length = length;
    cfm->status = status;

    /* Send the message */
    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void GATTC_WriteReqInd(ke_msg_id_t const msg_id,
 *                                        struct gattc_write_req_ind
 *                                        const *param,
 *                                        ke_task_id_t const dest_id,
 *                                        ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handle received write request indication
 *                 from a GATT Controller
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gattc_write_req_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_WriteReqInd(ke_msg_id_t const msg_id,
                       struct gattc_write_req_ind const *param,
                       ke_task_id_t const dest_id,
                       ke_task_id_t const src_id)
{
    /* Retrieve peer device index */
    signed int conidx = KE_IDX_GET(src_id);
    struct gattc_write_cfm *cfm =
        KE_MSG_ALLOC(GATTC_WRITE_CFM,
                     KE_BUILD_ID(TASK_GATTC, conidx),
                     TASK_APP, gattc_write_cfm);
    uint8_t status = GAP_ERR_NO_ERROR;
    uint16_t attnum;

    /* Verify the correctness of the write request. Set the attribute index if
     * the request is valid */
    attnum = (param->handle - gatt_env.start_hdl);
    if (param->offset)
    {
        status = ATT_ERR_INVALID_OFFSET;
    }
    else if (param->handle <= gatt_env.start_hdl)
    {
        status = ATT_ERR_INVALID_HANDLE;
    }
    else if ((attnum >= gatt_env.att_db_len) ||
             !(gatt_env.att_db[attnum].att.perm & (PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE))))
    {
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }

    PRINTF("GATTC_WriteReqInd\r\n");
    /* If there is no error, copy the requested attribute value, using the
     * callback function */
    if (status == GAP_ERR_NO_ERROR)
    {
        if (gatt_env.att_db[attnum].callback != NULL)
        {
            status = gatt_env.att_db[attnum].callback(conidx,
                                                      attnum,
                                                      param->handle,
                                                      gatt_env.att_db[attnum].data,
                                                      param->value,
                                                      MIN(param->length, gatt_env.att_db[attnum].length),
                                                      GATTC_WRITE_REQ_IND);
        }
        else    /* No callback function has been set for this attribute, just do a memcpy */
        {
            memcpy(gatt_env.att_db[attnum].data, param->value, MIN(param->length, gatt_env.att_db[attnum].length));
        }
    }
    cfm->handle = param->handle;
    cfm->status = status;

    /* Send the message */
    ke_msg_send(cfm);
}

/* ----------------------------------------------------------------------------
 * Function      : void CUSTOMSS_AttInfoReqInd(
 *                                  ke_msg_id_t const msg_id,
 *                                  struct gattc_read_req_ind const *param,
 *                                  ke_task_id_t const dest_id,
 *                                  ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Request Attribute info to upper layer
 *                 could be trigger during prepare write to check if attribute
 *                 modification is authorized by profile/application or not and
 *                 to get current attribute length
 * Inputs        : - msg_id     - Kernel message ID number
 *                 - param      - Message parameters in format of
 *                                struct gattc_read_req_ind
 *                 - dest_id    - Destination task ID number
 *                 - src_id     - Source task ID number
 * Outputs       : return value - Indicate if the message was consumed;
 *                                compare with KE_MSG_CONSUMED
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void GATTC_AttInfoReqInd(ke_msg_id_t const msg_id,
                         struct gattc_read_req_ind const *param,
                         ke_task_id_t const dest_id,
                         ke_task_id_t const src_id)
{
    /* Retrieve peer device index */
    signed int conidx = KE_IDX_GET(src_id);
    uint8_t status = GAP_ERR_NO_ERROR;
    uint8_t length = 0;
    uint16_t attnum;
    attnum = (param->handle - gatt_env.start_hdl);

    PRINTF("GATTC_AttInfoReqInd, %d, %d\r\n", attnum, gatt_env.att_db[attnum].att_idx);

    if (param->handle <= gatt_env.start_hdl)
    {
        status = ATT_ERR_INVALID_OFFSET;
    }
    else if ((attnum >= gatt_env.att_db_len) ||
             !(gatt_env.att_db[attnum].att.perm & PERM(WRITE_REQ, ENABLE)))

    {
        status = ATT_ERR_WRITE_NOT_PERMITTED;
    }
    else
    {
        length = gatt_env.att_db[attnum].length;
    }

    /* Attribute Information confirmation message to inform if peer
     * device is authorized to modify attribute value, and to get current
     * attribute length.
     */
    struct gattc_att_info_cfm *cfm =
        KE_MSG_ALLOC(GATTC_ATT_INFO_CFM,
                     KE_BUILD_ID(TASK_GATTC, conidx),
                     TASK_APP, gattc_att_info_cfm);

    cfm->handle = param->handle;
    cfm->length = length;
    cfm->status = status;

    /* Send the message */
    ke_msg_send(cfm);
}
