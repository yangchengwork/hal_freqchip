/*
  ******************************************************************************
  * @file    driver_psd_dac.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   codec HAL module driver.
  *          This file provides firmware functions to manage the 
  *          PWM Sigma-Delta DAC for Codec.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      psd_dac_IRQHandler
 *
 * @brief   Handle PSD_DAC interrupt request.
 *
 * @param   huart: UART handle.
 */
__WEAK void psd_dac_IRQHandler(PSD_DAC_HandleTypeDef *hpsd)
{
    uint32_t lu32_Status = __PSD_DAC_GET_INT_STATUS();

    /* left channel empty */
    if (lu32_Status & PSD_DAC_INT_DACFF_L_EMPTY)
    {
        if (hpsd->DAC_FIFO_LeftEmpty_Callback)
        {
            hpsd->DAC_FIFO_LeftEmpty_Callback(hpsd);
        }
    }
    /* right channel empty */
    if (lu32_Status & PSD_DAC_INT_DACFF_R_EMPTY)
    {
        if (hpsd->DAC_FIFO_RightEmpty_Callback)
        {
            hpsd->DAC_FIFO_RightEmpty_Callback(hpsd);
        }
    }

    /* left channel almost empty */
    if (lu32_Status & PSD_DAC_INT_DACFF_L_AEMPTY)
    {
        if (hpsd->DAC_FIFO_LeftAlmostEmpty_Callback)
        {
            hpsd->DAC_FIFO_LeftAlmostEmpty_Callback(hpsd);
        }
    }
    /* right channel almost empty */
    if (lu32_Status & PSD_DAC_INT_DACFF_R_AEMPTY)
    {
        if (hpsd->DAC_FIFO_RightAlmostEmpty_Callback)
        {
            hpsd->DAC_FIFO_RightAlmostEmpty_Callback(hpsd);
        }
    }
}

/*********************************************************************
 * @fn      psd_dac_init
 *
 * @brief   Initializes the psd_dac according to the specified
 *          parameters in the PSD_DAC_HandleTypeDef.
 * @param   hpsd: psd dac handle.
 */
void psd_dac_init(PSD_DAC_HandleTypeDef *hpsd)
{
    __PSD_DAC_USB_MODE_ENABLE();

    /* Data source */
    __PSD_DAC_DATA_SRC(hpsd->Init.DataSrc);

    /* mono or stereo */
    if (hpsd->Init.MonoEn)
        __PSD_DAC_MONO_ENABLE();
    else
        __PSD_DAC_STEREO_ENABLE();

    /* channel mix */
    __PSD_DAC_LEFT_CHANNEL_MIX_CONFIG(hpsd->Init.Left_mix);
    __PSD_DAC_RIGHT_CHANNEL_MIX_CONFIG(hpsd->Init.Right_mix);

    /* clock source, sampling rate, oversampling level */
    __PSD_DAC_CLOCK_SRC(hpsd->Init.ClockSource);
    __PSD_DAC_SAMPLING_RATE(hpsd->Init.SampleRate);
    __PSD_DAC_OVERSAMPLING_RATE_MODE(hpsd->Init.Oversampling_Level);

    /* I2SRX */
    if (hpsd->Init.DataSrc)
    {
        /* master,slave */
        __PSD_DAC_I2S_MODE_CONFIG(hpsd->Init.I2S_Mode);
        /* Left channel Data copy to right channel config */
        __PSD_DAC_I2S_CHANNEL_COPY_CONFIG(hpsd->Init.I2S_ChannelCopy);
        /* foramt */
        __PSD_DAC_I2S_FORAMT(hpsd->Init.I2S_Standard);
        /* width */
        __PSD_DAC_I2S_DATA_WIDTH(hpsd->Init.I2S_DataFormat);

        /* master config bclk, lrclk */
        if (hpsd->Init.I2S_Mode)
        {
            __PSD_DAC_I2S_BCLK_SRC(hpsd->Init.BCLK_Source);

            /* BCLK */
            if (hpsd->Init.BCLK_DIV == 0)
                __PSD_DAC_I2S_BCLK_DIV(0);
            else
                __PSD_DAC_I2S_BCLK_DIV(hpsd->Init.BCLK_DIV - 1);

            /* LRCLK */
            if (hpsd->Init.LRCLK_DIV == 0)
                __PSD_DAC_I2S_LRCLK_DIV(0);
            else
                __PSD_DAC_I2S_LRCLK_DIV(hpsd->Init.LRCLK_DIV - 1);
        }
    }
    /* DACFIFO */
    else
    {
        /* DataFormat */
        __PSD_DAC_FIFO_SET_BITWD_LEFT(hpsd->Init.DAC_DataFormat);
        __PSD_DAC_FIFO_SET_BITWD_RIGHT(hpsd->Init.DAC_DataFormat);

        /* FIFO operation mode */
        if (hpsd->Init.DAC_DataFormat > PSD_DAC_FIFO_FORMAT_16BIT)
            __PSD_DAC_FIFO_OP_MODE(PSD_DAC_FUNC_DISABLE);
        else
            __PSD_DAC_FIFO_OP_MODE(hpsd->Init.DAC_FIFOCombine);

        if (hpsd->Init.DAC_FIFOThreshold > 0)
        {
            __PSD_DAC_FIFO_EMPTY_THRESHOLD_LEFT(hpsd->Init.DAC_FIFOThreshold);
            __PSD_DAC_FIFO_EMPTY_THRESHOLD_RIGHT(hpsd->Init.DAC_FIFOThreshold);
            __PSD_DAC_DMA_THRESHOLD_LEFT(hpsd->Init.DAC_FIFOThreshold);
            __PSD_DAC_DMA_THRESHOLD_RIGHT(hpsd->Init.DAC_FIFOThreshold);
        }
        /* default enable dma */
        __PSD_DAC_DMA_ENABLE();
    }

    /* IP clock enable step */
    __PSD_DAC_I2S_CLOCK_ENABLE();
    __PSD_DAC_DAC_CLOCK_ENABLE();
    __PSD_DAC_I2S_ENABLE();
    __PSD_DAC_DAC_ENABLE();
    __PSD_DAC_RESET();
}

