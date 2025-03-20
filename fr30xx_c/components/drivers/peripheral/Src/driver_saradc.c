/*
  ******************************************************************************
  * @file    driver_saradc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   ADC module driver.
  *          This file provides firmware functions to manage the 
  *          successive approximati ADC peripheral.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/******************************************************************************
 * @fn      saradc_init
 *
 * @brief   Initialize the ADC according to the specified parameters
 *          in the saradc_InitConfig_t. 
 *
 * @param   Init : saradc_InitConfig_t structure that contains the
 *                 configuration information for ADC module.
 */
void saradc_init(saradc_InitConfig_t *Init)
{
    /* saradc power */
    __SARADC_ANALOG_POWER_ENABLE();
    __SARADC_ANALOG_RESET();
    __SARADC_ANALOG_RESET_STOP();
    while(__SARADC_IS_ANALOG_RESET());
    /* saradc reset */
    __SARADC_RESET();
    __SARADC_FIFO_RESET();

    /* if use internal reference source, enable BUF */
    if (Init->saradc_reference >= SARADC_REF_INTERNAL_1P2V)
    {
        __SARADC_INTERNAL_REFERENCE_BUF_ENABLE();
        __SARADC_INTERNAL_REFERENCE_VOLTAGE_CONFIG(Init->saradc_reference - SARADC_REF_INTERNAL_1P2V);
    }
    /* reference source select */
    __SARADC_ANALOG_REFERENCE_SOURCE_SELECT(Init->saradc_reference & 0x3);
    /* measure input enable */
    __SARADC_INPUT_CHANNEL_ENABLE();
    /* saradc sampling clock divider */
    if (Init->saradc_clock_div > 254)
        __SARADC_SAMPLING_CLOCK_DIV_CONFIG(254/2);
    else if (Init->saradc_clock_div > 2)
        __SARADC_SAMPLING_CLOCK_DIV_CONFIG(Init->saradc_clock_div/2);
    else 
        __SARADC_SAMPLING_CLOCK_DIV_CONFIG(1);

    /* saradc interval clock divider */
    if (Init->saradc_interval_clock_div == 0)
        __SARADC_INTERVAL_CLOCK_DIV_CONFIG(1);
    else
        __SARADC_INTERVAL_CLOCK_DIV_CONFIG(Init->saradc_interval_clock_div);
    /* saradc measure voltage divider config */
    __SARADC_MEASURE_VOLTAGE_DIVIDER_CONFIG(Init->saradc_voltage_divider);
    /* saradc work mode */
    __SARADC_WORK_MODE_SELECT(Init->saradc_mode);

    /* Timing */
    if (Init->saradc_sampling_cycle > 15)
        __SARADC_ANALOG_SAMPLING_TIME_CONFIG(0xF);
    else if (Init->saradc_sampling_cycle > 4)
        __SARADC_ANALOG_SAMPLING_TIME_CONFIG(Init->saradc_sampling_cycle - 1);
    else
        __SARADC_ANALOG_SAMPLING_TIME_CONFIG(3);
    
    __SARADC_SAMPLING_TIMEOUT_CONFIG(0xF);
    __SARADC_CONVERT_TIMEOUT_CONFIG(0xF);
    __SARADC_HOLDTIME_LENGTH_CONFIG(6);
}

/******************************************************************************
 * @fn      sar_adc_loop_config
 *
 * @brief   loop mode config
 *
 * @param   sConfig : saradc_LoopModeConfig_t structure that contains the
 *                    configuration information for ADC loop mode.
 */
