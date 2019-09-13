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
 * event.h
 * - Simple event listener implementation based on a linked list
 * ----------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Date: 2019/01/11 17:48:52 $
 * ------------------------------------------------------------------------- */
#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

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
#include <ke_msg.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MsgHandler
{
    uint16_t msg_id;
    void (*callback)(ke_msg_id_t const msg_id, void const *param,
                     ke_task_id_t const dest_id, ke_task_id_t const src_id);
    struct MsgHandler *next;
} MsgHandler_t;

bool MsgHandler_Add(ke_msg_id_t const msg_id,
                    void (*callback)(ke_msg_id_t const msg_id, void const *param,
                                     ke_task_id_t const dest_id, ke_task_id_t const src_id));

bool MsgHandler_Remove(ke_msg_id_t const msg_id,
                       void (*callback)(ke_msg_id_t const msg_id, void const *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id));

int MsgHandler_Notify(ke_msg_id_t const msg_id, void *param,
                      ke_task_id_t const dest_id, ke_task_id_t const src_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* MSG_HANDLER_H */
