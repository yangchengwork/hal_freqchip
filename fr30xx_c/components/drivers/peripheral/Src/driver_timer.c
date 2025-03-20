/*
  ******************************************************************************
  * @file    driver_timer.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   Timer module driver.
  *          This file provides firmware functions to manage the Timer peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024  FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      timer_init
 *
 * @brief   timer initialize.
 *
 * @param   TIMERx: Timer handle.
 *          fu32_LoadCount: Timer Load Count.
 */
void timer_init(struct_Timer_t *TIMERx, uint32_t fu32_LoadCount)
{
    TIMERx->LoadCount = fu32_LoadCount;

    TIMERx->Control.MODE = 1;
}

/************************************************************************************
 * @fn      timer_int_enable
 *
 * @brief   timer interrupt enable.
 *
 * @param   TIMERx: Timer handle.
 */
void timer_int_enable(struct_Timer_t *TIMERx)
{
    TIMERx->Control.INT_MASK = 0;
}

/************************************************************************************
 * @fn      timer_int_disable
 *
 * @brief   timer interrupt disable.
 *
 * @param   TIMERx: Timer handle.
 */
void timer_int_disable(struct_Timer_t *TIMERx)
{
    TIMERx->Control.INT_MASK = 1;
}

/************************************************************************************
 * @fn      timer_int_clear
 *
 * @brief   timer interrupt status clear.
 *
 * @param   TIMERx: Timer handle.
 */
void timer_int_clear(struct_Timer_t *TIMERx)
{
    uint32_t lu32_TempValue;
    
    lu32_TempValue = TIMERx->IntClear;
}

/************************************************************************************
 * @fn      timer_int_status
 *
 * @brief   get timer interrupt status.
 *
 * @param   TIMERx: Timer handle.
 */
bool timer_int_status(struct_Timer_t *TIMERx)
{
    return TIMERx->IntStatus;
}

/************************************************************************************
 * @fn      timer_start
 *
 * @brief   Timer start count.
 *
 * @param   TIMERx: Timer handle.
 */
void timer_start(struct_Timer_t *TIMERx)
{
    TIMERx->Control.ENABLE = 1;
}

/************************************************************************************
 * @fn      timer_stop
 *
 * @brief   Timer stop count.
 *
 * @param   TIMERx: Timer handle.
 */
void timer_stop(struct_Timer_t *TIMERx)
{
    TIMERx->Control.ENABLE = 0;
}

/************************************************************************************
 * @fn      timer_get_CurrentCount
 *
 * @brief   get Timer current count.
 *
 * @param   TIMERx: Timer handle.
 */
uint32_t timer_get_CurrentCount(struct_Timer_t *TIMERx)
{
    return TIMERx->CurrentValue;
}
