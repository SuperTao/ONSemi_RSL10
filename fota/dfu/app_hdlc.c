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
 * app_hdlc.c
 * - DFU data link protocol
 * ------------------------------------------------------------------------- */

#include "config.h"

#include <stdint.h>
#include <rsl10.h>
#include <rsl10_ke.h>

#include "app_hdlc.h"
#include "app_ble.h"
#include "sys_man.h"
#include "msg_handler.h"

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define CRC_CCITT_CONF          (CRC_CCITT                      |   \
                                 CRC_BIT_ORDER_NON_STANDARD     |   \
                                 CRC_FINAL_REVERSE_NON_STANDARD |   \
                                 CRC_FINAL_XOR_NON_STANDARD)

#define CRC_CCITT_GOOD          0x0F47
#define CRC_CCITT_SIZE          sizeof(crc_ccitt_t)

#define FRAME_OVERHEAD          (MIN_FRAME_LEN + 3)
#define FRAME_FLAG              0x00
#define FRAME_HDR_SIZE          1
#define MIN_FRAME_LEN           (FRAME_HDR_SIZE + CRC_CCITT_SIZE)
#define MAX_FRAME_LEN           (MIN_FRAME_LEN + CFG_HDLC_SDU_MAX_SIZE)

#define INC_SEQNUM(v)           (v = ((v) + 1) & SEQNUM_MASK)
#define SEQNUM_INVALID          -1
#define SEQNUM_MASK             0x07
#define NR_POS                  5
#define NS_POS                  1

#define I_QUEUE_SIZE            (SEQNUM_MASK + 1)
#define HDLC_WINDOW_SIZE        4    /* must be smaller than I_QUEUE_SIZE */
#define HDLC_N200_RC            2

/* ----------------------------------------------------------------------------
 * Local variables and types
 * --------------------------------------------------------------------------*/

typedef enum
{
    /* Command/Response Flag */
    CMD_FRAME   = 0x000,
    RESP_FRAME  = 0x100,
    CR_MASK     = 0x100,
    FRAME_MASK  = 0x003,

    /* Poll/Final Flag */
    P_0         = 0x000,
    P_1         = 0x010,
    P_MASK      = 0x110,
    F_0         = 0x100,
    F_1         = 0x110,
    F_MASK      = 0x110,
    NO_FRAME    = 0xE0,

    /* I Frames */
    I_FRAME     = 0x00,
    I_MASK      = 0x11,

    /* S Frames */
    S_FRAME     = 0x01,
    S_MASK      = 0x1F,
    RR          = 0x01,    /* Cmd & Resp */
    RNR         = 0x05,    /* Cmd & Resp */
    REJ         = 0x09,    /* only Resp */
    SREJ        = 0x0D,    /* only Resp */
    /* U Frames */
    U_FRAME     = 0x03,
    U_MASK      = 0xFF,
    SNRM        = 0x83,    /* only Cmd */
    SNRME       = 0xCF,    /* only Cmd */
    SARM        = 0x0F,    /* only Cmd */
    SARME       = 0x4F,    /* only Cmd */
    SABM        = 0x2F,    /* only Cmd */
    SABME       = 0x6F,    /* only Cmd */
    SIM         = 0x07,    /* only Cmd */
    DISC        = 0x43,    /* only Cmd */
    UA          = 0x63,    /* only Resp */
    DM          = 0x0F,    /* only Resp */
    RD          = 0x43,    /* only Resp */
    RIM         = 0x07,    /* only Resp */
    UI          = 0x03,    /* Cmd & Resp */
    UP          = 0x23,    /* only Cmd */
    RSET        = 0x8F,    /* only Cmd */
    XID         = 0xAF,    /* Cmd & Resp */
    TEST        = 0xE3,    /* Cmd & Resp */
    FRMR        = 0x87,    /* only Resp */
    NR0         = 0x0B,    /* Cmd & Resp */
    NR1         = 0x8B,    /* Cmd & Resp */
    NR2         = 0x4B,    /* Cmd & Resp */
    NR3         = 0xCB,    /* Cmd & Resp */
    AC0         = 0x67,    /* Cmd & Resp */
    AC1         = 0xE7,    /* Cmd & Resp */
    CFGR        = 0xC7,    /* Cmd & Resp */
    BCN         = 0xE7,    /* only Resp */
} hdlc_frame_types_t;