/*********************************************************************
 * @fn      psd_dac_mute_contrl
 *
 * @brief   mute contrl.
 * 
 * @param   fe_Channel: mute channel select.
 *          This parameter can be one of the following values:
 *          @arg PSD_DAC_CH_L:  left channel mute.
 *          @arg PSD_DAC_CH_R:  right channel mute.
 *          @arg PSD_DAC_CH_LR: left and right channel mute.
 */
void psd_dac_mute_contrl(enum_PSD_DAC_Channel_t fe_Channel)
{
    __PSD_DAC_MUTE_CTRL(fe_Channel);
}

/*********************************************************************
 * @fn      psd_dac_set_volume
 *
 * @brief   set channel volume.
 * 
 * @param   fe_Channel: mute channel select.
 *          This parameter can be one of the following values:
 *          @arg PSD_DAC_CH_L:  set left channel volume.
 *          @arg PSD_DAC_CH_R:  set right channel volume.
 *          @arg PSD_DAC_CH_LR: set left and right channel volume.
 * @param   fu16_Volume: volume = 20*lg(fu32_Volume/2^12)
 */
void psd_dac_set_volume(enum_PSD_DAC_Channel_t fe_Channel, uint16_t fu16_Volume)
{
    if (fe_Channel == PSD_DAC_CH_L)
    {
        __PSD_DAC_SET_LEFT_VOLUME(fu16_Volume);
    }
    else if (fe_Channel == PSD_DAC_CH_R)
    {
        __PSD_DAC_SET_RIGHT_VOLUME(fu16_Volume);
    }
    else
    {
        __PSD_DAC_SET_LEFT_VOLUME(fu16_Volume);
        __PSD_DAC_SET_RIGHT_VOLUME(fu16_Volume);
    }
}

/*********************************************************************
 * @fn      psd_dac_set_dac_fifothreshold
 *
 * @brief   set DAC FIFO almost empty threshold.
 * 
 * @param   fu16_thr: This parameter can be a value 1 ~ 63.
 */
void psd_dac_set_dac_fifothreshold(uint16_t fu16_thr)
{
    if (fu16_thr > 63)
        fu16_thr = 63;

    __PSD_DAC_FIFO_EMPTY_THRESHOLD_LEFT(fu16_thr);
    __PSD_DAC_FIFO_EMPTY_THRESHOLD_RIGHT(fu16_thr);
    __PSD_DAC_DMA_THRESHOLD_LEFT(fu16_thr);
    __PSD_DAC_DMA_THRESHOLD_RIGHT(fu16_thr);
}

