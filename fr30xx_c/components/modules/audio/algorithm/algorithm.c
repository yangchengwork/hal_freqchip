#include "audio_rpmsg.h"
#include "algorithm.h"
#include "FreeRTOS.h"
#include "dsp.h"

void *algorithm_init(uint8_t algo_sel, uint32_t sample_rate, uint8_t ns_level, uint16_t agc_mode, uint32_t *frame_size)
{
    struct rpmsg_sync_msg_algorithm_init *sync_msg;
    void *result;
    
    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_algorithm_init));
    if (sync_msg == NULL) {
        return NULL;
    }
    sync_msg->algo_sel = algo_sel;
    sync_msg->sample_rate = sample_rate;
    sync_msg->ns_level = ns_level;
    sync_msg->agc_mode = agc_mode;
    sync_msg->frame_size = frame_size;
    
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_AUDIO_ALGO_INIT, sync_msg, (uint32_t *)&result);

    vPortFree(sync_msg);

    return result;
}

int algorithm_launch(void *handle, int16_t *farend, int16_t *nearend, int16_t **out)
{
    struct rpmsg_sync_msg_algorithm_launch *sync_msg;
    int result;
    bool trans_addr = false;
    
    sync_msg = pvPortMalloc(sizeof(struct rpmsg_sync_msg_algorithm_launch));
    if (sync_msg == NULL) {
        return -1;
    }
    
    if (((uint32_t)farend >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)farend < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        farend = (void *)MCU_SRAM_2_DSP_DRAM(farend);
    }
    if (((uint32_t)nearend >= DSP_DRAM_MCU_BASE_ADDR) && ((uint32_t)nearend < (DSP_DRAM_MCU_BASE_ADDR+DSP_DRAM_SIZE))) {
        nearend = (void *)MCU_SRAM_2_DSP_DRAM(nearend);
    }
    
    sync_msg->handle = handle;
    sync_msg->farend = farend;
    sync_msg->nearend = nearend;
    sync_msg->out = out;
    
    if (*out == NULL) {
        trans_addr = true;
    }
    
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_AUDIO_ALGO_LAUNCH, sync_msg, NULL);
    
    if (trans_addr) {
        *out = (void *)DSP_DRAM_2_MCU_SRAM(*out);
    }

    vPortFree(sync_msg);
    
    return 0;
}

void algorithm_destroy(void *handle)
{
    struct rpmsg_sync_msg_algorithm_destroy_t sync_msg;
    
    sync_msg.handle = handle;
    rpmsg_sync_invoke(rpmsg_get_remote_instance(), RPMSG_SYNC_FUNC_AUDIO_ALGO_RELEASE, (void *)&sync_msg, NULL);
}
