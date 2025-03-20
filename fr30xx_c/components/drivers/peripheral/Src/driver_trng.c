/*
  ******************************************************************************
  * @file    driver_trng.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   TRNG module driver.
  *          This file provides firmware functions to manage the 
  *          True Random Number Generator (TRNG) peripheral.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

static int trng_sel_smp_verify(int Rand_sel_index, int Sample_cnt_index)
{ 
    int Error;

    TRNG->TRNG_IMR = 0XFFFFFF00;
    TRNG->TRNG_SrcEN = 0;
    TRNG->TRNG_DebugControl = 0;

    /* Selects the number of inverters in the ring oscillator */
    TRNG->TRNG_Config = Rand_sel_index;
    while((TRNG->TRNG_Config & Rand_sel_index) != Rand_sel_index);
    TRNG->TRNG_SAMPLE_CNT1 = Sample_cnt_index;
    while((TRNG->TRNG_SAMPLE_CNT1 & Sample_cnt_index) != Sample_cnt_index);

    TRNG->TRNG_AUTOCORR = 0;
    TRNG->TRNG_SrcEN = 1;

    while(!(TRNG->TRNG_ISR));

    if (TRNG->TRNG_ISR & TRNG_STATUS_EHR_VALID)
        Error = 0;
    else
        Error = 1;

    if (Error)
    {
        TRNG->TRNG_Reset = 1;
    }

    TRNG->TRNG_ICR = 0XFFFFFFFF;

    return !Error;
}

/************************************************************************************
 * @fn      trng_init
 *
 * @brief   trng init.
 */
void trng_init(void)
{
    #define MAX_SAMPLE_CNT_INDX    (0xFFFFFF00)

    uint32_t Rand_sel_index;
    uint32_t Sample_cnt_index;
    uint32_t Rand_Sample_Passed;

    for (Rand_sel_index = 1; Rand_sel_index < 4; Rand_sel_index++)
    {
        Rand_Sample_Passed = 0;

        Sample_cnt_index = 33333;

        do{
            Rand_Sample_Passed = trng_sel_smp_verify(Rand_sel_index, Sample_cnt_index);
            Sample_cnt_index += 111;
        } while ((Sample_cnt_index <= (MAX_SAMPLE_CNT_INDX)) && !Rand_Sample_Passed);

        while(TRNG->TRNG_BIST_CNTR0 <= 0);
        while(TRNG->TRNG_BIST_CNTR1 <= 0);
        while(TRNG->TRNG_BIST_CNTR2 <= 0);

        if (Rand_Sample_Passed)
            break;
    }
    TRNG->TRNG_SrcEN = 0;
}

/************************************************************************************
 * @fn      trng_read_rand_num
 *
 * @brief   Read random numbers.
 * 
 * @param   fp_buffer: random numbers buffer.
 * @param   length: random numbers length, max 24 byte.
 */
void trng_read_rand_num(uint8_t *fp_buffer, uint8_t length)
{
    int i;

    uint32_t lu32_buffer[6];
    uint8_t *Point = (uint8_t *)lu32_buffer;

    if (length > 24)
        length = 24;

    /* TRNG enable */
    TRNG->TRNG_SrcEN = 1;
    /* Wait VALID */
    while(!(TRNG->TRNG_ISR & TRNG_STATUS_EHR_VALID));

    for(int i = 0; i < 6; i++)
        lu32_buffer[i] = TRNG->TRNG_Data[i];

    for (int i = 0; i < length; i++)
        fp_buffer[i] = Point[i];

    /* TRNG disable */
    TRNG->TRNG_SrcEN = 0;
    /* Clear status */
    TRNG->TRNG_ICR = TRNG_STATUS_EHR_VALID;
}