/*********************************************************************
 * @fn      psd_dac_set_samplerate
 *
 * @brief   sampling rate update.
 * 
 * @param   fe_rate: sampling rate.
 *          This parameter can be one of the following values:
 *          @arg PSD_DAC_SAMPLE_RATE_8000:   8K
 *          @arg PSD_DAC_SAMPLE_RATE_12000:  12K
 *          @arg PSD_DAC_SAMPLE_RATE_16000:  16K
 *          @arg PSD_DAC_SAMPLE_RATE_24000:  24K
 *          @arg PSD_DAC_SAMPLE_RATE_32000:  32K
 *          @arg PSD_DAC_SAMPLE_RATE_48000:  48K
 *          @arg PSD_DAC_SAMPLE_RATE_96000:  96K
 *          @arg PSD_DAC_SAMPLE_RATE_192000: 192K
 *          @arg PSD_DAC_SAMPLE_RATE_11025:  11.025K
 *          @arg PSD_DAC_SAMPLE_RATE_22050:  22.050K
 *          @arg PSD_DAC_SAMPLE_RATE_44100:  44.1K
 *          @arg PSD_DAC_SAMPLE_RATE_88200:  88.2K
 *          @arg PSD_DAC_SAMPLE_RATE_176400: 176.4K
 */
void psd_dac_set_samplerate(enum_PSD_DAC_SampleRate_t fe_rate)
{
    __PSD_DAC_SAMPLING_RATE(fe_rate);
}

/*********************************************************************
 * @fn      psd_dac_set_dataformat
 *
 * @brief   dac Data bit width update.
 * 
 * @param   fe_format: Data bit width.
 *          This parameter can be one of the following values:
 *          @arg PSD_DAC_FIFO_FORMAT_8BIT:  8bit
 *          @arg PSD_DAC_FIFO_FORMAT_16BIT: 16bit
 *          @arg PSD_DAC_FIFO_FORMAT_20BIT: 20bit
 *          @arg PSD_DAC_FIFO_FORMAT_24BIT: 24bit
 */
void psd_dac_set_dataformat(enum_PSD_DAC_DacFIFO_Format_t fe_format)
{
    /* DataFormat */
    __PSD_DAC_FIFO_SET_BITWD_LEFT(fe_format);
    __PSD_DAC_FIFO_SET_BITWD_RIGHT(fe_format);

    /* FIFO operation mode */
    if (fe_format > PSD_DAC_FIFO_FORMAT_16BIT)
        __PSD_DAC_FIFO_OP_MODE(PSD_DAC_FUNC_DISABLE);
}

/*********************************************************************
 * @fn      psd_dac_set_gain_compensate
 *
 * @brief   set gain compensate.
 * 
 * @param   fu16_comp: gain compensate value.
 */
void psd_dac_set_gain_compensate(uint16_t fu16_comp)
{
    if (fu16_comp > 0xFFF)
        fu16_comp = 0xFFF;

    __PSD_DAC_SET_GAIN_COMPENSATE(fu16_comp);
}

/*********************************************************************
 * @fn      psd_dac_pll_divider
 *
 * @brief   set pll divider.
 * 
 * @param   fu8_div:  value.
 */
void psd_dac_pll_divider(uint8_t fu8_div)
{
    if (fu8_div > 0x3F)
        fu8_div = 0x3F;

    __PSD_DAC_SET_AUPLL_DIV(fu8_div);
}

/*********************************************************************
 * @fn      psd_dac_i2s_mclk_config
 *
 * @brief   MCLK output config.
 * 
 * @param   fu16_comp: gain compensate value.
 */
void psd_dac_i2s_mclk_config(struct_PSD_DAC_MCLKConfig_t *sConfig)
{
    /* mclk source select */
    __PSD_DAC_MCLK_SOURCE_SELECT(sConfig->MCLK_Source);
    /* mclk div type */
    __PSD_DAC_MCLK_DIV_TYPE(sConfig->MCLK_DIV_Type);
    /* mclk div */
    __PSD_DAC_MCLK_DIV(sConfig->MCLK_DIV);
}

/*********************************************************************
 * @fn      psd_dac_i2s_mclk_out_enable/disable
 *
 * @brief   MCLK output enable/disable When Data source select I2S.
 * 
 * @param   fu16_comp: gain compensate value.
 */
void psd_dac_i2s_mclk_out_enable(void)
{
    __PSD_DAC_MCLK_ENABLE();
}
void psd_dac_i2s_mclk_out_disable(void)
{
    __PSD_DAC_MCLK_DISABLE();
}

/************************************************************************************
 * @fn      psd_dac_int_enable/disable
 *
 * @brief   interrupt enable/disable.
 *
 * @param   fe_Int_Status: interrupt source select
 */
void psd_dac_int_enable(enum_PSD_DAC_INT_Status_t fe_Int_Status)
{
    __PSD_DAC_INT_STATUS_ENABLE(fe_Int_Status);
}
void psd_dac_int_disable(enum_PSD_DAC_INT_Status_t fe_Int_Status)
{
    __PSD_DAC_INT_STATUS_DISABLE(fe_Int_Status);
}

