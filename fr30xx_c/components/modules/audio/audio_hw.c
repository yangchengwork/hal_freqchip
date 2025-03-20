#include "audio_hw.h"
#include "audio_common.h"

#include "fr30xx.h"
#include "FreeRTOS.h"

#define AUDIO_HW_PDM_RX_INT_LEVEL           16
#define AUDIO_HW_STORE_FRAME_COUNT          40

#define PSD_DAC_FIFO_HALF_DEPTH          32

struct audio_hw_env_t {
    struct co_list hw_list;
};

struct audio_hw_handle_t {
    audio_hw_t *audio_hw;
    void *hw_handle;
};

static struct audio_hw_env_t audio_hw_env = {0};
static struct audio_hw_handle_t i2s_hw_table[2];
static struct audio_hw_handle_t pdm_hw_table[2];

static struct audio_hw_handle_t codec_hw;
static struct audio_hw_handle_t psd_dac_hw;

static void i2s_rx_callback(I2S_HandleTypeDef *i2s_handle)
{
    uint8_t index;
    audio_hw_t *hw;

    for (index=0; index<3; index++) {
        if (i2s_hw_table[index].hw_handle == i2s_handle) {
            break;
        }
    }
    
    if (index < 3) {
        hw = i2s_hw_table[index].audio_hw;

        /* store data into internal buffer */        
        if (hw->channels == AUDIO_CHANNELS_MONO) {
            int16_t *pcm = (void *)&hw->pcm[hw->channels * sizeof(int16_t) * hw->wr_ptr];
            for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH;) {
                pcm[i++] = i2s_handle->I2Sx->DATA_L;
            }
        }
        else {
            uint32_t *pcm = (void *)&hw->pcm[hw->channels * sizeof(int16_t) * hw->wr_ptr];
            for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH;) {
                pcm[i++] = i2s_handle->I2Sx->DATA_L;
            }
        }
        
        hw->wr_ptr += I2S_FIFO_HALF_DEPTH;
        if (hw->wr_ptr >= hw->pcm_samples) {
            hw->wr_ptr = 0;
        }

        /* notify receivers new data are available */
        audio_hw_output_t *output;
        output = (void *)co_list_pick(&hw->output_list);
        while (output) {
            if (output->handler) {
                output->handler(I2S_FIFO_HALF_DEPTH);
            }
            
            output = (void *)output->hdr.next;
        }
    }
}

static void i2s_tx_callback(I2S_HandleTypeDef *i2s_handle)
{
    uint8_t index;
    audio_hw_t *hw;

    for (index=0; index<3; index++) {
        if (i2s_hw_table[index].hw_handle == i2s_handle) {
            break;
        }
    }
    
    if (index < 3) {
        hw = i2s_hw_table[index].audio_hw;
        
        /* request new data to send through I2S */
        if (hw->request_handler) {
            hw->request_handler(hw->pcm_out, I2S_FIFO_HALF_DEPTH, hw->channels);
        }
        else {
            memset(hw->pcm_out, 0, hw->channels * sizeof(int16_t) * I2S_FIFO_HALF_DEPTH);
        }
        if (hw->channels == AUDIO_CHANNELS_MONO) {
            for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH;) {
                i2s_handle->I2Sx->DATA_L = hw->pcm_out[i++];
            }
        }
        else {
            uint32_t *pcm = (void *)&hw->pcm_out[0];
            for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH;) {
                i2s_handle->I2Sx->DATA_L = pcm[i++];
            }
        }
    }
}

static void pdm_rx_callback(PDM_HandleTypeDef *pdm_handle)
{
    uint8_t index;
    audio_hw_t *hw;

    for (index=0; index<3; index++) {
        if (pdm_hw_table[index].hw_handle == pdm_handle) {
            break;
        }
    }
    
    if (index < 3) {
        hw = pdm_hw_table[index].audio_hw;

        /* store data into internal buffer */
        pdm_read_data(pdm_handle, (void *)&hw->pcm[hw->channels * sizeof(int16_t) * hw->wr_ptr]);
        hw->wr_ptr += AUDIO_HW_PDM_RX_INT_LEVEL;
        if (hw->wr_ptr >= hw->pcm_samples) {
            hw->wr_ptr = 0;
        }

        /* notify receivers new data are available */
        audio_hw_output_t *output;
        output = (void *)co_list_pick(&hw->output_list);
        while (output) {
            if (output->handler) {
                output->handler(AUDIO_HW_PDM_RX_INT_LEVEL);
            }
            
            output = (void *)output->hdr.next;
        }
    }
}