typedef enum
{
    /* uncorrectable link errors */
    LINK_ERROR_NR,
    LINK_ERROR_RC
} link_errors_t;

typedef int8_t hdlc_seqnum_t;

typedef struct
{
    const uint8_t *data_p;
    uint16_t size;
} hdls_queue_entry_t;

typedef struct
{
    hdlc_seqnum_t va;
    hdlc_seqnum_t vs;
    hdlc_seqnum_t vq;
    hdlc_seqnum_t vr;
    bool peer_reveiver_busy;
    bool own_receiver_busy;
    bool ack_pending;
    uint8_t s_frame_pending;
    uint8_t rc;
    hdls_queue_entry_t i_queue_a[I_QUEUE_SIZE];
} hdlc_state_t;

typedef uint16_t crc_ccitt_t;

typedef enum
{
    DECODE_FRAME,
    DECODE_SYNC,

    ENCODE_IDLE,
    ENCODE_BUSY,
    ENCODE_HDR,
    ENCODE_DATA,
    ENCODE_FCS
} coder_state_t;

typedef struct
{
    coder_state_t state;
    uint16_t max_size;
    uint16_t seq_nb;
    uint8_t fragment_a[CFG_BLE_MAX_DATA_SIZE];
} encoder_state_t;

typedef struct
{
    coder_state_t state;
    int16_t cobs_cnt;
    crc_ccitt_t frame_crc;
    uint16_t frame_len;
    uint8_t frame_a[MAX_FRAME_LEN];
} decoder_state_t;

static hdlc_state_t hdlc_state_a[CFG_HDLC_NB_LINKS];
static encoder_state_t encoder_state_a[CFG_HDLC_NB_LINKS];
static decoder_state_t decoder_state_a[CFG_HDLC_NB_LINKS];

/* ----------------------------------------------------------------------------
 * Function      : bool EncodeFrame(uint_fast8_t link, uint_fast8_t header,
 *                                  const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Builds a frame from header, SDU and FCS, COBS encodes it
 *                 and sends it over BLE.
 * Inputs        : link             - link ID
 * Inputs        : header           - frame header
 * Inputs        : data_p           - pointer to SDU
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  if BLE transmitter is ready
 * Outputs       : return value     - false if BLE transmitter is busy
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool EncodeFrame(uint_fast8_t link, uint_fast8_t header,
                        const uint8_t *data_p, uint_fast16_t size)
{
    #define COBS_ENCODE(o)          \
        octet = (o) & 0xFF;         \
        if (octet == FRAME_FLAG)    \
        {                           \
            *code_p = code;         \
            code_p  = dst_p++;      \
            code    = 0x01;         \
        }                           \
        else                        \
        {                           \
            *dst_p++ = octet;       \
            code++;                 \
        }

    uint_fast8_t octet;
    crc_ccitt_t crc;
    encoder_state_t    *state_p   = &encoder_state_a[link];
    uint_fast8_t code      = FRAME_FLAG;
    uint8_t            *dst_p     = state_p->fragment_a;
    uint8_t            *code_p    = dst_p++;

    if (size > state_p->max_size)
    {
        return false;
    }

    /* Select correct CRC algorithm for FCS */
    CRC->CTRL  = CRC_CCITT_CONF;
    CRC->VALUE = CRC_CCITT_INIT_VALUE;
    COBS_ENCODE(FRAME_FLAG);
    COBS_ENCODE(CRC->ADD_8 = header);
    for (; size > 0; size--)
    {
        COBS_ENCODE(CRC->ADD_8 = *data_p++);
    }
    crc = CRC->FINAL;
    COBS_ENCODE(crc >> 0);
    COBS_ENCODE(crc >> 8);
    *code_p  = code;
    *dst_p++ = FRAME_FLAG;

    size = dst_p - state_p->fragment_a;
    App_Ble_DataReq(link, state_p->seq_nb++, state_p->fragment_a, size);

    return true;
}

