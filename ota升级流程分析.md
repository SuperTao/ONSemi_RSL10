ota升级是将编译生成的fota文件，通过特定的service里面写内容实现的。

下面来大概看一下fota这个工程的代码内容。

开始更新，往RSL10写新程序时，调用函数如下：

```
app_ble.c

static uint8_t DfusCallback(uint8_t conidx, uint16_t attidx, uint16_t handle,
                            uint8_t *toData, const uint8_t *fromData,
                            uint16_t lenData,  uint16_t operation)
{
    if (operation == GATTC_WRITE_REQ_IND)
    {
        switch (attidx)
        {
            case DFU_DATA_VAL:
            {
            	// 发送固件传到这里进行处理
                if (dfu_ccc_value)
                {
                    App_Ble_DataInd(conidx, fromData, lenData);
                }
            }
            break;
```

```
void App_Ble_DataInd(uint_fast8_t link,
                     const uint8_t *data_p, uint_fast16_t size)
{
    if (link < CFG_HDLC_NB_LINKS)
    {
    	// 对数据进行解析
        DecodeFrame(link, data_p, size);
    }
}
```

```
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

```

```
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

```


```
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
```

```
bool App_Hdlc_DataInd(uint_fast8_t link,
                      const uint8_t *data_p, uint_fast16_t size)
{
    return DataInd(&current_msg, data_p, size);
}
```

```
static bool DataInd(message_t *msg_p,
                    const uint8_t *data_p, uint_fast16_t size)
{
    /* check for message begin */
	// 数据开头
    if (msg_p->state == MSG_WAIT)
    {
        memcpy(&msg_p->header, data_p, sizeof(msg_p->header));
        data_p += sizeof(msg_p->header);
        size   -= sizeof(msg_p->header);
        msg_p->rx_len = 0;
        msg_p->state  = MSG_BEGIN;
        HandleMsg(msg_p, NULL, 0);
    }

    /* remove message padding */
    if (size + msg_p->rx_len > msg_p->header.body_len)
    {
        size = msg_p->header.body_len - msg_p->rx_len;
    }

    if (size > 0)
    {
        if (msg_p->state == MSG_DATA)
        {
            HandleMsg(msg_p, data_p, size);
        }
        msg_p->rx_len += size;
    }

    /* check for message end */
    if (msg_p->rx_len == msg_p->header.body_len)
    {
        if (msg_p->state == MSG_DATA)
        {
            msg_p->state = MSG_END;
            HandleMsg(msg_p, NULL, 0);
        }
        msg_p->state = MSG_WAIT;
    }

    return true;
}
```

```
static void HandleMsg(message_t *msg_p,
                      const uint8_t *data_p, uint_fast16_t size)
					  
static bool ImageDownloadCmd(message_t *msg_p,
                             const uint8_t *data_p, uint_fast16_t size)
							 
数据传输完毕，进行校验。并设置对应的标志位。主函数中检测到标志位，将数据写入flash中。
static image_dnl_resp_status_t CheckImage(message_t *msg_p,
                                          image_dnl_t *dnl_p,
                                          uint_fast16_t size)
```

主函数将接收到的数据写入flash中。

```
int main(void)
{
    Init();

    /* kernel scheduler loop */
    for (;;)
    {
        /* Handle the kernel scheduler */
        Kernel_Schedule();

        /* Polling the modules */
        App_Dfu_Poll();
        Drv_Targ_Poll();
    }

    /* never get here */
    return 0;
}
```

```
void App_Dfu_Poll(void)
{
    if (ProgramImage(&current_msg, &image_download))
    {
        Drv_Targ_SetBackgroundFlag();
    }
}
```

```
static bool ProgramImage(message_t *msg_p, image_dnl_t *dnl_p)
{
    static const uint32_t invalid_mark_a[2] = { 0, 0 };
    // 判断状态是否正在进行，这个状态是接受完数据时改变的
    if (dnl_p->state   == PROG_ONGOING &&
        dnl_p->prog_len < msg_p->rx_len)
    {
        if (dnl_p->prog_len < dnl_p->erase_len)
        {
            uint_fast32_t len;
            uint_fast32_t adr    = dnl_p->flash_start_adr + dnl_p->prog_len;
            uint8_t      *data_p = (uint8_t *)msg_p->body_a +
                                   dnl_p->prog_len % sizeof(msg_p->body_a);

            /* check if at least two words are available to program */
            if (msg_p->rx_len - dnl_p->prog_len < sizeof(flash_quantum_t))
            {
                /* wait for more data except we reached the end of the message */
                if (msg_p->rx_len < msg_p->header.body_len)
                {
                    return true;
                }
                len = msg_p->header.body_len - dnl_p->prog_len;

                /* fill up to a flash prog quantum */
                memset(data_p + len, -1, sizeof(flash_quantum_t) - len);
            }
            // 将数据写入到flash中
            /* program flash */
            if (Drv_Flash_Program(adr, (uint32_t *)data_p))
            {
```