static void psd_dac_tx_callback(PSD_DAC_HandleTypeDef *PSD_DAC_Handle)
{
    audio_hw_t *hw;
    hw = psd_dac_hw.audio_hw;

    if (hw->request_handler) {
        hw->request_handler(hw->pcm_out, PSD_DAC_FIFO_HALF_DEPTH, hw->channels);
    }
    else {
        memset(hw->pcm_out, 0, hw->channels * sizeof(int16_t) * PSD_DAC_FIFO_HALF_DEPTH);
    }

    if (hw->channels == AUDIO_CHANNELS_MONO) {
        psd_dac_write_dac_fifo_left(hw->pcm_out, PSD_DAC_FIFO_HALF_DEPTH);
    }
    else {
        psd_dac_write_dac_fifo_Combine(hw->pcm_out, PSD_DAC_FIFO_HALF_DEPTH);
    }
}

void i2s0_irq(void)
{
//    fputc('I', NULL);
    i2s_IRQHandler(i2s_hw_table[0].hw_handle);
}

void i2s1_irq(void)
{
    i2s_IRQHandler(i2s_hw_table[1].hw_handle);
}

void pdm0_irq(void)
{
    pdm_IRQHandler(pdm_hw_table[0].hw_handle);
}

void pdm1_irq(void)
{
    pdm_IRQHandler(pdm_hw_table[1].hw_handle);
}

void psd_dac_irq(void)
{
    psd_dac_IRQHandler(psd_dac_hw.hw_handle);
}

