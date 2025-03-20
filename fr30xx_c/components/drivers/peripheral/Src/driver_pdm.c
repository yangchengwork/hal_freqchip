/*
  ******************************************************************************
  * @file    driver_pdm.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   pdm module driver.
  *          This file provides firmware functions to manage the 
  *          pulse-duration modulation (PDM) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      pdm_IRQHandler
 *
 * @brief   Handle PDM interrupt request.
 *
 * @param   hpdm: PDM handle.
 */
__WEAK void pdm_IRQHandler(PDM_HandleTypeDef *hpdm)
{
    if (__PDM_IS_FIFO_ALMOST_FULL(hpdm->PDMx)) 
    {
        if (hpdm->RxCallback) {
            hpdm->RxCallback(hpdm);
        }
    }
}

/************************************************************************************
 * @fn      pdm_init
 *
 * @brief   Initialize the PDM according to the specified parameters in the struct_PDMInit_t
 *
 * @param   hpdm: PDM handle.
 */
void pdm_init(PDM_HandleTypeDef *hpdm)
{
    /* Clock default use 24M */
    hpdm->PDMx->Config.USB_MODE = 1;
    /* Sample Rate */
    hpdm->PDMx->Config.SAMPLE_RATE = hpdm->Init.SampleRate;
    /* Over Samplek Mode */
    hpdm->PDMx->Config.OSR_MODE = hpdm->Init.OverSampleMode;
    /* channel config */
    if (hpdm->Init.ChannelMode == PDM_STEREO) {
        hpdm->PDMx->Config.MONO    = 0;
        hpdm->PDMx->Config.CH_SEL  = 0;
    }
    else{
        hpdm->PDMx->Config.MONO    = 1;
        hpdm->PDMx->Config.CH_SEL  = hpdm->Init.ChannelMode;
    }
    
    /* fifo almost Full Threshold */
    hpdm->PDMx->FF_AFLL_LVL = hpdm->Init.FIFO_FullThreshold;
    
    pdm_vol_set(hpdm, hpdm->Init.Volume);

    hpdm->PDMx->Config.RST = 1;
    hpdm->PDMx->Config.RST = 0;
    hpdm->PDMx->FF_RST.rst = 0x07;
    hpdm->PDMx->FF_RST.rst = 0x00;
}

/************************************************************************************
 * @fn      pdm_start_IT
 *
 * @brief   PDM read start in interrupt mode.
 *
 * @param   hpdm: PDM handle.
 * @param   fp_Data: buffer used to store received data, If fp_Data is NULL, application layer
 *                   should take charge of reading data from fifo.
 */
void pdm_start_IT(PDM_HandleTypeDef *hpdm, void *fp_Data)
{
    if (hpdm->b_RxBusy)
        return;

    __PDM_CLK_ENABLE(hpdm->PDMx);
    __PDM_ENABLE(hpdm->PDMx);

    __PDM_FIFO_ALMOST_FULL_INT_ENABLE(hpdm->PDMx);

    hpdm->b_RxBusy = true;
    hpdm->p_RxData = fp_Data;
}

/************************************************************************************
 * @fn      pdm_init
 *
 * @brief   Initialize the PDM according to the specified parameters in the struct_PDMInit_t
 *
 * @param   hpdm: PDM handle.
 * @param   vol: target volume, unit is dB
 */
void pdm_vol_set(PDM_HandleTypeDef *hpdm, int8_t vol)
{
    uint32_t vol_cfg;

    if(vol <= 18) {
        vol_cfg = PDM_VOL_dB(vol);
        hpdm->Init.Volume = vol;
    }
    else {
        return;
    }
    hpdm->PDMx->VOL_L = vol_cfg;
    hpdm->PDMx->VOL_R = vol_cfg;
}

/************************************************************************************
 * @fn      pdm_read_data
 *
 * @brief   read data from PDM fifo, read size is FIFO_FullThreshold
 *
 * @param   hpdm: PDM handle.
 * @param   pcm: buffer used to store received data.
 */
void pdm_read_data(PDM_HandleTypeDef *hpdm, void *fp_Data)
{
    uint8_t thd = hpdm->Init.FIFO_FullThreshold;

    if (hpdm->Init.ChannelMode == PDM_STEREO) 
    {
        uint32_t *p_u32 = (void *)fp_Data;
        while (thd--) {
            *p_u32++ = hpdm->PDMx->DATA;
        }
    }
    else 
    {
        uint16_t *p_u16 = (void *)fp_Data;
        while (thd--) {
            *p_u16++ = hpdm->PDMx->DATA;
        }
    }
}

/************************************************************************************
 * @fn      pdm_start
 *
 * @brief   PDM sampling start.
 *
 * @param   hpdm: PDM handle.
 */
void pdm_start(PDM_HandleTypeDef *hpdm)
{
    __PDM_CLK_ENABLE(hpdm->PDMx);
    __PDM_ENABLE(hpdm->PDMx);
}

/************************************************************************************
 * @fn      pdm_stop
 *
 * @brief   PDM sampling stop.
 *
 * @param   hpdm: PDM handle.
 */
void pdm_stop(PDM_HandleTypeDef *hpdm)
{
    __PDM_DISABLE(hpdm->PDMx);
    __PDM_CLK_DISABLE(hpdm->PDMx);

    hpdm->b_RxBusy = false;
}
