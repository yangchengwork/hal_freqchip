/*
  ******************************************************************************
  * @file    driver_wdt.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   wdt module driver.
  *          This file provides firmware functions to manage the 
  *          Watchdog Timer (WDT) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      wdt_init
 *
 * @brief   wdt init.
 */
void wdt_init(struct_WDT_t *hwdt, enum_WDTMode_t fe_mode, uint32_t fu32_WDTCount)
{
    ool_write(PMU_REG_ACC_KEY, 0xDE);
    ool_write(PMU_REG_BLOCK_CTRL, ool_read(PMU_REG_BLOCK_CTRL) & 0XFD);
    ool_write(PMU_REG_ACC_KEY, 0x00);
    
    hwdt->wdt_CR.RPL  = 0;
    hwdt->wdt_CR.RMOD = fe_mode;

    hwdt->wdt_CNT = fu32_WDTCount;
    hwdt->wdt_CRR = 0x76;
}

/************************************************************************************
 * @fn      wdt_start
 *
 * @brief   wdt start.
 */
void wdt_start(struct_WDT_t *hwdt)
{
    hwdt->wdt_CR.WDT_EN = 1;
}

/************************************************************************************
 * @fn      wdt_stop
 *
 * @brief   wdt stop.
 */
void wdt_stop(struct_WDT_t *hwdt)
{
    hwdt->wdt_CR.WDT_EN = 0;
}

/************************************************************************************
 * @fn      wdt_feed
 *
 * @brief   feed wdt.
 */
void wdt_feed(struct_WDT_t *hwdt)
{
    hwdt->wdt_CRR = 0x76;
}

/************************************************************************************
 * @fn      wdt_feed
 *
 * @brief   feed wdt.
 */
void wdt_int_clear(struct_WDT_t *hwdt)
{
    volatile uint32_t EOI = hwdt->wdt_EOI;
}