audio_hw_t *audio_hw_create(audio_hw_type_t type, audio_hw_request_pcm_t handler, uint32_t base_addr, audio_hw_dir_t dir, uint32_t sample_rate, uint8_t channels)
{
    audio_hw_t *tmp;
    bool reject = false;
    
    /* check duplication */
    tmp = (void *)co_list_pick(&audio_hw_env.hw_list);
    while (tmp) {
        if (type == tmp->type) {
            reject = true;
        }
        tmp = (void *)tmp->hdr.next;
    }
    if (reject) {
        return NULL;
    }
    
    tmp = pvPortMalloc(sizeof(audio_hw_t));
    if (tmp == NULL) {
        goto __err;
    }
    tmp->type = type;
    tmp->dir = dir;
    tmp->channels = channels;
    tmp->sample_rate = sample_rate;
    tmp->base_addr = base_addr;
    tmp->hw_handle = NULL;
    tmp->request_handler = handler;
    tmp->pcm_out = NULL;
    tmp->wr_ptr = 0;
    co_list_init(&tmp->output_list);
    tmp->pcm = NULL;

    switch (type) {
        case AUDIO_HW_TYPE_I2S:
            {
                I2S_HandleTypeDef *i2s_handle = pvPortMalloc(sizeof(I2S_HandleTypeDef));
                if (i2s_handle == NULL) {
                    goto __err;
                }
                tmp->hw_handle = i2s_handle;
                
                switch (base_addr) {
                    case I2S0_BASE:
                        __SYSTEM_I2S0_CLK_ENABLE();
                        i2s_handle->I2Sx = I2S0;
                        i2s_hw_table[0].hw_handle = i2s_handle;
                        i2s_hw_table[0].audio_hw = tmp;
                        break;
                    case I2S1_BASE:
                        __SYSTEM_I2S1_CLK_ENABLE();
                        i2s_handle->I2Sx = I2S1;
                        i2s_hw_table[1].hw_handle = i2s_handle;
                        i2s_hw_table[1].audio_hw = tmp;
                        break;
                    default:
                        goto __err;
                }

                i2s_handle->Init.Mode = I2S_MODE_MASTER;
                i2s_handle->Init.Standard = I2S_STANDARD_PHILIPS;
                i2s_handle->Init.DataFormat = I2S_DATA_FORMAT_16BIT;
                if (sample_rate == 48000) {
                    /* I2S Audio Clock Source: 24.576M */
                    __SYSTEM_I2S_TUNE_SELECT_MODE1();
                    __SYSTEM_I2S_TUNE_ENABLE();
                    __SYSTEM_I2S_TUNE_CLK_GATE(42,125);
                    
                    i2s_handle->Init.BCLKDIV       = 8;  
                    i2s_handle->Init.ChannelLength = 32;
                }
                else if (sample_rate == 44100) {
                    /* I2S Audio Frequency: 22.5792M */
                    __SYSTEM_I2S_TUNE_SELECT_MODE0();
                    __SYSTEM_I2S_TUNE_ENABLE();
                    __SYSTEM_I2S_TUNE_CLK_GATE(17,625);
                    
                    i2s_handle->Init.BCLKDIV = 8;
                    i2s_handle->Init.ChannelLength = 32;
                }
                else if (sample_rate == 16000) {
                    /* I2S Audio Clock Source: 24.576M */
                    __SYSTEM_I2S_TUNE_SELECT_MODE1();
                    __SYSTEM_I2S_TUNE_ENABLE();
                    __SYSTEM_I2S_TUNE_CLK_GATE(42,125);
                    
                    i2s_handle->Init.BCLKDIV = 24;
                    i2s_handle->Init.ChannelLength = 32;
                }
                else {
                    goto __err;
                }
                
                /* insert new audio hw to list before enable interrupt */
                co_list_push_back(&audio_hw_env.hw_list, &tmp->hdr);

                /* Initialize and start I2S */
                i2s_init(i2s_handle);
                if (dir & AUDIO_HW_DIR_IN) {
                    tmp->pcm_samples = I2S_FIFO_HALF_DEPTH * AUDIO_HW_STORE_FRAME_COUNT;
                    tmp->pcm = (void *)pvPortMalloc(sizeof(int16_t) * channels * tmp->pcm_samples);
                    if (tmp->pcm == NULL) {
                        goto __err;
                    }

                    i2s_handle->RxIntCallback = i2s_rx_callback;
                    i2s_receive_IT(i2s_handle);
                }
                if (dir & AUDIO_HW_DIR_OUT) {
                    tmp->pcm_out = (void *)pvPortMalloc(sizeof(int16_t) * channels * I2S_FIFO_HALF_DEPTH);
                    if (tmp->pcm_out == NULL) {
                        goto __err;
                    }

                    i2s_handle->TxIntCallback = i2s_tx_callback;
                    i2s_transmit_IT(i2s_handle);
                }

                switch (base_addr) {
                    case I2S0_BASE:
                        NVIC_EnableIRQ(I2S0_IRQn);
                        break;
                    case I2S1_BASE:
                        NVIC_EnableIRQ(I2S1_IRQn);
                        break;
                    default:
                        goto __err;
                }
            }
            break;
        case AUDIO_HW_TYPE_PDM:
            {
                if (dir != AUDIO_HW_DIR_IN) {
                    goto __err;
                }

                PDM_HandleTypeDef *pdm_handle = pvPortMalloc(sizeof(PDM_HandleTypeDef));
                if (pdm_handle == NULL) {
                    goto __err;
                }
                tmp->hw_handle = pdm_handle;
                tmp->pcm_samples = AUDIO_HW_PDM_RX_INT_LEVEL * AUDIO_HW_STORE_FRAME_COUNT;
                tmp->pcm = (void *)pvPortMalloc(sizeof(int16_t) * channels * tmp->pcm_samples);
                if (tmp->pcm == NULL) {
                    goto __err;
                }
                
                switch (base_addr) {
                    case PDM0_BASE:
                        __SYSTEM_PDM0_CLK_ENABLE();
                        pdm_handle->PDMx = PDM0;
                        pdm_hw_table[0].hw_handle = pdm_handle;
                        pdm_hw_table[0].audio_hw = tmp;
                        break;
                    case PDM1_BASE:
                        __SYSTEM_PDM1_CLK_ENABLE();
                        pdm_handle->PDMx = PDM1;
                        pdm_hw_table[1].hw_handle = pdm_handle;
                        pdm_hw_table[1].audio_hw = tmp;
                        break;
                    default:
                        goto __err;
                }

                if (sample_rate == 48000) {
                    pdm_handle->Init.SampleRate = PDM_SAMPLE_RATE_48000;
                }
                else if (sample_rate == 44100) {
                    pdm_handle->Init.SampleRate = PDM_SAMPLE_RATE_44100;
                }
                else if (sample_rate == 16000) {
                    pdm_handle->Init.SampleRate = PDM_SAMPLE_RATE_16000;
                }
                else {
                    goto __err;
                }
                pdm_handle->Init.OverSampleMode = PDM_OSM_0;
                if (channels == AUDIO_CHANNELS_MONO) {
                    pdm_handle->Init.ChannelMode = PDM_MONO_LEFT;
                }
                else if (channels == AUDIO_CHANNELS_STEREO) {
                    pdm_handle->Init.ChannelMode = PDM_STEREO;
                }
                else {
                    goto __err;
                }
                pdm_handle->Init.Volume = 24;
                pdm_handle->Init.FIFO_FullThreshold = AUDIO_HW_PDM_RX_INT_LEVEL;
                
                pdm_handle->p_RxData = NULL;
                pdm_handle->RxCallback = pdm_rx_callback;
                
                /* insert new audio hw to list before enable interrupt */
                co_list_push_back(&audio_hw_env.hw_list, &tmp->hdr);

                pdm_init(pdm_handle);
                pdm_start_IT(pdm_handle, NULL);
                switch (base_addr) {
                    case PDM0_BASE:
                        NVIC_EnableIRQ(PDM0_IRQn);
                        break;
                    case PDM1_BASE:
                        NVIC_EnableIRQ(PDM1_IRQn);
                        break;
                    default:
                        goto __err;
                }
            }
            break;
        case AUDIO_HW_TYPE_PSD_DAC:
            {
                if (dir != AUDIO_HW_DIR_OUT) {
                    goto __err;
                }
                psd_dac_hw.hw_handle = pvPortMalloc(sizeof(PSD_DAC_HandleTypeDef));
                PSD_DAC_HandleTypeDef *PSD_DAC_Handle = psd_dac_hw.hw_handle;
                psd_dac_hw.audio_hw = tmp;
                tmp->hw_handle = PSD_DAC_Handle;
                tmp->pcm_out = (void *)pvPortMalloc(sizeof(int16_t) * channels * PSD_DAC_FIFO_HALF_DEPTH);
                if (tmp->pcm_out == NULL) {
                    goto __err;
                }
                __SYSTEM_PSD_DAC_CLK_ENABLE();
                __SYSTEM_PSD_DAC_PLL_CLK_ENABLE();
                __SYSTEM_PSD_DAC_CLK_SELECT_AUPLL();

                if (sample_rate == 48000) {
                    PSD_DAC_Handle->Init.SampleRate = PSD_DAC_SAMPLE_RATE_48000;
                }
                else if (sample_rate == 44100) {
                    PSD_DAC_Handle->Init.SampleRate = PSD_DAC_SAMPLE_RATE_44100;
                }
                else if (sample_rate == 16000) {
                    PSD_DAC_Handle->Init.SampleRate = PSD_DAC_SAMPLE_RATE_16000;
                }
                else if (sample_rate == 8000) {
                    PSD_DAC_Handle->Init.SampleRate = PSD_DAC_SAMPLE_RATE_8000;
                }
                else {
                    goto __err;
                }

                if ((sample_rate == 48000)
                    || (sample_rate == 32000)
                    || (sample_rate == 16000)
                    || (sample_rate == 8000)) {
                    PSD_DAC_Handle->Init.ClockSource = PSD_DAC_CLOCK_SOURCE_24M_MODE;
                }
                else {
                    System_ClkConfig_t ClkConfig;
                    /* AUPLL clock = HSE_VALUE*N + HSE_VALUE*(K/D) */
                    /* AUPLL CLK Config */
                    ClkConfig.AUPLL_CFG.PowerEn = PLL_POWER_ENABLE;
                    ClkConfig.AUPLL_CFG.PLL_N = 7;
                    ClkConfig.AUPLL_CFG.PLL_K = 5264;
                    ClkConfig.AUPLL_CFG.PLL_D = 10000;
                    System_AUPLL_config(&ClkConfig.AUPLL_CFG, 1000);
                    psd_dac_pll_divider(8-1);
                    PSD_DAC_Handle->Init.ClockSource = PSD_DAC_CLOCK_SOURCE_AUPLL;
                }

                PSD_DAC_Handle->Init.DataSrc = PSD_DAC_DATA_SOURCE_DACFIFO;
                PSD_DAC_Handle->Init.Right_mix = PSD_DAC_FUNC_DISABLE;
                PSD_DAC_Handle->Init.Left_mix = PSD_DAC_FUNC_DISABLE;

                if(channels == 1)
                {
                    PSD_DAC_Handle->Init.MonoEn = PSD_DAC_FUNC_ENABLE;
                }else if(channels == 2)
                {
                    PSD_DAC_Handle->Init.MonoEn = PSD_DAC_FUNC_DISABLE;
                }else{
                    goto __err;
                }

                PSD_DAC_Handle->Init.Oversampling_Level = PSD_DAC_OVERSAMPLING_HIGH;
                PSD_DAC_Handle->Init.DAC_DataFormat = PSD_DAC_FIFO_FORMAT_16BIT;
                PSD_DAC_Handle->Init.DAC_FIFOCombine = PSD_DAC_FUNC_ENABLE;
                PSD_DAC_Handle->Init.DAC_FIFOThreshold = PSD_DAC_FIFO_HALF_DEPTH;

                psd_dac_init(PSD_DAC_Handle);
                __PSD_DAC_NORMAL_MODE_ENABLE();

                psd_dac_set_volume(PSD_DAC_CH_LR, 0x4000);
                psd_dac_int_enable(PSD_DAC_INT_DACFF_L_EMPTY | PSD_DAC_INT_DACFF_L_AEMPTY);

                PSD_DAC_Handle->DAC_FIFO_LeftEmpty_Callback = psd_dac_tx_callback;
                PSD_DAC_Handle->DAC_FIFO_LeftAlmostEmpty_Callback = psd_dac_tx_callback;
                /* insert new audio hw to list before enable interrupt */

                co_list_push_back(&audio_hw_env.hw_list, &tmp->hdr);
                NVIC_EnableIRQ(PSD_DAC_IRQn);
            }
            break;
        default:
            goto __err;
    }
    
    return tmp;
    
__err:
    if (tmp->hw_handle) {
        vPortFree(tmp->hw_handle);
    }
    if (tmp->pcm) {
        vPortFree(tmp->pcm);
    }
    if (tmp->pcm_out) {
        vPortFree(tmp->pcm_out);
    }
    co_list_extract(&audio_hw_env.hw_list, &tmp->hdr);
    vPortFree(tmp);

    return NULL;
}