/************************************************************************************
 * @fn      psd_dac_int_get_status/raw status
 *
 * @brief   get interrupt status/raw status.
 */
uint32_t psd_dac_int_get_status(void)
{
    return __PSD_DAC_GET_INT_STATUS();
}
uint32_t psd_dac_int_get_raw_status(void)
{
    return __PSD_DAC_GET_INT_RAW_STATUS();
}

/************************************************************************************
 * @fn      psd_dac_write_dac_fifo_left/
 *          psd_dac_write_dac_fifo_right/
 *          psd_dac_write_dac_fifo_left_right/
 *
 * @brief   write dac fifo LR.
 *
 * @param   fp_data: dac data data buffer.
 * @param   fu32_length: read data length.
 */
void psd_dac_write_dac_fifo_left(void *fp_data, uint32_t fu32_length)
{
    union_PSD_DACData_t Data;

    switch (__PSD_DAC_FIFO_GET_BITWD_LEFT())
    {
        case PSD_DAC_FIFO_FORMAT_8BIT:
        {
            Data.p_u8 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u8++);
        }break;

        case PSD_DAC_FIFO_FORMAT_16BIT:
        {
            Data.p_u16 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u16++);
        }break; 

        case PSD_DAC_FIFO_FORMAT_20BIT:
        case PSD_DAC_FIFO_FORMAT_24BIT:
        {
            Data.p_u32 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u32++);
        }break; 

        default:break;
    }
}
void psd_dac_write_dac_fifo_right(void *fp_data, uint32_t fu32_length)
{
    union_PSD_DACData_t Data;

    switch (__PSD_DAC_FIFO_GET_BITWD_RIGHT())
    {
        case PSD_DAC_FIFO_FORMAT_8BIT:
        {
            Data.p_u8 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u8++);
        }break;

        case PSD_DAC_FIFO_FORMAT_16BIT:
        {
            Data.p_u16 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u16++);
        }break; 

        case PSD_DAC_FIFO_FORMAT_20BIT:
        case PSD_DAC_FIFO_FORMAT_24BIT:
        {
            Data.p_u32 = fp_data;
            while(fu32_length--)
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u32++);
        }break; 

        default:break;
    }
}
void psd_dac_write_dac_fifo_left_right(void *fp_data, uint32_t fu32_length)
{
    union_PSD_DACData_t Data;

    switch (__PSD_DAC_FIFO_GET_BITWD_RIGHT())
    {
        case PSD_DAC_FIFO_FORMAT_8BIT:
        {
            Data.p_u8 = fp_data;
            while(fu32_length--)
            {
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u8++);
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u8++);
            }
        }break;

        case PSD_DAC_FIFO_FORMAT_16BIT:
        {
            Data.p_u16 = fp_data;
            while(fu32_length--)
            {
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u16++);
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u16++);
            }
        }break; 

        case PSD_DAC_FIFO_FORMAT_20BIT:
        case PSD_DAC_FIFO_FORMAT_24BIT:
        {
            Data.p_u32 = fp_data;
            while(fu32_length--)
            {
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u32++);
                __PSD_DAC_WRITE_FIFO_DATA_RIGHT(*Data.p_u32++);
            }
        }break; 

        default:break;
    }
}

/************************************************************************************
 * @fn      psd_dac_write_dac_fifo_Combine
 *
 * @brief   when use 8bit/16bit data Format, data can be merged into the left channel.
 *
 * @param   fp_data: dac data data buffer.
 * @param   fu32_size: read data length.
 */
bool psd_dac_write_dac_fifo_Combine(void *fp_data, uint32_t fu32_length)
{
    union_PSD_DACData_t Data;

    if (__PSD_DAC_FIFO_GET_BITWD_RIGHT() >= PSD_DAC_FIFO_FORMAT_20BIT)
        return -1;
    if (__PSD_DAC_FIFO_GET_BITWD_LEFT() >= PSD_DAC_FIFO_FORMAT_20BIT)
        return -1;
    
    switch (__PSD_DAC_FIFO_GET_BITWD_RIGHT())
    {
        case PSD_DAC_FIFO_FORMAT_8BIT:
        {
            Data.p_u16 = fp_data;
            while(fu32_length--)
            {
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u16++);
            }
        }break;

        case PSD_DAC_FIFO_FORMAT_16BIT:
        {
            Data.p_u32 = fp_data;
            while(fu32_length--)
            {
                __PSD_DAC_WRITE_FIFO_DATA_LEFT(*Data.p_u32++);
            }
        }break; 

        default:break;
    }

    return 0;
}
