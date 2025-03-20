/*
  ******************************************************************************
  * @file    driver_pmu_rtc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   rtc module driver.
  *          This file provides firmware functions to manage the 
  *          Real Time Clock (RTC) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

static str_Time_t AlarmTime_A;
static str_Time_t AlarmTime_B;

/*********************************************************************
 * @fn      rtc_AlarmA_Handler
 *
 * @brief   Alarm interrupt handler 
 */
__WEAK void rtc_AlarmA_Handler(void)
{
    /* Update according to the periodic period in the rtc_AlarmConfig function. */
    rtc_AlarmUpdate(AlARM_A);
}
/*********************************************************************
 * @fn      rtc_AlarmB_Handler
 *
 * @brief   Alarm interrupt handler 
 */
__WEAK void rtc_AlarmB_Handler(void)
{
    /* Update according to the periodic period in the rtc_AlarmConfig function. */
    rtc_AlarmUpdate(AlARM_B);
}

/*********************************************************************
 * @fn      rtc_init
 *
 * @brief   rtc init
 *
 * @param   InitValue: RTC init value.
 */
void rtc_init(uint32_t InitValue)
{
    /* RTC clock enable */
    ool_write(PMU_REG_CLK_EN, ool_read(PMU_REG_CLK_EN) | PMU_RTC_CLK_EN_BIT);
    /* config init value */
    ool_write32(PMU_REG_RTC_COUNTER_0, InitValue);
    /* load init into cnt register */
    ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | PMU_RTC_UPD_EN_BIT);

    AlarmTime_A.UnitBackup = 0;
    AlarmTime_B.UnitBackup = 0;
}

/*********************************************************************
 * @fn      rtc_CountEnable
 *
 * @brief   RTC count enable
 */
void rtc_CountEnable(void)
{
    ool_write(PMU_REG_CLK_EN, ool_read(PMU_REG_CLK_EN) | PMU_RTC_CLK_EN_BIT);
}

/*********************************************************************
 * @fn      rtc_CountDisable
 *
 * @brief   RTC count disable
 */
void rtc_CountDisable(void)
{
    ool_write(PMU_REG_CLK_EN, ool_read(PMU_REG_CLK_EN) & ~PMU_RTC_CLK_EN_BIT);
}

/*********************************************************************
 * @fn      rtc_AlarmConfig
 *
 * @brief   rtc alarm config
 *
 * @param   fe_Alarm:   alarm select.
 * @param   lu8_hour:   hour
 * @param   lu8_Minute: minute
 * @param   lu8_Second: second
 */
void rtc_AlarmConfig(enum_Alarm_t fe_Alarm, uint32_t fu32_hour, uint32_t fu32_Minute, uint32_t fu32_Second)
{
    uint32_t lu32_Second;

    lu32_Second  = fu32_hour * 3600;
    lu32_Second += fu32_Minute * 60;
    lu32_Second += fu32_Second;

    switch (fe_Alarm)
    {
        case AlARM_A: 
        {
            /* Timing unit backup */
            AlarmTime_A.UnitBackup = lu32_Second;

            /* Convert to count value */
            lu32_Second *= system_get_LPRCCLK();
            lu32_Second += rtc_GetCount();

            rtc_AlarmSet(AlARM_A, lu32_Second);

            rtc_AlarmEnable(AlARM_A);

            pmu_enable_isr(PMU_RTC_A_INT_MSK_BIT);
        }break;

        case AlARM_B: 
        {
            /* Timing backup */
            AlarmTime_B.UnitBackup = lu32_Second;

            /* Convert to count value */
            lu32_Second *= system_get_LPRCCLK();
            lu32_Second += rtc_GetCount();

            rtc_AlarmSet(AlARM_B, lu32_Second);

            rtc_AlarmEnable(AlARM_B);

            pmu_enable_isr(PMU_RTC_B_INT_MSK_BIT);
        }break;

        default: break; 
    }
}

/*********************************************************************
 * @fn      rtc_GetCount
 *
 * @brief   Get rtc current counter value
 *
 * @param   None.
 * @return  lu32_CountValue: rtc current counter value.
 */
uint32_t rtc_GetCount(void)
{
    uint32_t lu32_CountValue;

    /* get current RTC counter */
    ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | PMU_RTC_SAMPLE_BIT);
    while ( ool_read(PMU_REG_RTC_CTRL) & PMU_RTC_SAMPLE_BIT);

    lu32_CountValue = ool_read32(PMU_REG_RTC_COUNTER_0);

    return lu32_CountValue;
}