void audio_hw_destroy(audio_hw_t *hw)
{
    if (hw == NULL) {
        return;
    }
    
    GLOBAL_INT_DISABLE();
    if (co_list_extract(&audio_hw_env.hw_list, &hw->hdr)) {
        switch (hw->type) {
            case AUDIO_HW_TYPE_I2S:
                i2s_deinit(hw->hw_handle);
                switch (hw->base_addr) {
                    case I2S0_BASE:
                        __SYSTEM_I2S0_CLK_DISABLE();
                        NVIC_DisableIRQ(I2S0_IRQn);
                        break;
                    case I2S1_BASE:
                        __SYSTEM_I2S1_CLK_DISABLE();
                        NVIC_DisableIRQ(I2S1_IRQn);
                        break;
                    default:
                        break;
                }
                break;
            case AUDIO_HW_TYPE_PDM:
                pdm_stop(hw->hw_handle);
                switch (hw->base_addr) {
                    case PDM0_BASE:
                        __SYSTEM_PDM0_CLK_DISABLE();
                        NVIC_DisableIRQ(PDM0_IRQn);
                        break;
                    case PDM1_BASE:
                        __SYSTEM_PDM1_CLK_DISABLE();
                        NVIC_DisableIRQ(PDM1_IRQn);
                        break;
                    default:
                        break;
                }
                break;
            case AUDIO_HW_TYPE_PSD_DAC:
                __SYSTEM_PSD_DAC_CLK_DISABLE();
                NVIC_DisableIRQ(PSD_DAC_IRQn);
                break;
            default:
                break;
        }
    }
    GLOBAL_INT_RESTORE();
    
    audio_hw_output_t *output;
    output = (void *)co_list_pop_front(&hw->output_list);
    while (output) {
        vPortFree(output);
        output = (void *)co_list_pop_front(&hw->output_list);
    }
    
    if (hw->hw_handle) {
        vPortFree(hw->hw_handle);
    }
    if (hw->pcm) {
        vPortFree(hw->pcm);
    }
    if (hw->pcm_out) {
        vPortFree(hw->pcm_out);
    }
    vPortFree(hw);
}

