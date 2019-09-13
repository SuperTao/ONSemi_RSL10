/* ----------------------------------------------------------------------------
 * Copyright (c) 2018 Semiconductor Components Industries, LLC (d/b/a
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
 * msg_handler.c
 * - Message Handler source file
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */
#include "config.h"
#include <rsl10.h>
#include <msg_handler.h>
#include <ble_gap.h>
#include <ble_gatt.h>

static MsgHandler_t *msgHandlerHead = NULL;

/* ----------------------------------------------------------------------------
 * Function      : bool MsgHandler_Add(ke_msg_id_t const msg_id,
 *                               void (*callback)(ke_msg_id_t const msg_id,
 *                               void const *param, ke_task_id_t const dest_id,
 *                               ke_task_id_t const src_id))
 * ----------------------------------------------------------------------------
 * Description   : Add a message handler to the linked list of handlers.
 * Inputs        : msg_id   - A task identifier, such as TASK_ID_GAPM
 *                            or a message identifier, such as GAPM_CMP_EVT;
 *                 callback - A callback function associated to the msg_id
 * Outputs       : true if successful, false otherwise
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool MsgHandler_Add(ke_msg_id_t const msg_id,
                    void (*callback)(ke_msg_id_t const msg_id, void const *param,
                                     ke_task_id_t const dest_id, ke_task_id_t const src_id))
{
    MsgHandler_t *newElem = malloc(sizeof(MsgHandler_t));

    if (!newElem)    /* Malloc error */
    {
        return false;
    }

    newElem->msg_id = msg_id;
    newElem->callback = callback;
    newElem->next = NULL;

    if (!msgHandlerHead)    /* If list is empty, newElem will be the new head */
    {
        msgHandlerHead = newElem;
    }
    else
    {
        MsgHandler_t *tmp = msgHandlerHead;
        MsgHandler_Remove(msg_id, callback);    /* Avoid handler duplication, in case it already exists */

        while (tmp->next)    /* Go to the end of the list */
        {
            tmp = tmp->next;
        }
        tmp->next = newElem;    /* Insert newElem at the end of the list */
    }

    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : bool MsgHandler_Remove(ke_msg_id_t const msg_id,
 *                               void (*callback)(ke_msg_id_t const msg_id,
 *                               void const *param, ke_task_id_t const dest_id,
 *                               ke_task_id_t const src_id))
 * ----------------------------------------------------------------------------
 * Description   : Remove message handler from list
 * Inputs        : msg_id   - A task identifier, such as TASK_ID_GAPM
 *                            or a message identifier, such as GAPM_CMP_EVT;
 *                 callback - A callback function associated to the msg_id
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
bool MsgHandler_Remove(ke_msg_id_t const msg_id,
                       void (*callback)(ke_msg_id_t const msg_id, void const *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id))
{
    MsgHandler_t *tmp, *prev;
    bool removed = false;

    if (!msgHandlerHead)
    {
        return false;
    }

    tmp = msgHandlerHead;

    /* If the element to be removed is the head of the list */
    if ((msgHandlerHead->msg_id == msg_id) && (msgHandlerHead->callback == callback))
    {
        msgHandlerHead = msgHandlerHead->next;
        free(tmp);
        removed = true;
    }
    else    /* Search the remainder of the list */
    {
        do
        {
            prev = tmp;    /* Keep track of previous element */
            tmp = tmp->next;
        }
        while (tmp && ((tmp->msg_id != msg_id) || (tmp->callback != callback)));

        if (tmp)    /* if element found */
        {
            prev->next = tmp->next;    /* Remove element from list */
            free(tmp);
            removed = true;
        }
    }

    return removed;
}

/* ----------------------------------------------------------------------------
 * Function      : int MsgHandler_Notify(ke_msg_id_t const msg_id,
 *                               void const *param, ke_task_id_t const dest_id,
 *                               ke_task_id_t const src_id)
 * ----------------------------------------------------------------------------
 * Description   : Search the lists and call back the functions associated with
 *                 the msg_id, following the priority order (HIGH to LOW). This
 *                 function was designed to be used as the default handler of
 *                 the kernel and shall NOT be called directly by the
 *                 application. To notify an event, the application should
 *                 enqueue a message in the kernel, in order to avoid chaining
 *                 the context of function calls (stack overflow).
 * Inputs        : msg_id   - A task identifier, such as TASK_ID_GAPM
 *                            or a message identifier, such as GAPM_CMP_EVT;
 *                 param    - Message parameter
 *                 dest_id  - destination task
 *                 src_id   - source task
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
int MsgHandler_Notify(ke_msg_id_t const msg_id, void *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    MsgHandler_t *tmp = msgHandlerHead;
    uint8_t task_id = KE_IDX_GET(msg_id);

    /* First notify abstraction layer handlers */
    switch (task_id)
    {
        case TASK_ID_GAPC:
        {
            GAPC_MsgHandler(msg_id, param, dest_id, src_id);
        }
        break;

        case TASK_ID_GAPM:
        {
            GAPM_MsgHandler(msg_id, param, dest_id, src_id);
        }
        break;

        case TASK_ID_GATTC:
        {
            GATTC_MsgHandler(msg_id, param, dest_id, src_id);
        }
        break;

        case TASK_ID_GATTM:
        {
            GATTM_MsgHandler(msg_id, param, dest_id, src_id);
        }
        break;
    }

    /* Notify subscribed application/profile handlers */
    while (tmp)
    {
        /* If message ID matches or the handler should be called for all
         * messages of this task type */
        if ((tmp->msg_id == msg_id) || (tmp->msg_id == task_id))
        {
            tmp->callback(msg_id, param, dest_id, src_id);
        }
        tmp = tmp->next;
    }
    return KE_MSG_CONSUMED;
}
