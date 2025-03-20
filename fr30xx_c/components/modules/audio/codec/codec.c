/*
 * codec.c
 *
 *  Created on: 2018-3-28
 *      Author: Administrator
 */
#include <stdio.h>
#include <stdlib.h>

#include "codec.h"
#include "audio_rpmsg.h"
#include "dsp.h"

/************************************************************************************
 * @fn      codec_decoder_init
 *
 * @brief   reqeust to init a decoder instance.
 *
 * @param   decoder_type: decoder type, @ref codec_decoder_type.
 * @param   param: decoder parameters.
 *
 * @return  decoder instance, the value should be NULL when initialization is failed.
 */
struct codec_decoder_handle *codec_decoder_init(uint8_t decoder_type, void *param)
{
    struct rpmsg_sync_msg_decoder_init_t *sync_msg;
    void *result;
    uint32_t ret;
    
    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_init_t));
    if (sync_msg == NULL) {
        return NULL;
    }
    sync_msg->decoder_type = decoder_type;
    sync_msg->param = param;
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_INIT, sync_msg, (uint32_t *)&result);

    vPortFree(sync_msg);

    return result;
}

/************************************************************************************
 * @fn      codec_decoder_destroy
 *
 * @brief   remove a created decoder instance.
 *
 * @param   handle: decoder instance
 */
void codec_decoder_destroy(struct codec_decoder_handle *handle)
{
    struct rpmsg_sync_msg_decoder_destroy_t sync_msg;
    void *result;
    uint32_t ret;
    
    if (handle == NULL) {
        return;
    }
    
    sync_msg.handle = handle;

    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_DESTROY, (void *)&sync_msg, (uint32_t *)&result);
}

/************************************************************************************
 * @fn      codec_decoder_decode
 *
 * @brief   reqeust to execute decode.
 *
 * @param   handle: decoder instance
 * @param   in_buf: buffer used to store raw data, caller should take care that this 
 *                  buffer should be accessable for dsp.
 * @param   in_length: the length of raw data, this value will be updated to length of
 *                     dealed data after decode operation is executed.
 * @param   out_buf: used to store the buffer address of decoded data. Decoder will allocate
 *                   out buffer for internal used to store decoded data. If *out_buf is NULL,
 *                   decoder will update *out_buf to internal used out buffer address. If *out_buf
 *                   is not NULL, the decoded data will be copied into *out_buf.
 * @param   out_length: the length of decoded data.
 *
 * @return  execute result.
 */
int codec_decoder_decode(struct codec_decoder_handle *handle,
                            const uint8_t *in_buf,
                            uint32_t *in_length,
                            uint8_t **out_buf,
                            uint32_t *out_length)
{
    struct rpmsg_sync_msg_decoder_exec_t *sync_msg;
    //void *result;
    int result;
    uint32_t ret;
    bool trans_addr = false;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_exec_t));
    if (sync_msg == NULL) {
        return CODEC_ERROR_INSUFFICIENT_RESOURCE;
    }
    
    if (((uint32_t)in_buf >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)in_buf < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        in_buf = (const void *)MCU_SRAM_2_DSP_DRAM(in_buf);
    }
    
    sync_msg->handle = handle;
    sync_msg->in_buffer = in_buf;
    sync_msg->in_length = in_length;
    sync_msg->out_buffer = out_buf;
    sync_msg->out_length = out_length;

    if (*out_buf == NULL) {
        trans_addr = true;
    }
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_EXEC, sync_msg, (uint32_t *)&result);

    if (trans_addr) {
        if ((uint32_t)*out_buf >= DSP_DRAM_BASE_ADDR) {
            *out_buf = (void *)DSP_DRAM_2_MCU_SRAM(*out_buf);
        }
    }
    
    vPortFree(sync_msg);

    return (int)result;
}

/************************************************************************************
 * @fn      codec_decoder_plc
 *
 * @brief   reqeust to execute packet loss compensation.
 *
 * @param   handle: decoder instance
 * @param   out_buf: used to store the buffer address of decoded data. Decoder will allocate
 *                   out buffer for internal used to store decoded data. If *out_buf is NULL,
 *                   decoder will update *out_buf to internal used out buffer address. If *out_buf
 *                   is not NULL, the decoded data will be copied into *out_buf.
 * @param   out_length: the length of decoded data.
 *
 * @return  execute result.
 */
int codec_decoder_plc(struct codec_decoder_handle *handle,
                            uint8_t **out_buf,
                            uint32_t *out_length)
{
    struct rpmsg_sync_msg_decoder_plc_t *sync_msg;
    void *result;
    uint32_t ret;
    bool trans_addr = false;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_exec_t));
    if (sync_msg == NULL) {
        return CODEC_ERROR_INSUFFICIENT_RESOURCE;
    }
    sync_msg->handle = handle;
    sync_msg->out_buffer = out_buf;
    sync_msg->out_length = out_length;

    if (*out_buf == NULL) {
        trans_addr = true;
    }
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_PLC, sync_msg, (uint32_t *)&result);

    if (trans_addr) {
        if ((uint32_t)*out_buf >= DSP_DRAM_BASE_ADDR) {
            *out_buf = (void *)DSP_DRAM_2_MCU_SRAM(*out_buf);
        }
    }
    
    vPortFree(sync_msg);

    return (int)result;
}

/************************************************************************************
 * @fn      codec_decoder_input_over
 *
 * @brief   reqeust to execute action when the input of an audio sample to the decoder instance is over.
 *
 * @param   handle: decoder instance
 * @param   out_buf: used to store the buffer address of decoded data. Decoder will allocate
 *                   out buffer for internal used to store decoded data. If *out_buf is NULL,
 *                   decoder will update *out_buf to internal used out buffer address. If *out_buf
 *                   is not NULL, the decoded data will be copied into *out_buf.
 * @param   out_length: the length of decoded data.
 * @param   exec_done: returned by DSP decoder instance to tell whether all of input audio data sample are
 *                      decoded and returned or not.
 *
 * @return  execute result.
 */
