/*
  ******************************************************************************
  * @file    driver_pwm.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   SPI module driver.
  *          This file provides firmware functions to manage the 
  *          Pulse-Width Modulatio (PWM) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      pwm_config
 *
 * @brief   PWM mode. Config channel paramter.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select output channel.(1bit ~ 1channel)
 *          fstr_Config: Config paramter.
 */
void pwm_config(struct_PWM_t *PWMx, uint16_t fu16_channel, struct_PWM_Config_t fstr_Config)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Channel;

    PWMx->OutputSelect &= ~fu16_channel;

    /* Configure Select channel */
    while (fu16_channel >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Channel = fu16_channel & (1uL << lu32_Position);

        if (lu32_Current_Channel) 
        {
            /* stop  sync update */
            PWMx->Update.PWM_Update &= ~lu32_Current_Channel;

            PWMx->Edge[lu32_Position].Posedge = fstr_Config.Posedge;
            PWMx->Edge[lu32_Position].Negedeg = fstr_Config.Negedge;

            PWMx->Frequency[lu32_Position].Prescale = fstr_Config.Prescale - 1;
            PWMx->Frequency[lu32_Position].Period   = fstr_Config.Period - 1;
        }

        lu32_Position++;
    }
}

/************************************************************************************
 * @fn      pwm_complementary_config
 *
 * @brief   Complementary outputs with dead-time insertion
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_MainChannel:     Select main          output channel.(1bit ~ 1channel)
 * @param   ComplementaryChannel: Select complementary output channel.(1bit ~ 1channel)
 *          fstr_Config: Config paramter.
 */
bool pwm_complementary_config(struct_PWM_t *PWMx, uint16_t fu16_MainChannel, uint16_t ComplementaryChannel, struct_PWM_Complementary_Config_t fstr_Config)
{ 
    struct_PWM_Config_t PWM_Config;
    
    PWM_Config.Prescale = fstr_Config.Prescale;
    PWM_Config.Period   = fstr_Config.Period;

    if (fstr_Config.MianDeadTime != 0) 
    {
        if (fstr_Config.MianDeadTime - 1 >= PWM_Config.Period - 1 - fstr_Config.DutyCycle - fstr_Config.MianDeadTime) 
            return false;

        PWM_Config.Posedge = fstr_Config.MianDeadTime - 1;
        PWM_Config.Negedge = PWM_Config.Period - 1 - fstr_Config.DutyCycle - fstr_Config.MianDeadTime;
    }
    else 
    {
        /* Error check */
        if (fstr_Config.DutyCycle >= PWM_Config.Period) 
            return false;
        if (fstr_Config.DutyCycle == 0)
            return false;

        PWM_Config.Posedge = PWM_Config.Period - 1;
        PWM_Config.Negedge = PWM_Config.Period - 1 - fstr_Config.DutyCycle;
    }

    pwm_config(PWMx, fu16_MainChannel, PWM_Config);
    
    if (fstr_Config.CompDeadTime != 0) 
    {
        if (PWM_Config.Period - 1 - fstr_Config.DutyCycle + fstr_Config.MianDeadTime >= PWM_Config.Period - 1 - fstr_Config.CompDeadTime) 
            return false;

        PWM_Config.Posedge = PWM_Config.Period - 1 - fstr_Config.DutyCycle + fstr_Config.MianDeadTime;
        PWM_Config.Negedge = PWM_Config.Period - 1 - fstr_Config.CompDeadTime;
    }
    else 
    {
        /* Error check */
        if (fstr_Config.DutyCycle >= PWM_Config.Period) 
            return false;
        if (fstr_Config.DutyCycle == 0)
            return false;

        PWM_Config.Posedge = PWM_Config.Period - 1 - fstr_Config.DutyCycle;
        PWM_Config.Negedge = PWM_Config.Period - 1;
    }

    pwm_config(PWMx, ComplementaryChannel, PWM_Config);
    
    return true;
}

/************************************************************************************
 * @fn      pwm_output_enable
 *
 * @brief   PWM_DAC mode. output enable.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select output channel.(1bit ~ 1channel)
 */
void pwm_output_enable(struct_PWM_t *PWMx, uint16_t fu16_channel)
{
    PWMx->Update.PWM_Update |= fu16_channel;

    PWMx->CNT_EN     |=  fu16_channel;
    PWMx->ChannelEN  |=  fu16_channel;
}

/************************************************************************************
 * @fn      pwm_output_disable
 *
 * @brief   PWM_DAC mode. output disable.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu8_channel: Select output channel.(1bit ~ 1channel)
 */