bool saradc_loop_config(saradc_LoopConfig_t *sConfig)
{
    if (__SARADC_GET_WORK_MODE() == SARADC_QUEUE_MODE)
        return -1;

    /* loop mode trigger mode */
    __SARADC_TRIGGER_MODE(sConfig->loop_triggerMode);
    /* loop mode max channel */
    if (sConfig->loop_max_channel != 0)
        __SARADC_MAX_CHANNEL_CONFIG(sConfig->loop_max_channel - 1);
    /* loop mode interval */
    __SARADC_LOOP_INTERVAL_LENGTH(sConfig->loop_interval);

    if (sConfig->loop_FIFO_enable)
    {
        /* FIFO,DMA config */
        __SARADC_FIFO_ENABLE();
        __SARADC_FIFO_CHANNEL_INDEX_ENABLE();
        __SARADC_FIFO_SELECT_CHANNEL(sConfig->loop_FIFO_channel_sel);
        if (sConfig->loop_FIFO_almost_threshold >= 64)
            __SARADC_FIFO_THRESHOLD_CONFIG(64);
        else if (sConfig->loop_FIFO_almost_threshold >= 63)
            __SARADC_FIFO_THRESHOLD_CONFIG(63);
        else if (sConfig->loop_FIFO_almost_threshold > 0)
            __SARADC_FIFO_THRESHOLD_CONFIG(sConfig->loop_FIFO_almost_threshold - 1);
        else
            __SARADC_FIFO_THRESHOLD_CONFIG(sConfig->loop_FIFO_almost_threshold);

        __SARADC_DMA_ENABLE();
        if (sConfig->loop_FIFO_almost_threshold >= 64)
            __SARADC_DMA_THRESHOLD_CONFIG(64);
        else
            __SARADC_DMA_THRESHOLD_CONFIG(sConfig->loop_FIFO_almost_threshold);
    }
    else
    {
        __SARADC_FIFO_DISABLE();
    }

    return 0;
}

/******************************************************************************
 * @fn      saradc_loop_convert_start/stop
 *
 * @brief   loop mode hardware trigger start/stop. 
 */
void saradc_loop_convert_start(void)
{
    __SARADC_CONVERT_START();
}
void saradc_loop_convert_stop(void)
{
    __SARADC_CONVERT_STOP();
    __SARADC_RESET();
}

/******************************************************************************
 * @fn      saradc_loop_software_trigger_convert
 *
 * @brief   loop mode software trigger start/stop. 
 */
void saradc_loop_software_trigger_convert(void)
{
    __SARADC_CONVERT_START();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __SARADC_SOFT_TRIGGER_CONVERT();
}

/******************************************************************************
 * @fn      saradc_wdt_config
 *
 * @brief   saradc wdt config.
 * 
 * @param   fe_wdt : saradc wdt select.
 * @param   sConfig : saradc wdt config info.
 */
void saradc_wdt_config(enum_saradc_wdt_t fe_wdt, saradc_WdtConfig_t *sConfig)
{
    if (fe_wdt == SARADC_WDT0)
    {
        /* saradc wdt mode config */
        __SARADC_WDT0_MODE_CONFIG(sConfig->saradc_WDT_mode);
        /* saradc wdt monitor channel config */
        __SARADC_WDT0_SET_MONITOR_CHANNEL(sConfig->saradc_WDT_monitor_channel);
        /* saradc wdt channel trigger threshold */
        if (sConfig->saradc_WDT_trigger_threshold == 0)
            __SARADC_WDT0_TRIGGER_THRESHOLD(0x0);
        else if (sConfig->saradc_WDT_trigger_threshold <= 16)
            __SARADC_WDT0_TRIGGER_THRESHOLD(sConfig->saradc_WDT_trigger_threshold - 1);
        else
            __SARADC_WDT0_TRIGGER_THRESHOLD(0xF);
        /* saradc wdt min/max limit value */
        __SARADC_WDT0_MIN_LIMIT(sConfig->saradc_WDT_min_limit);
        __SARADC_WDT0_MAX_LIMIT(sConfig->saradc_WDT_max_limit);
    }
    else
    {
        /* saradc wdt mode config */
        __SARADC_WDT1_MODE_CONFIG(sConfig->saradc_WDT_mode);
        /* saradc wdt monitor channel config */
        __SARADC_WDT1_SET_MONITOR_CHANNEL(sConfig->saradc_WDT_monitor_channel);
        /* saradc wdt channel trigger threshold */
        if (sConfig->saradc_WDT_trigger_threshold == 0)
            __SARADC_WDT1_TRIGGER_THRESHOLD(0x0);
        else if (sConfig->saradc_WDT_trigger_threshold <= 16)
            __SARADC_WDT1_TRIGGER_THRESHOLD(sConfig->saradc_WDT_trigger_threshold - 1);
        else
            __SARADC_WDT1_TRIGGER_THRESHOLD(0xF);
        /* saradc wdt min/max limit value */
        __SARADC_WDT1_MIN_LIMIT(sConfig->saradc_WDT_min_limit);
        __SARADC_WDT1_MAX_LIMIT(sConfig->saradc_WDT_max_limit);
    }
}

