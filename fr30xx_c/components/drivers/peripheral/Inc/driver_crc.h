/*
  ******************************************************************************
  * @file    driver_crc.h
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   Header file of CRC module.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#ifndef __DRIVER_CRC_H__
#define __DRIVER_CRC_H__

#include "fr30xx.h"

/** @addtogroup CRC_Registers_Section
  * @{
  */
/* ################################ Register Section Start ################################ */

#define CRC_START (0x01)
#define CRC_CLEAR (0x08)

typedef struct 
{
    volatile   uint32_t           CRC_CTRL;           /* Offset 0x00 */
    volatile   uint32_t           CRC_STATUS;         /* Offset 0x04 */
    volatile   uint32_t           CRC_FIFO_DATA;      /* Offset 0x08 */
    volatile   uint32_t           CRC_RESULT;         /* Offset 0x0C */
}struct_CRC_t;

#define CRC        ((struct_CRC_t *)CRC_BASE)

/* ################################ Register Section END ################################## */
/**
  * @}
  */


/** @addtogroup CRC_Initialization_Config_Section
  * @{
  */
/* ################################ Initialization��Config Section Start ################################ */

typedef enum
{
    CRC_INITVALUE_0          = 0x08u,
    CRC_INITVALUE_1          = 0x18u,   
    
    CRC_MULTINOMIAL_16_1021  = 0x02u,
    CRC_MULTINOMIAL_16_8005  = 0x04u,
    CRC_MULTINOMIAL_32       = 0x06u,
      
    CRC_INVERT_EN            = 0x60u,

    CRC_XOR_OUT_EN           = 0x80u,
}enum_CRC_Accumulate;

/* Mode               bit wide  |  polynomial  |  init value  |  Result XOR value  |  Input invert  |  Output invert
  CRC8                  8             0x07         0x00              0x00                False            False
  CRC8_ROHC             8             0x07         0xFF              0x00                True             True
  CRC16_IBM             16            0x8005       0x0000            0x0000              True             True
  CRC16_MAXIM           16            0x8005       0x0000            0xFFFF              True             True
  CRC16_USB             16            0x8005       0xFFFF            0xFFFF              True             True
  CRC16_MODBUS          16            0x8005       0xFFFF            0x0000              True             True
  CRC16_CCITT           16            0x1021       0x0000            0x0000              True             True
  CRC16_CCITT_FALSE     16            0x1021       0xFFFF            0x0000              False            False
  CRC16_X25             16            0x1021       0xFFFF            0xFFFF              True             True
  CRC16_XMODEM          16            0x1021       0x0000            0x0000              False            False
  CRC32                 32            0x04C11DB7   0xFFFFFFFF        0xFFFFFFFF          True             True
  CRC32_MPEG2           32            0x04C11DB7   0xFFFFFFFF        0x00000000          False            False
*/
typedef enum
{
    FR_DRIVER_WRAPPER(CRC8)                =  (CRC_INITVALUE_0),
    FR_DRIVER_WRAPPER(CRC8_ROHC)           =  (CRC_INITVALUE_1 | CRC_INVERT_EN),
    FR_DRIVER_WRAPPER(CRC16_IBM)           =  (CRC_INITVALUE_0 | CRC_MULTINOMIAL_16_8005 | CRC_INVERT_EN),
    FR_DRIVER_WRAPPER(CRC16_MAXIM)         =  (CRC_INITVALUE_0 | CRC_MULTINOMIAL_16_8005 | CRC_INVERT_EN | CRC_XOR_OUT_EN),
    FR_DRIVER_WRAPPER(CRC16_USB)           =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_16_8005 | CRC_INVERT_EN | CRC_XOR_OUT_EN),
    FR_DRIVER_WRAPPER(CRC16_MODBUS)        =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_16_8005 | CRC_INVERT_EN),
    FR_DRIVER_WRAPPER(CRC16_CCITT)         =  (CRC_INITVALUE_0 | CRC_MULTINOMIAL_16_1021 | CRC_INVERT_EN),
    FR_DRIVER_WRAPPER(CRC16_CCITT_FALSE)   =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_16_1021),
    FR_DRIVER_WRAPPER(CRC16_X25)           =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_16_1021 | CRC_INVERT_EN | CRC_XOR_OUT_EN),
    FR_DRIVER_WRAPPER(CRC16_XMODEM)        =  (CRC_INITVALUE_0 | CRC_MULTINOMIAL_16_1021),
    FR_DRIVER_WRAPPER(CRC32)               =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_32 | CRC_INVERT_EN | CRC_XOR_OUT_EN),
    FR_DRIVER_WRAPPER(CRC32_MPEG2)         =  (CRC_INITVALUE_1 | CRC_MULTINOMIAL_32),
}enum_CRC_MODE_SEL_t;

/* ################################ Initialization��Config Section END ################################## */
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/* Initial crc with initial value and mode */
void crc_init(enum_CRC_MODE_SEL_t fe_crc_mode);

/* CRC Calculate */
uint32_t crc_Calculate(uint8_t *fp_Data, uint32_t fu32_size);

#endif