int codec_decoder_input_over(struct codec_decoder_handle *handle,
                            uint8_t **out_buf,
                            uint32_t *out_length,
                            uint8_t *exec_done)
{
    struct rpmsg_sync_msg_decoder_input_over_t *sync_msg;
    void *result;
    uint32_t ret;
    bool trans_addr = false;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_input_over_t));
    if (sync_msg == NULL) {
        return CODEC_ERROR_INSUFFICIENT_RESOURCE;
    }
    sync_msg->handle = handle;
    sync_msg->out_buffer = out_buf;
    sync_msg->out_length = out_length;
    sync_msg->exec_done = exec_done;

    if (*out_buf == NULL) {
        trans_addr = true;
    }
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_INPUT_DONE, sync_msg, (uint32_t *)&result);

    if (trans_addr) {
        if ((uint32_t)*out_buf >= DSP_DRAM_BASE_ADDR) {
            *out_buf = (void *)DSP_DRAM_2_MCU_SRAM(*out_buf);
        }
    }
    
    vPortFree(sync_msg);

    return (int)result;
}

/************************************************************************************
 * @fn      codec_decoder_get_param
 *
 * @brief   get information of latest decoded frame.
 *
 * @param   handle: decoder instance
 * @param   sample_rate: sample rate.
 * @param   channels: channel number.
 *
 * @return  0: get success, 1: get failed.
 */
int codec_decoder_get_param(struct codec_decoder_handle *handle, uint32_t *sample_rate, uint8_t *channels)
{
    struct rpmsg_sync_msg_decoder_get_param_t *sync_msg;
    void *result;
    uint32_t ret;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_get_param_t));
    if (sync_msg == NULL) {
        return CODEC_ERROR_INSUFFICIENT_RESOURCE;
    }
    sync_msg->handle = handle;
    sync_msg->sample_rate = sample_rate;
    sync_msg->channels = channels;

    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_DEC_GET_PARAM, sync_msg, (uint32_t *)&result);

    vPortFree(sync_msg);
    
    return (int)result;
}

/************************************************************************************
 * @fn      codec_encoder_init
 *
 * @brief   reqeust to init a encoder instance.
 *
 * @param   encoder_type: encoder type, @ref codec_encoder_type.
 * @param   param: encoder parameters.
 *
 * @return  encoder instance, the value should be NULL when initialization is failed.
 */
struct codec_encoder_handle *codec_encoder_init(uint8_t encoder_type, void *param)
{
    struct rpmsg_sync_msg_encoder_init_t *sync_msg;
    void *result;
    uint32_t ret;
    
    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_encoder_init_t));
    if (sync_msg == NULL) {
        return NULL;
    }
    sync_msg->encoder_type = encoder_type;
    sync_msg->param = param;
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_ENC_INIT, sync_msg, (uint32_t *)&result);

    vPortFree(sync_msg);

    return result;
}

/************************************************************************************
 * @fn      codec_encoder_destroy
 *
 * @brief   remove a created encoder instance.
 *
 * @param   handle: encoder instance
 */
void codec_encoder_destroy(struct codec_encoder_handle *handle)
{
    struct rpmsg_sync_msg_encoder_destroy_t sync_msg;
    void *result;
    uint32_t ret;
    
    if (handle == NULL) {
        return;
    }
    
    sync_msg.handle = handle;

    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_ENC_DESTROY, (void *)&sync_msg, (uint32_t *)&result);
}

/************************************************************************************
 * @fn      codec_encoder_encode
 *
 * @brief   reqeust to execute encode.
 *
 * @param   handle: encoder instance
 * @param   in_buf: buffer used to store PCM data, caller should take care that this 
 *                  buffer should be accessable for dsp.
 * @param   in_length: the length of PCM data, this value will be updated to length of
 *                     dealed data after encode operation is executed.
 * @param   out_buf: used to store the buffer address of encoded data. Encoder will allocate
 *                   out buffer for internal used to store decoded data. If *out_buf is NULL,
 *                   decoder will update *out_buf to internal used out buffer address. If *out_buf
 *                   is not NULL, the decoded data will be copied into *out_buf.
 * @param   out_length: the length of encoded data.
 *
 * @return  execute result.
 */
int codec_encoder_encode(struct codec_encoder_handle *handle,
                            const uint8_t *in_buf,
                            uint32_t *in_length,
                            uint8_t **out_buf,
                            uint32_t *out_length)
{
    struct rpmsg_sync_msg_encoder_exec_t *sync_msg;
    int result;
    uint32_t ret;
    bool trans_addr = false;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_encoder_exec_t));
    if (sync_msg == NULL) {
        return CODEC_ERROR_INSUFFICIENT_RESOURCE;
    }
    
    if (((uint32_t)in_buf >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)in_buf < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        in_buf = (void *)MCU_SRAM_2_DSP_DRAM(in_buf);
    }
    
    sync_msg->handle = handle;
    sync_msg->in_buffer = in_buf;
    sync_msg->in_length = in_length;
    sync_msg->out_buffer = out_buf;
    sync_msg->out_length = out_length;
    
    if (*out_buf == NULL) {
        trans_addr = true;
    }

    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_ENC_EXEC, sync_msg, (uint32_t *)&result);
    
    if (trans_addr) {
        if ((uint32_t)*out_buf >= DSP_DRAM_BASE_ADDR) {
            *out_buf = (void *)DSP_DRAM_2_MCU_SRAM(*out_buf);
        }
    }

    vPortFree(sync_msg);

    return (int)result;
}