/******************************************************************************
 * @fn      saradc_wdt_enable/disable
 *
 * @brief   get the number of data in the fifo.
 */
void saradc_wdt_enable(enum_saradc_wdt_t fe_wdt)
{
    if (fe_wdt == SARADC_WDT0)
        __SARADC_WDT0_ENABLE();
    else
        __SARADC_WDT1_ENABLE();
}
void saradc_wdt_disable(enum_saradc_wdt_t fe_wdt)
{
    if (fe_wdt == SARADC_WDT0)
        __SARADC_WDT0_DISABLE();
    else
        __SARADC_WDT1_DISABLE();
}

/******************************************************************************
 * @fn      saradc_queue_config
 *
 * @brief   saradc queue config.
 * 
 * @param   fe_queue : saradc queue select. 
 * @param   sConfig : saradc queue config info.
 */
bool saradc_queue_config(enum_saradc_queue_t fe_queue, saradc_QueueConfig_t *sConfig)
{
    if (__SARADC_GET_WORK_MODE() == SARADC_LOOP_MODE)
        return -1;

    int i;

    uint32_t TempValue;

    typedef struct 
    {
        uint32_t Queuex_Config;
        uint32_t Queuex_ChannelSel0;
        uint32_t Queuex_ChannelSel1;
    }str_queue_t;

    str_queue_t *Queue;

    switch (fe_queue)
    {
        case SARADC_QUEUE0: Queue = (str_queue_t *)&SARADC->SARADC_Queue0_Config; break;
        case SARADC_QUEUE1: Queue = (str_queue_t *)&SARADC->SARADC_Queue1_Config; break;
        case SARADC_QUEUE2: Queue = (str_queue_t *)&SARADC->SARADC_Queue2_Config; break;
        default:break;
    }

    /* Queuex Config value */
    #define QUEUE_MODE_POS     (1)
    #define QUEUE_CH_POS       (4)
    #define QUEUE_CYCLE_POS    (8)
    TempValue = sConfig->saradc_queue_mode << QUEUE_MODE_POS;
    if (sConfig->saradc_queue_channels != 0)
        TempValue |= ((sConfig->saradc_queue_channels - 1) << QUEUE_CH_POS);
    TempValue |= sConfig->saradc_queue_cycle << QUEUE_CYCLE_POS;
    /* Write Queuex Config register */
    Queue->Queuex_Config = TempValue;

    /* Channel Select0 */
    TempValue = 0;
    for(int i = 0; i < sConfig->saradc_queue_channels; i++)
    {
        TempValue |= sConfig->saradc_queue_chx_select[i] << (i*4);
    }
    Queue->Queuex_ChannelSel0 = TempValue;

    /* Channel Select1 */
    if (sConfig->saradc_queue_channels > 8)
    {
        TempValue = 0;
        for(int i = 0; i < sConfig->saradc_queue_channels-8; i++)
        {
            TempValue |= sConfig->saradc_queue_chx_select[i+8] << (i*4);
        }
        Queue->Queuex_ChannelSel1 = TempValue;
    }

    return 0;
}

/******************************************************************************
 * @fn      saradc_queue_enable-/disable
 *
 * @brief   get the number of data in the fifo.
 */
