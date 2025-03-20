/*
  ******************************************************************************
  * @file    driver_crc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   CRC module driver.
  *          This file provides firmware functions to manage the 
  *          CRC(Cyclic Redundancy Check) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      crc_init
 *
 * @brief   Initial crc according to the mode
 *
 * @param   fe_crc_mode: crc mode.
 */
void crc_init(enum_CRC_MODE_SEL_t fe_crc_mode)
{
    /* CRC control config */
    CRC->CRC_CTRL = fe_crc_mode | CRC_START;
}

/************************************************************************************
 * @fn      crc_Calculate
 *
 * @brief   Calculate CRC from the input data.
 *
 * @param   fp_Data: Input calculated data.
 * @param   fu32_size: The data size need to do crc.
 */
uint32_t crc_Calculate(uint8_t *fp_Data, uint32_t fu32_size)
{   
    if (fu32_size == 0)
    {
        return 0;
    }

    while (fu32_size >= 8)
    {
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        
        fu32_size -= 8;
    }
    
    while(fu32_size)
    {
        CRC->CRC_FIFO_DATA = *(fp_Data++);
        fu32_size--;
    }

    return CRC->CRC_RESULT;
}