audio_hw_output_t *audio_hw_output_add(audio_hw_t *hw, audio_hw_receive_pcm_ntf_t handler)
{
    audio_hw_output_t *output;

    if (hw == NULL) {
        return NULL;
    }
    
    output = pvPortMalloc(sizeof(audio_hw_output_t));
    if (output) {
        GLOBAL_INT_DISABLE();
        output->audio_hw = hw;
        output->handler = handler;
        output->rd_ptr = hw->wr_ptr;
        co_list_push_back(&hw->output_list, &output->hdr);
        GLOBAL_INT_RESTORE();
    }
    
    return output;
}

void audio_hw_output_remove(audio_hw_t *hw, audio_hw_output_t *output)
{
    if (hw == NULL) {
        return;
    }

    GLOBAL_INT_DISABLE();
    co_list_extract(&hw->output_list, &output->hdr);
    GLOBAL_INT_RESTORE();
    
    vPortFree(output);
}

static void *copy_pcm(audio_hw_output_t *output, void *pcm, uint32_t samples, uint8_t channels)
{
    audio_hw_t *hw = output->audio_hw;

    if (channels == AUDIO_CHANNELS_MONO) {
        if (hw->channels == AUDIO_CHANNELS_MONO) {
            int16_t *src = (int16_t *)hw->pcm + output->rd_ptr;
            int16_t *dst = pcm;
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src++;
            }
            pcm = dst;
        }
        else {
            int16_t *src = (int16_t *)hw->pcm + (output->rd_ptr << 1);
            int16_t *dst = pcm;
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src;
                src += 2;
            }
            pcm = dst;
        }
    }
    else {
        if (hw->channels == AUDIO_CHANNELS_MONO) {
            int16_t *src = (int16_t *)hw->pcm + output->rd_ptr;
            int16_t *dst = pcm;
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src;
                *dst++ = *src++;
            }
            pcm = dst;
        }
        else {
            uint32_t *src = (uint32_t *)hw->pcm + output->rd_ptr;
            uint32_t *dst = pcm;
            for (uint32_t i=0; i<samples; i++) {
                *dst++ = *src++;
            }
            pcm = dst;
        }
    }
    
    return pcm;
}

uint32_t audio_hw_read_pcm(audio_hw_output_t *output, void *pcm, uint32_t samples, uint8_t channels)
{
    audio_hw_t *hw = output->audio_hw;
    uint32_t tail_samples;
    uint32_t read_samples = 0;

    tail_samples = hw->pcm_samples - output->rd_ptr;
    if (tail_samples <= samples) {
        pcm = copy_pcm(output, pcm, tail_samples, channels);
        output->rd_ptr = 0;
        samples -= tail_samples;
        read_samples = tail_samples;
    }
    if (samples) {
        pcm = copy_pcm(output, pcm, samples, channels);
        output->rd_ptr += samples;
        read_samples += samples;
    }

    return read_samples;
}