void saradc_queue_enable(enum_saradc_queue_t fe_queue)
{
    __SARADC_CONVERT_START();

    switch (fe_queue)
    {
        case SARADC_QUEUE0: __SARADC_QUEUE0_ENABLE(); break;
        case SARADC_QUEUE1: __SARADC_QUEUE1_ENABLE(); break;
        case SARADC_QUEUE2: __SARADC_QUEUE2_ENABLE(); break;
        default:break;
    }
}
void saradc_queue_disable(enum_saradc_queue_t fe_queue)
{
    switch (fe_queue)
    {
        case SARADC_QUEUE0: __SARADC_QUEUE0_DISABLE(); break;
        case SARADC_QUEUE1: __SARADC_QUEUE1_DISABLE(); break;
        case SARADC_QUEUE2: __SARADC_QUEUE2_DISABLE(); break;
        default:break;
    }
}

/******************************************************************************
 * @fn      saradc_get_fifo_count
 *
 * @brief   get the number of data in the fifo.
 */
uint16_t saradc_get_fifo_count(void)
{
    return __SARADC_GET_FIFO_COUNT();
}
/******************************************************************************
 * @fn      saradc_get_fifo_data
 *
 * @brief   get data in the fifo.
 */
void saradc_get_fifo_data(uint16_t *fp_data, uint32_t fu32_length)
{
    while(fu32_length--)
    {
        *fp_data++ = __SARADC_GET_FIFO_DATA();
    }
}

/******************************************************************************
 * @fn      adc_get_channel_valid_status
 *
 * @brief   get Channel valid status.
 * 
 * @param   fe_channel : channel number(0 ~ 15)
 */
bool saradc_get_channel_valid_status(enum_saradc_channel_t fe_channel)
{
    return (__SARADC_GET_CHANNEL_VALID_STATUS() & (1 << fe_channel)) ? true : false;
}

/******************************************************************************
 * @fn      saradc_get_channel_data
 *
 * @brief   get logic Channel convert Data.
 * @param   fe_channel : channel number(0 ~ 15)
 */
uint16_t saradc_get_channel_data(enum_saradc_channel_t fe_channel)
{
    return __SARADC_GET_CHANNEL_DATA(fe_channel);
}

/******************************************************************************
 * @fn      saradc_channel_single_config
 *
 * @brief   logic Channel config. single-ended mode.
 * @param   fe_channel : channel number(0 ~ 15)
 * @param   fe_Map     : channel map select.
 */
void saradc_channel_single_config(enum_saradc_channel_t fe_channel, enum_ADC_Channel_Map_t fe_Map)
{
    /* differential mode disable */
    __SARADC_CHANNEL_DIFFERENCE_DISABLE(fe_channel);
    /* MAP P config */
    __SARADC_CHANNEL_MAP_P_CONFIG(fe_channel, fe_Map);
}

/******************************************************************************
 * @fn      saradc_channel_differential_config
 *
 * @brief   logic Channel config. differential mode.
 * @param   fe_channel : channel number(0 ~ 15)
 * @param   fe_Map_P   : difference Positive channel map select.
 * @param   fe_Map_N   : difference negative channel map select.
 */
void saradc_channel_differential_config(enum_saradc_channel_t fe_channel, enum_ADC_Channel_Map_t fe_Map_P, enum_ADC_Channel_Map_t fe_Map_N)
{
    /* differential mode enable */
    __SARADC_CHANNEL_DIFFERENCE_ENABLE(fe_channel);
    /* MAP A/B config */
    __SARADC_CHANNEL_MAP_P_CONFIG(fe_channel, fe_Map_P);
    __SARADC_CHANNEL_MAP_N_CONFIG(fe_channel, fe_Map_N);
}

/******************************************************************************
 * @fn      saradc_vbe_measure_enable
 *
 * @brief   The vbe measurement circuit enable.
 */
void saradc_vbe_measure_enable(void)
{
    pmu_adc_vbe_power_ctrl(true);
    __SARADC_VBE_MEASURE_CONFIG();
}
void saradc_vbe_measure_disable(void)
{
    pmu_adc_vbe_power_ctrl(false);
}

/******************************************************************************
 * @fn      saradc_vbat_measure_enable
 *
 * @brief   The vbat measurement circuit enable and config vbat divider.
 * @param   fe_VbatCfg : vbat divider select.
 */
