/*
  ******************************************************************************
  * @file    driver_cali.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   Calibration module driver.
  *          This file provides firmware functions to calibrate RC frequency
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

#include "co_util.h"

/************************************************************************************
 * @fn      cali_IRQHandler
 *
 * @brief   Handle Calibration interrupt request.
 *
 * @param   hcali: Calibration handle.
 */
__WEAK void cali_IRQHandler(CALI_HandleTypeDef *hcali)
{
    __CALI_INT_CLR();

    if (hcali->DoneCallback) {
        hcali->DoneCallback(hcali, __CALI_RESULT_GET());
    }
}

/************************************************************************************
 * @fn      cali_init
 *
 * @brief   Initialize the calibration module
 *
 * @param   hcali: calibration handle.
 */
void cali_init(CALI_HandleTypeDef *hcali)
{
    __CALI_DISABLE();
    __CALI_INT_DISABLE();
    __CALI_UP_MODE_SET(hcali->mode);
    __CALI_CNT_SET(hcali->rc_cnt);
}

/************************************************************************************
 * @fn      cali_start
 *
 * @brief   start calibration with block mode, call cali_calc_rc_freq to calcuate RC
 *          frequency with calibration result.
 *
 * @param   hcali: calibration handle.
 *
 * @return  calibration result
 */
uint32_t cali_start(CALI_HandleTypeDef *hcali)
{
    uint32_t result;

    __CALI_ENABLE();
    
    while (__CALI_IS_DONE() == 0);
    result = __CALI_RESULT_GET();
    
    __CALI_DISABLE();
    
    return result;
}

/************************************************************************************
 * @fn      cali_start_IT
 *
 * @brief   start calibration with interrupt mode
 *
 * @param   hcali: calibration handle.
 */
void cali_start_IT(CALI_HandleTypeDef *hcali)
{
    __CALI_INT_ENABLE();
    __CALI_ENABLE();
}

/************************************************************************************
 * @fn      cali_stop
 *
 * @brief   stop on-going calibration
 *
 * @param   hcali: calibration handle.
 */
void cali_stop(CALI_HandleTypeDef *hcali)
{
    __CALI_DISABLE();
}

/************************************************************************************
 * @fn      cali_calc_rc_freq
 *
 * @brief   calculate RC frequency with calibrated result
 *
 * @param   hcali: calibration handle.
 *          cali_result: calibration result from cali_start or DoneCallback
 *
 * @return  calculate result
 */
uint32_t cali_calc_rc_freq(CALI_HandleTypeDef *hcali, uint32_t cali_result)
{
    uint32_t tmp_high,tmp_low;
    uint32_t lp_frequency;
    mul_64(&tmp_low, &tmp_high, 24000000, hcali->rc_cnt);
    lp_frequency = simple_div_64(tmp_low, tmp_high, cali_result);
    return lp_frequency;
}