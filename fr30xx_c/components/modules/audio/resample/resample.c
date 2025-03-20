#include "audio_rpmsg.h"
#include "resample.h"
#include "FreeRTOS.h"
#include "dsp.h"

void *resample_init(enum resample_type type, uint8_t channels)
{
    struct rpmsg_sync_msg_resample_init_t *sync_msg;
    void *result;
    uint32_t ret;
    
    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_decoder_init_t));
    if (sync_msg == NULL) {
        return NULL;
    }
    sync_msg->resample_type = type;
    sync_msg->channels = channels;
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_RESAMPLE_INIT, sync_msg, (uint32_t *)&result);

    vPortFree(sync_msg);

    return result;
}

void resample_destroy(void *handle)
{
    struct rpmsg_sync_msg_resample_destroy_t sync_msg;
    void *result;
    uint32_t ret;
    
    if (handle == NULL) {
        return;
    }
    
    sync_msg.handle = handle;

    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_RESAMPLE_DESTROY, (void *)&sync_msg, (uint32_t *)&result);
}

int resample_exec(void *handle, const uint8_t *indata, uint32_t *insize, uint8_t **out_buf, uint32_t *out_length)
{
    struct rpmsg_sync_msg_resample_exec_t *sync_msg;
    void *result;
    uint32_t ret;
    bool trans_addr = false;

    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_encoder_exec_t));
    if (sync_msg == NULL) {
        return -1;
    }
    
      if (((uint32_t)indata >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)indata < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        indata = (void *)((uint32_t)indata - DSP_DRAM_MCU_BASE_ADDR + DSP_DRAM_BASE_ADDR);
    }

    sync_msg->handle = handle;
    sync_msg->in_buffer = indata;
    sync_msg->in_length = insize;
    sync_msg->out_buffer = out_buf;
    sync_msg->out_length = out_length;

    if (*out_buf == NULL) {
        trans_addr = true;
    }
    
    ret = rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_RESAMPLE_EXEC, sync_msg, (uint32_t *)&result);
    
    if (trans_addr) {
        *out_buf = (void *)(DSP_DRAM_MCU_BASE_ADDR + (uint32_t)*out_buf - DSP_DRAM_BASE_ADDR);
    }

    vPortFree(sync_msg);

    return (int)result;
}

enum resample_type resample_get_type(uint32_t in_sample_rate, uint32_t out_sample_rate)
{
    switch (in_sample_rate) {
        case 48000:
            switch (out_sample_rate) {
                case 44100:
                    return RESAMPLE_TYPE_D_48000_44100;
                case 16000:
                   return RESAMPLE_TYPE_D_48000_16000;
                default:
                    return RESAMPLE_TYPE_INVALID;
            }
       case 44100:
           switch (out_sample_rate) {
               case 16000:
                   return RESAMPLE_TYPE_D_44100_16000;
               default:
                   return RESAMPLE_TYPE_INVALID;
           }
        default:
            return RESAMPLE_TYPE_INVALID;
    }
}