void saradc_vbat_measure_enable(enum_saradc_vbat_ctrl_t fe_VbatCfg)
{
    pmu_adc_vbat_power_ctrl(true);
    __SARADC_VBAT_MEASURE_CONFIG(fe_VbatCfg);
}
void saradc_vbat_measure_disable(void)
{
    pmu_adc_vbat_power_ctrl(false);
    __SARADC_VBAT_MEASURE_CONFIG(SARADC_VABT_POWERDOWN);
}

/******************************************************************************
 * @fn      saradc_mic_bias_config
 *
 * @brief   mic bias config.
 * @param   fe_micbias : vbat output voltage select.
 */
void saradc_mic_bias_output_enable(enum_saradc_micbias_ctrl_t fe_micbias)
{
    pmu_adc_micbias_power_ctrl(true);
    __SARADC_MIC_BIAS_CONFIG(fe_micbias);
}
void saradc_mic_bias_output_disable(void)
{
    pmu_adc_micbias_power_ctrl(false);
    __SARADC_MIC_BIAS_CONFIG(SARADC_MICBIAS_OUTPUT_POWERDOWN);
}

/******************************************************************************
 * @fn      saradc_pga_enable/disable
 */
void saradc_pga_enable(void)
{
    __SARADC_PGA0_POWER_ENABLE();
    __SARADC_PGA1_POWER_ENABLE();
    __SARADC_PGA2_POWER_ENABLE();

    __SARADC_PGA1I_TO_PGA2_EN(1);
    __SARADC_PGA1O_TO_PGA2_EN(0);
    __SARADC_PGA1_GAIN_CONFIG(0);
    __SARADC_PGA2_GAIN_CONFIG(8);

    __SARADC_PGA1_OP_OSTG_IS_CONFIG(3);
    __SARADC_PGA2_OP_OSTG_IS_CONFIG(3);
    __SARADC_PGA1_SW_CONFIG(0);
    __SARADC_PGA2_SW_CONFIG(5);
}
void saradc_pga_disable(void)
{
    __SARADC_PGA0_POWER_DISABLE();
    __SARADC_PGA1_POWER_DISABLE();
    __SARADC_PGA2_POWER_DISABLE();
}

/******************************************************************************
 * @fn      saradc_gain_enable/disable
 */
void saradc_gain_enable(void)
{
    __SARADC_GAIN_ENABLE();
    __SARADC_GAIN_DEN_CONFIG(1);
    __SARADC_GAIN_NOM_CONFIG(2);
    __SARADC_GAIN_DC_CONFIG(0x800);
}
void saradc_gain_disable(void)
{
    __SARADC_GAIN_DISABLE();
}

/******************************************************************************
 * @fn      saradc_int_enable
 *
 * @brief   saradc interrupt enable.
 */
void saradc_int_enable(enum_saradc_int_status_t fe_int)
{
    __SARADC_INT_ENABLE(fe_int);
}
/******************************************************************************
 * @fn      saradc_int_disable
 *
 * @brief   saradc interrupt disable.
 */
void saradc_int_disable(enum_saradc_int_status_t fe_int)
{
    __SARADC_INT_DISABLE(fe_int);
}
/******************************************************************************
 * @fn      saradc_int_status_clear
 *
 * @brief   saradc interrupt status clear.
 */
void saradc_int_status_clear(enum_saradc_int_status_t fe_int)
{
    __SARADC_CLR_INT_STATUS(fe_int);
}
/******************************************************************************
 * @fn      saradc_get_int_status
 *
 * @brief   saradc get interrupt status.(int Enbale & int raw status)
 */
uint32_t saradc_get_int_status(void)
{
    return __SARADC_GET_INT_STATUS();
}
/******************************************************************************
 * @fn      saradc_get_int_raw_status
 *
 * @brief   saradc get interrupt raw status.
 */
uint32_t saradc_get_int_raw_status(void)
{
    return __SARADC_GET_INT_RAW_STATUS();
}
