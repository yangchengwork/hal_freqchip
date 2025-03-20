/*
  ******************************************************************************
  * @file    driver_tick.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2021
  * @brief   Tick module driver.
  *          This file provides firmware functions to manage the System Tick peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/

#include "fr30xx.h"

/************************************************************************************
 * @fn      tick_IRQHandler
 *
 * @brief   Handle Tick interrupt request.
 *
 * @param   htick: Tick handle.
 */
void tick_IRQHandler(TICK_HandleTypeDef *htick)
{
    uint32_t status = Tick->INT_STA.Word;

    /* system wake up from sleep */
    if (status & TICK_INT_TYPE_SLP) {
        if ((htick) && (htick->TickSleepCallback)) {
            htick->TickSleepCallback();
        }
        Tick->INT_STA.Bits.SLP = 1;
    }
    /* tick reach target value */
    if (status & TICK_INT_TYPE_TGT) {
        if ((htick) && (htick->TickTargetCallback)) {
            htick->TickTargetCallback();
        }
        Tick->INT_STA.Bits.TGT = 1;
        Tick->INT_CTL.Bits.TGT = 0;
    }
}

/************************************************************************************
 * @fn      tick_init
 *
 * @brief   Init System Tick.
 *
 * @param   htick: Tick handle.
 */
void tick_init(TICK_HandleTypeDef *handle)
{
    /* enable Timerstamp compare default  */
    Tick->CTL.CMP_EN = 1;
    /* enable sleep interrupt as default */
    Tick->INT_CTL.Bits.SLP = 1;
}

/************************************************************************************
 * @fn      tick_get
 *
 * @brief   Get current System Tick.
 *
 * @param   clk: pointer used to store clock counter.
 *          fine: pointer used to store fine counter.
 */
void tick_get(uint32_t *clk, uint32_t *fine)
{
    Tick->CTL.SMP = 1;
    while(Tick->CTL.SMP == 1);
    
    *clk = Tick->CLK_SMP;
    *fine = Tick->FINE_SMP;
}

/************************************************************************************
 * @fn      tick_set_target
 *
 * @brief   Set system tick target time and wait for the target time in block mode.
 *
 * @param   clk: target clock counter.
 *          fine: target fine counter.
 */
void tick_set_target(uint32_t clk, uint32_t fine)
{
    Tick->CLK_TGT = clk;
    Tick->FINE_TGT = fine;

    while (Tick->INT_RAW.Bits.TGT == 0);
    Tick->INT_STA.Bits.TGT = 1;
}

/************************************************************************************
 * @fn      tick_set_target_IT
 *
 * @brief   Set system tick target time in interrupt mode.
 *
 * @param   clk: target clock counter.
 *          fine: target fine counter.
 */
void tick_set_target_IT(uint32_t clk, uint32_t fine)
{
    Tick->CLK_TGT = clk;
    Tick->FINE_TGT = fine;

    Tick->INT_CTL.Bits.TGT = 1;
}

/************************************************************************************
 * @fn      tick_start_corr
 *
 * @brief   Used to correction system tick after wake from sleep mode.
 *
 * @param   clk: clock counter corrcetion value.
 *          fine: fine counter corrcetion value.
 */
void tick_start_corr(uint32_t clk, uint32_t fine)
{
    Tick->CLK_CORR = clk;
    Tick->FINE_CORR = fine;

    Tick->SLP_CTL.SLP_CORR_EN = 1;
    while(Tick->SLP_CTL.SLP_CORR_EN == 1);
}