void pwm_output_disable(struct_PWM_t *PWMx, uint16_t fu16_channel)
{
    /* stop sync update */
    PWMx->Update.PWM_Update &= ~fu16_channel;

    PWMx->ChannelEN &=  ~fu16_channel;
    PWMx->CNT_EN    &=  ~fu16_channel;
}

/************************************************************************************
 * @fn      pwm_output_status
 *
 * @brief   PWM_DAC mode. output inverter disable.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fe_channel: Select output channel.(1bit ~ 1channel)
 *  
 * @return  true : PWM runing.
 *          false: PWM stop.
 */
bool pwm_output_status(struct_PWM_t *PWMx, enum_PWMChannel_t fe_channel)
{
    if (PWMx->Update.PWM_Status & fe_channel) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

/************************************************************************************
 * @fn      pwm_output_updata
 *
 * @brief   channel Posedge/Negedeg/Prescale/Period updata
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select output channel.(1bit ~ 1channel)
 */
void pwm_output_updata(struct_PWM_t *PWMx, uint16_t fu16_channel)
{
    /* start sync update */
    PWMx->Update.PWM_Update |= fu16_channel;
}

/************************************************************************************
 * @fn      pwm_capture_config
 *
 * @brief   Capture mode. Config channel paramter.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select capture channel(1bit ~ 1channel).
 *          fstr_Config: Config paramter.
 */
void pwm_capture_config(struct_PWM_t *PWMx, uint16_t fu16_channel, struct_Capture_Config_t fstr_Config)
{
    uint32_t lu32_Position = 0;
    uint32_t lu32_Current_Channel;

    /* Set Capture Prescale */
    PWMx->CapturePrescale = 0x10 | fstr_Config.CapturePrescale; 

    /* Configure Select channel */
    while (fu16_channel >> lu32_Position != 0) 
    {
        /* Get current pin position */
        lu32_Current_Channel = fu16_channel & (1uL << lu32_Position);

        if (lu32_Current_Channel) 
        {
            /* Set Capture mode */
            if (fstr_Config.CaptureMode == MODE_LOOP) 
            {
                PWMx->CaptureCtrl.Capture_Mode &= ~lu32_Current_Channel;
            }
            else 
            {
                PWMx->CaptureCtrl.Capture_Mode |= lu32_Current_Channel;
            }
        }

        lu32_Position++;
    }
}


/************************************************************************************
 * @fn      pwm_capture_enable
 *
 * @brief   capture enable.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select capture channel.(1bit ~ 1channel)
 */
void pwm_capture_enable(struct_PWM_t *PWMx, uint16_t fu16_channel)
{
    PWMx->CaptureCtrl.Capture_EN |= fu16_channel;
}

/************************************************************************************
 * @fn      pwm_capture_disable
 *
 * @brief   pwm_capture_disable.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fu16_channel: Select capture channel.(1bit ~ 1channel)
 */
void pwm_capture_disable(struct_PWM_t *PWMx, uint16_t fu16_channel)
{
    PWMx->CaptureCtrl.Capture_EN &= ~fu16_channel;
}

/************************************************************************************
 * @fn      pwm_capture_status
 *
 * @brief   get pwm_capture_status.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fe_channel: Select capture channel.(1bit ~ 1channel)
 * @return  true : capture result value ready.
 *          false: capture result value not ready.
 */
bool pwm_capture_status(struct_PWM_t *PWMx, enum_PWMChannel_t fe_channel)
{
    return (PWMx->CaptureStatus & fe_channel) ? true : false;
}

/************************************************************************************
 * @fn      pwm_capture_status_clear
 *
 * @brief   capture status claear.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fe_channel: Select capture channel.(1bit ~ 1channel)
 */
void pwm_capture_status_clear(struct_PWM_t *PWMx,enum_PWMChannel_t fe_channel)
{
    PWMx->CaptureStatus |= fe_channel;
}

/************************************************************************************
 * @fn      pwm_capture_value
 *
 * @brief   get pwm_capture_value.
 *
 * @param   PWMx: PWM0、PWM1.
 *          fe_channel: Select capture channel.(1bit ~ 1channel)
 */
uint32_t pwm_capture_value(struct_PWM_t *PWMx, enum_PWMChannel_t fe_channel)
{
    uint8_t i;
    
    for (i = 0; i < 16; i++)
    {
        if (fe_channel & 1 << i)
            break;
    }

    return PWMx->CaptureValue[i];
}

