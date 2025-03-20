/*
  ******************************************************************************
  * @file    SWD.h
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2023
  * @brief   Header file of SWD.c
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#ifndef __SWD_H__
#define __SWD_H__

#include <stdint.h>
#include <stdbool.h>

#include "fr30xx.h"

static inline void SWD_DELAY(void)
{
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
  __NOP();__NOP();__NOP();__NOP();__NOP();
}

#define DELAY_CYCLES    10U

static inline void PIN_DELAY(void)
{
#if (DELAY_CYCLES >= 1U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 2U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 3U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 4U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 5U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 6U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 7U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 8U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 9U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 10U)
	__NOP();
#endif
#if (DELAY_CYCLES >= 20U)
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
#endif
#if (DELAY_CYCLES >= 30U)
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
#endif
#if (DELAY_CYCLES >= 40U)
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
#endif
#if (DELAY_CYCLES >= 50U)
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
#endif
#if (DELAY_CYCLES >= 60U)
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();__NOP();
#endif
}

#if DELAY_CYCLES == 0
#define SWD_DELAY()
#else
#define SWD_DELAY()    PIN_DELAY()
#endif

// DAP Transfer Request
#define DAP_TRANSFER_APnDP              (1)
#define DAP_TRANSFER_RnW                (2)
#define DAP_TRANSFER_A2                 (3)
#define DAP_TRANSFER_A3                 (8)

#define REQ_AP    (1)
#define REQ_DP    (0)
#define REQ_R     (2)
#define REQ_W     (0)
#define REQ_ADDR_0    (0x00)
#define REQ_ADDR_1    (0x04)
#define REQ_ADDR_2    (0x08)
#define REQ_ADDR_3    (0x0C)


// DAP Transfer Response
#define DAP_TRANSFER_OK                 (0x04)
#define DAP_TRANSFER_WAIT               (0x02)
#define DAP_TRANSFER_FAULT              (0x01)

/* Exported functions --------------------------------------------------------*/

void SWD_IO_init(void);

void SWD_Connect(void);

void SWD_Enable_Debug(void);

void SWD_W_REG(uint32_t ADDR, uint32_t Value);
void SWD_R_REG(uint32_t ADDR, uint32_t *Buffer, uint32_t Length);

void SWD_W_SystemReg(uint32_t baudrate);

#endif