/*********************************************************************
 * @fn      rtc_CountUpdate
 *
 * @brief   Update RTC counter
 *
 * @param   fu32_CountValue: update value.
 * @return  None.
 */
void rtc_CountUpdate(uint32_t fu32_CountValue)
{
    /* config init value */
    ool_write32(PMU_REG_RTC_COUNTER_0, fu32_CountValue);
    /* load init into cnt register */
    ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | PMU_RTC_UPD_EN_BIT);
}

/*********************************************************************
 * @fn      rtc_AlarmUpdate
 *
 * @brief   rtc alarm Update. 
 *          Update according to the periodic period in the rtc_AlarmConfig function.
 *
 * @param   fe_Alarm:   alarm select.
 */
void rtc_AlarmUpdate(enum_Alarm_t fe_Alarm)
{
    uint32_t lu32_AddValue;
    uint32_t lu32_AlarmValue;

    /* Convert to count value */
    if (fe_Alarm == AlARM_A)
        lu32_AddValue = AlarmTime_A.UnitBackup * system_get_LPRCCLK();
    else
        lu32_AddValue = AlarmTime_B.UnitBackup * system_get_LPRCCLK();

    lu32_AlarmValue = rtc_AlarmRead(fe_Alarm);

    lu32_AlarmValue += lu32_AddValue;

    rtc_AlarmSet(fe_Alarm, lu32_AlarmValue);
}

/*********************************************************************
 * @fn      rtc_AlarmEnable
 *
 * @brief   Alarm Enable
 */
void rtc_AlarmEnable(enum_Alarm_t fe_Alarm)
{
    ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) | fe_Alarm);
} 

/*********************************************************************
 * @fn      rtc_ALarmDisable
 *
 * @brief   Alarm Disable
 */
void rtc_AlarmDisable(enum_Alarm_t fe_Alarm)
{
    ool_write(PMU_REG_RTC_CTRL, ool_read(PMU_REG_RTC_CTRL) & ~fe_Alarm);
}

/*********************************************************************
 * @fn      rtc_AlarmRead
 *
 * @brief   read rtc alarm config value
 *
 * @return  lu32_ConfigValue: alarm config value.
 */
uint32_t rtc_AlarmRead(enum_Alarm_t fe_Alarm)
{
    uint32_t fu32_AlarmValue;

    switch (fe_Alarm)
    {
        case AlARM_A: 
        {
            fu32_AlarmValue = ool_read32(PMU_REG_ALARM_A_COUNTER_0);
        }break;

        case AlARM_B: 
        {
            fu32_AlarmValue = ool_read32(PMU_REG_ALARM_B_COUNTER_0);
        }break;

        default: break; 
    }
    
    return fu32_AlarmValue;
}

/*********************************************************************
 * @fn      rtc_AlarmSet
 *
 * @brief   Set rtc alarm config value
 *
 * @param   fu32_AlarmValue: alarm config value.
 */
void rtc_AlarmSet(enum_Alarm_t fe_Alarm, uint32_t fu32_AlarmValue)
{
    switch (fe_Alarm)
    {
        case AlARM_A: 
        {
            ool_write32(PMU_REG_ALARM_A_COUNTER_0, fu32_AlarmValue);
        }break;

        case AlARM_B: 
        {
            ool_write32(PMU_REG_ALARM_B_COUNTER_0, fu32_AlarmValue);
        }break;
    }
}

void rtc_AlarmStart_ms(enum_Alarm_t fe_Alarm, uint32_t fu32_ms)
{
    uint32_t lu32_AlarmValue;
    
    lu32_AlarmValue = fu32_ms * system_get_LPRCCLK() / 1000;
    lu32_AlarmValue += rtc_GetCount();

    rtc_AlarmSet(AlARM_A, lu32_AlarmValue);
    
    rtc_AlarmEnable(AlARM_A);
    
    pmu_enable_isr(PMU_RTC_A_INT_MSK_BIT);
}

void rtc_AlarmUpdate_ms(enum_Alarm_t fe_Alarm, uint32_t fu32_ms)
{
   
    uint32_t lu32_AddValue = fu32_ms * system_get_LPRCCLK() / 1000;
    uint32_t lu32_AlarmValue = rtc_AlarmRead(fe_Alarm);
    
    lu32_AlarmValue += lu32_AddValue;
    
    rtc_AlarmSet(AlARM_A, lu32_AlarmValue);
}