/* ----------------------------------------------------------------------------
 * Function      : uint_fast16_t DecodeFrameType(const uint8_t *frame_p)
 * ----------------------------------------------------------------------------
 * Description   : Decodes a frame header.
 * Inputs        : frame_p          - pointer to frame
 * Outputs       : return value     - frame type code
 *                                    (NO_FRAME for unknown frames)
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static uint_fast16_t DecodeFrameType(const uint8_t *frame_p)
{
    uint_fast16_t header = *frame_p;

    switch (header & FRAME_MASK)
    {
        case U_FRAME:
        {
            switch (header)
            {
                case SABM:
                case SABME:
                case DISC:
                case UI:
                {
                    return (header & U_MASK) | CMD_FRAME;
                }

                case UA:
                case DM:
                case FRMR:
                    return (header & U_MASK) | RESP_FRAME;
            }
        }
        break;

        case S_FRAME:
        {
            /* in this implementation all S frames are responses */
            return (header & S_MASK) | RESP_FRAME;
        }

        default:
        {
            /* in this implementation all I frames are commands */
            return (header & I_MASK) | CMD_FRAME;
        }
    }
    return NO_FRAME;
}

/* ----------------------------------------------------------------------------
 * Function      : void LinkError(hdlc_state_t *state_p, link_errors_t error)
 * ----------------------------------------------------------------------------
 * Description   : Handles an uncorrectable link error.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : error            - lenk error ID
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void LinkError(hdlc_state_t *state_p, link_errors_t error)
{
    /* not implemented */
}

/* ----------------------------------------------------------------------------
 * Function      : void SendMsg(hdlc_state_t *state_p, ke_msg_id_t msg)
 * ----------------------------------------------------------------------------
 * Description   : Sends a kernel message.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : msg              - message ID
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void SendMsg(hdlc_state_t *state_p, ke_msg_id_t msg)
{
    ke_task_id_t task_id = KE_BUILD_ID(TASK_APP, state_p - hdlc_state_a);

    ke_msg_send_basic(msg, task_id, task_id);
}

/* ----------------------------------------------------------------------------
 * Function      : bool SendFrame(hdlc_state_t *state_p, uint_fast8_t header,
 *                                const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Sends a frame.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : header           - frame header
 * Inputs        : data_p           - pointer to SDU (or NULL for no SDU)
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  if BLE transmitter is ready
 * Outputs       : return value     - false if BLE transmitter is busy
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool SendFrame(hdlc_state_t *state_p, uint_fast8_t header,
                      const uint8_t *data_p, uint_fast16_t size)
{
    /* a pending S frame has priority over a I frame */
    if (data_p != NULL && state_p->s_frame_pending != NO_FRAME)
    {
        header = state_p->s_frame_pending;
        data_p = NULL;
        size   = 0;
    }

    /* encode frame and try to send it */
    if (EncodeFrame(state_p - hdlc_state_a, header, data_p, size))
    {
        state_p->s_frame_pending = NO_FRAME;
        return (data_p != NULL);
    }

    /* if S frame could not be sent, set it as pending */
    if (data_p == NULL)
    {
        state_p->s_frame_pending = header;
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : void StartT200(hdlc_state_t *state_p)
 * ----------------------------------------------------------------------------
 * Description   : Starts timer T200 if it is not already running.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StartT200(hdlc_state_t *state_p)
{
    ke_task_id_t task_id = KE_BUILD_ID(TASK_APP, state_p - hdlc_state_a);

    if (!ke_timer_active(APP_HDLC_T200, task_id))
    {
        ke_timer_set(APP_HDLC_T200, task_id, KE_TIME_IN_SEC(CFG_HDLC_T200));
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void StopT200(hdlc_state_t *state_p)
 * ----------------------------------------------------------------------------
 * Description   : Stops timer T200.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void StopT200(hdlc_state_t *state_p)
{
    ke_task_id_t task_id = KE_BUILD_ID(TASK_APP, state_p - hdlc_state_a);

    ke_timer_clear(APP_HDLC_T200, task_id);
}

/* ----------------------------------------------------------------------------
 * Function      : hdlc_seqnum_t CheckNR(hdlc_state_t  *state_p,
 *                                       const uint8_t *frame_p)
 * ----------------------------------------------------------------------------
 * Description   : Checks if the value of the N(R) frame header field is
 *                 in the range V(A) <= N(R) <= V(S).
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : frame_p          - pointer to frame
 * Outputs       : return value     - N(R) value or
 *                                    SEQNUM_INVALID if out of range
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static hdlc_seqnum_t CheckNR(hdlc_state_t *state_p, const uint8_t *frame_p)
{
    hdlc_seqnum_t nr = (*frame_p >> NR_POS) & SEQNUM_MASK;

    if (((nr - state_p->va) & SEQNUM_MASK) <=
        ((state_p->vs - state_p->va) & SEQNUM_MASK))
    {
        return nr;
    }
    return SEQNUM_INVALID;
}

/* ----------------------------------------------------------------------------
 * Function      : hdlc_seqnum_t CheckNS(hdlc_state_t  *state_p,
 *                                       const uint8_t *frame_p)
 * ----------------------------------------------------------------------------
 * Description   : Checks if the value of the N(S) frame header field
 *                 equals V(R).
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : frame_p          - pointer to frame
 * Outputs       : return value     - N(R) value or
 *                                    SEQNUM_INVALID if unequal
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static hdlc_seqnum_t CheckNS(hdlc_state_t *state_p, const uint8_t *frame_p)
{
    hdlc_seqnum_t ns = (*frame_p >> NS_POS) & SEQNUM_MASK;

    if (ns == state_p->vr)
    {
        return ns;
    }
    return SEQNUM_INVALID;
}

/* ----------------------------------------------------------------------------
 * Function      : bool InsertInIQueue(hdlc_state_t  *state_p,
 *                                     const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Inserts a SDU into the I frame queue.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : data_p           - pointer to SDU
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  if SDU could be placed into queue
 * Outputs       : return value     - false if queue is full
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool InsertInIQueue(hdlc_state_t *state_p,
                           const uint8_t *data_p, uint_fast16_t size)
{
    hdlc_seqnum_t vq = state_p->vq;

    if (((vq - state_p->va) & SEQNUM_MASK) < HDLC_WINDOW_SIZE)
    {
        state_p->i_queue_a[vq].data_p = data_p;
        state_p->i_queue_a[vq].size   = size;
        INC_SEQNUM(state_p->vq);
        return true;
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : void TransmitSFrame(hdlc_state_t      *state_p,
 *                                     hdlc_frame_types_t type)
 * ----------------------------------------------------------------------------
 * Description   : Transmits a S frame.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : type             - frame type code
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void TransmitSFrame(hdlc_state_t *state_p, uint_fast16_t type)
{
    /* only response frames are supported */
    uint_fast8_t header = type | (state_p->vr << NR_POS);

    SendFrame(state_p, header, NULL, 0);
    state_p->ack_pending = false;
}

/* ----------------------------------------------------------------------------
 * Function      : void TransmitIFrames(hdlc_state_t *state_p)
 * ----------------------------------------------------------------------------
 * Description   : Transmits all pending I frames.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void TransmitIFrames(hdlc_state_t *state_p)
{
    hdlc_seqnum_t vs;

    if (!state_p->peer_reveiver_busy)
    {
        for (vs = state_p->vs; vs != state_p->vq; INC_SEQNUM(vs))
        {
            uint_fast8_t header = I_FRAME | P_0 |
                                  (state_p->vr << NR_POS) |
                                  (state_p->vs << NS_POS);
            if (!SendFrame(state_p, header,
                           state_p->i_queue_a[vs].data_p,
                           state_p->i_queue_a[vs].size))
            {
                break;
            }
            state_p->ack_pending = false;
            StartT200(state_p);
        }
        state_p->vs = vs;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void DiscardIFrames(hdlc_state_t *state_p, hdlc_seqnum_t nr)
 * ----------------------------------------------------------------------------
 * Description   : Discards I frames up to N(R) from the I frame queue.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : nr               - N(R) value
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void DiscardIFrames(hdlc_state_t *state_p, hdlc_seqnum_t nr)
{
    hdlc_seqnum_t va;

    for (va = state_p->va; va != nr; INC_SEQNUM(va))
    {
        state_p->rc = 0;
        App_Hdlc_DataCfm(state_p - hdlc_state_a,
                         state_p->i_queue_a[va].data_p);
    }
    if (va == state_p->vs)
    {
        StopT200(state_p);
    }
    state_p->va = va;
}

/* ----------------------------------------------------------------------------
 * Function      : void EnquiryResponse(hdlc_state_t *state_p)
 * ----------------------------------------------------------------------------
 * Description   : Sends back the enquiry response.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void EnquiryResponse(hdlc_state_t *state_p)
{
    /* not implemented */
}

/* ----------------------------------------------------------------------------
 * Function      : void SFrameInd(hdlc_state_t *state_p, const uint8_t *frame_p)
 * ----------------------------------------------------------------------------
 * Description   : Handles a received S frame.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : frame_p          - pointer to frame
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void SFrameInd(hdlc_state_t *state_p, const uint8_t *frame_p)
{
    hdlc_seqnum_t nr = CheckNR(state_p, frame_p);

    if (nr >= 0)
    {
        DiscardIFrames(state_p, nr);
        TransmitIFrames(state_p);
    }
    else
    {
        LinkError(state_p, LINK_ERROR_NR);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void IFrameInd(hdlc_state_t  *state_p,
 *                                const uint8_t *frame_p, uint_fast16_t len)
 * ----------------------------------------------------------------------------
 * Description   : Handles a received I frame.
 * Inputs        : state_p          - pointer to HDLC state vector
 * Inputs        : frame_p          - pointer to frame
 * Inputs        : len              - frame length (excluding FCS)
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void IFrameInd(hdlc_state_t *state_p,
                      const uint8_t *frame_p, uint_fast16_t len)
{
    hdlc_seqnum_t ns = CheckNS(state_p, frame_p);
    // 正在忙，通过notify把这个结果返回给主机
    if (state_p->own_receiver_busy)
    {
        TransmitSFrame(state_p, RNR | F_0);
    }
    else if (ns < 0)
    {
        TransmitSFrame(state_p, REJ | F_0);
    }
    // 正常处理数据
    else
    {
        if (!state_p->ack_pending)
        {
            SendMsg(state_p, APP_HDLC_ACKPEND);
            state_p->ack_pending = true;
        }
        INC_SEQNUM(state_p->vr);
        // 存储数据
        state_p->own_receiver_busy =
            !App_Hdlc_DataInd(state_p - hdlc_state_a,
                              frame_p + FRAME_HDR_SIZE,
                              len - FRAME_HDR_SIZE);
    }
    SFrameInd(state_p, frame_p);
}

/* ----------------------------------------------------------------------------
 * Function      : void AckPending(ke_msg_id_t msg_id, const void *param_p,
 *                                 ke_task_id_t dest_id, ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handles the pending acknowledge of a received I frame.
 * Inputs        : msg_id           - always APP_HDLC_ACKPEND
 * Inputs        : frame_p          - always NULL
 * Inputs        : dest_id          - the task responsible for the link
 * Inputs        : src_id           - the task responsible for the link
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void AckPending(ke_msg_id_t msg_id, const void *param_p,
                       ke_task_id_t dest_id, ke_task_id_t src_id)
{
    hdlc_state_t *state_p = &hdlc_state_a[KE_IDX_GET(dest_id)];

    if (state_p->ack_pending)
    {
        state_p->ack_pending = false;
        if (state_p->own_receiver_busy)
        {
            TransmitSFrame(state_p, RNR | F_0);
        }
        else
        {
            TransmitSFrame(state_p, RR | F_0);
        }
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void T200Expired(ke_msg_id_t msg_id, const void *param_p,
 *                                  ke_task_id_t dest_id, ke_task_id_t src_id)
 * ----------------------------------------------------------------------------
 * Description   : Handles the T200 expiry of a sent I frame.
 * Inputs        : msg_id           - always APP_HDLC_T200
 * Inputs        : frame_p          - always NULL
 * Inputs        : dest_id          - the task responsible for the link
 * Inputs        : src_id           - the task responsible for the link
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void T200Expired(ke_msg_id_t msg_id, const void *param_p,
                        ke_task_id_t dest_id, ke_task_id_t src_id)
{
    hdlc_state_t *state_p = &hdlc_state_a[KE_IDX_GET(dest_id)];

    if (++state_p->rc <= HDLC_N200_RC)
    {
        state_p->vs = state_p->va;
        TransmitIFrames(state_p);
    }
    else
    {
        LinkError(state_p, LINK_ERROR_RC);
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void FrameInd(uint_fast8_t   link,
 *                               const uint8_t *frame_p, uint_fast16_t len)
 * ----------------------------------------------------------------------------
 * Description   : Handles a received frame.
 * Inputs        : link             - link ID
 * Inputs        : frame_p          - pointer to frame
 * Inputs        : len              - frame length (including FCS)
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void FrameInd(uint_fast8_t link,
                     const uint8_t *frame_p, uint_fast16_t len)
{
    hdlc_seqnum_t nr;
    hdlc_state_t *state_p =  &hdlc_state_a[link];

    if (len > MAX_FRAME_LEN)
    {
        /* ignore too long frames */
        return;
    }
    else if (CRC->FINAL != CRC_CCITT_GOOD)
    {
        /* ignore frames with bad FCS */
        return;
    }
    else if (len < MIN_FRAME_LEN)
    {
        /* ignore too short frames */
        return;
    }

    len -= CRC_CCITT_SIZE;
    // 解析数据，或者数据帧头，然后进行判断
    switch (DecodeFrameType(frame_p))
    {
        case I_FRAME | P_0:
        {
            IFrameInd(state_p, frame_p, len);
        }
        break;

        case RR | P_1:
        {
            EnquiryResponse(state_p);

            /* FALL THROUGH */
        }

        case RR | P_0:
        case RR | F_1:
        case RR | F_0:
        {
            state_p->peer_reveiver_busy = false;
            SFrameInd(state_p, frame_p);
        }
        break;

        case RNR | P_1:
        {
            EnquiryResponse(state_p);

            /* FALL THROUGH */
        }

        case RNR | P_0:
        case RNR | F_1:
        case RNR | F_0:
        {
            state_p->peer_reveiver_busy = true;
            SFrameInd(state_p, frame_p);
        }
        break;

        case REJ | F_0:
        {
            nr = CheckNR(state_p, frame_p);
            if (nr >= 0)
            {
                state_p->vs = nr;
                state_p->peer_reveiver_busy = false;
                SFrameInd(state_p, frame_p);
            }
            else
            {
                LinkError(state_p, LINK_ERROR_NR);
            }
        }
        break;

        default:
        {
            /* ignore unknown frames */
        }
        break;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : bool DataReq(uint_fast8_t   link,
 *                              const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Handles a SDU data request from the upper layer.
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to SDU
 * Inputs        : size             - SDU size
 * Outputs       : return value     - true  if SDU could be placed into queue
 * Outputs       : return value     - false if queue is full
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static bool DataReq(uint_fast8_t link,
                    const uint8_t *data_p, uint_fast16_t size)
{
    hdlc_state_t *state_p =  &hdlc_state_a[link];

    if (InsertInIQueue(state_p, data_p, size))
    {
        TransmitIFrames(state_p);
        return true;
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * Function      : void DecodeFrame(uint_fast8_t   link,
 *                                  const uint8_t *data_p, uint_fast16_t size)
 * ----------------------------------------------------------------------------
 * Description   : Decodes a frame fragment received from the lower layer.
 * Inputs        : link             - link ID
 * Inputs        : data_p           - pointer to fragment
 * Inputs        : size             - fragment size
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
static void DecodeFrame(uint_fast8_t link,
                        const uint8_t *data_p, uint_fast16_t size)
{
    coder_state_t state     = decoder_state_a[link].state;
    int_fast16_t cobs_cnt  = decoder_state_a[link].cobs_cnt;
    uint_fast16_t frame_len = decoder_state_a[link].frame_len;
    uint8_t      *frame_a   = decoder_state_a[link].frame_a;

    CRC->VALUE = decoder_state_a[link].frame_crc;

    /* Reassemble COBS frame */
    for (; size > 0; size--)
    {
        uint8_t octet = *data_p++;

        if (octet == FRAME_FLAG)
        {
            if (frame_len > 0)
            {
                /* complete frame reception */
                FrameInd(link, frame_a, frame_len);
            }

            /* Select correct CRC algorithm for FCS */
            CRC->CTRL  = CRC_CCITT_CONF;
            CRC->VALUE = CRC_CCITT_INIT_VALUE;
            cobs_cnt   = 0;
            frame_len  = 0;
            state      = DECODE_FRAME;
        }
        else if (state == DECODE_FRAME)
        {
            if (--cobs_cnt < 0)
            {
                cobs_cnt = octet;
            }
            else
            {
                if (cobs_cnt == 0)
                {
                    cobs_cnt = octet;
                    octet = 0;
                }
                if (frame_len >= MAX_FRAME_LEN)
                {
                    /* frame too long -> abort reception */
                    FrameInd(link, frame_a, frame_len + 1);
                    frame_len = 0;
                    state     = DECODE_SYNC;
                }
                else
                {
                    CRC->ADD_8 = frame_a[frame_len++] = octet;
                }
            }
        }
    }

    decoder_state_a[link].state     = state;
    decoder_state_a[link].cobs_cnt  = cobs_cnt;
    decoder_state_a[link].frame_len = frame_len;
    decoder_state_a[link].frame_crc = CRC->VALUE;
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Hdlc_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Initializes the module
 * Inputs        : None
 * Outputs       : None
 * Assumptions   :
 * ------------------------------------------------------------------------- */
void App_Hdlc_Init(void)
{
    uint_fast8_t link;

    /* register message handlers */
    MsgHandler_Add(APP_HDLC_ACKPEND, AckPending);
    MsgHandler_Add(APP_HDLC_T200, T200Expired);

    for (link = 0; link < CFG_HDLC_NB_LINKS; link++)
    {
        hdlc_state_a[link].peer_reveiver_busy = true;
        hdlc_state_a[link].own_receiver_busy  = true;
    }
}

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
                      const uint8_t *data_p, uint_fast16_t size)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
        return DataReq(link, data_p, size);
    }
    return false;
}

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
void App_Ble_ActivationInd(uint_fast8_t link, uint_fast16_t max_size)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
        hdlc_state_a[link].va = 0;
        hdlc_state_a[link].vs = 0;
        hdlc_state_a[link].vq = 0;
        hdlc_state_a[link].vr = 0;
        hdlc_state_a[link].rc = 0;
        hdlc_state_a[link].peer_reveiver_busy = false;
        hdlc_state_a[link].own_receiver_busy  = false;
        hdlc_state_a[link].ack_pending        = false;
        hdlc_state_a[link].s_frame_pending    = NO_FRAME;

        encoder_state_a[link].state    = ENCODE_IDLE;
        encoder_state_a[link].max_size = max_size - FRAME_OVERHEAD;
        encoder_state_a[link].seq_nb   = 0;

        decoder_state_a[link].state     = DECODE_SYNC;
        decoder_state_a[link].frame_len = 0;
    }
}

/* ----------------------------------------------------------------------------
 * Function      : void App_Ble_DeactivationInd(uint_fast8_t  link)
 * ----------------------------------------------------------------------------
 * Description   : Indicates link deactivation
 * Inputs        : link             - link ID
 * Outputs       : None
 * Assumptions   : link is active
 * ------------------------------------------------------------------------- */
void App_Ble_DeactivationInd(uint_fast8_t link)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
        hdlc_state_a[link].peer_reveiver_busy = true;
        hdlc_state_a[link].own_receiver_busy  = true;
    }
}

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
                     App_Ble_status_t status)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
        /* not implemented yet */
    }
}

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
                     const uint8_t *data_p, uint_fast16_t size)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
    	// 对数据进行解析
        DecodeFrame(link, data_p, size);
    }
}
