/*
  ******************************************************************************
  * @file    driver_sd.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   SD controller HAL module driver.
  *          This file provides firmware functions to manage the 
  *          Secure Digital Input and Output Card (SDIO) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      SD_SDCardClass_Init
 *
 * @brief   SD Crad class initialization.
 */
void SD_SDCardClass_Init(struct_SD_t *SDx)
{
    /* Stop BOOT */
    __SD_BOOT_ACK_DISABLE(SDx);
    __SD_BOOT_STOP(SDx);
    __SD_ALTERNATE_BOOT_STOP(SDx);
    /* MMC Stream mode disable */
    __SD_MMC_STREAM_MODE_DISABLE(SDx);
    /* MMC SPI mode disable */
    __SD_SPI_MODE_DISABLE(SDx);


    /* Internal clock enable */
    __SD_INTERNAL_CLOCK_ENABLE(SDx);
    /* Wait internal clock to stabilize */
    while(!__SD_IS_INTERNAL_CLOCK_STABLE(SDx));

    /* SDCLK disable */
    __SD_SDCLK_DISABLE(SDx);
    /* Data Timeout Counter Value */
    __SD_DATA_TIMEOUT_COUNT(SDx, 0xE);
    /* Normal Speed mode */
    __SD_SPEED_MODE(SDx, 0);
    /* Initialization clock use 400K */
    uint32_t ClockSource;
    if (SDx == SDIO0)
        ClockSource = system_get_peripheral_clock(PER_CLK_SDIOH0);
    
    if (ClockSource)
        __SD_CLOCK_DIV_LOW_8BIT(SDx, ClockSource / 400000 / 2);
    else
        __SD_CLOCK_DIV_LOW_8BIT(SDx, 100);
    /* SDCLK enable */
    __SD_SDCLK_ENABLE(SDx);

    /* Bus push-pull mode */
    __SD_MMC_OD_PP(SDx, 1);
    /* Stop Reset */
    __SD_MMC_EXT_RST(SDx, 0);
    /* Bus voltage select 3.3V */
    __SD_BUS_VOLTAGE_SELECT(SDx, VOLTAGE_3_3);
    /* Bus Power on */
    __SD_BUS_POWER_ON(SDx);
    
    /* All interrupt status enable */
    __SD_INT_STATUS_ALL_ENABLE(SDx);

    /* Init Block Size 64 Byte */
    __SD_SET_BLOCK_SIZE(SDx, 64);
    /* SDMA Max Buffer as 512K Byte */
    __SD_SET_SDMA_BUFF(SDx, 0x7);
    /* Disable burst size */
    __SD_AHB_BURST_SIZE(SDx, 0);
}

/************************************************************************************
 * @fn      SD_SDMMCClass_Init
 *
 * @brief   Embedded Multi Media Card class initialization.
 */
void SD_SDMMCClass_Init(struct_SD_t *SDx)
{
    /* Stop BOOT */
    __SD_BOOT_ACK_DISABLE(SDx);
    __SD_BOOT_STOP(SDx);
    __SD_ALTERNATE_BOOT_STOP(SDx);
    /* MMC Stream mode disable */
    __SD_MMC_STREAM_MODE_DISABLE(SDx);
    /* MMC SPI mode disable */
    __SD_SPI_MODE_DISABLE(SDx);


    /* Internal clock enable */
    __SD_INTERNAL_CLOCK_ENABLE(SDx);
    /* Wait internal clock to stabilize */
    while(!__SD_IS_INTERNAL_CLOCK_STABLE(SDx));

    /* SDCLK disable */
    __SD_SDCLK_DISABLE(SDx);
    /* Data Timeout Counter Value */
    __SD_DATA_TIMEOUT_COUNT(SDx, 0xE);
    /* Normal Speed mode */
    __SD_SPEED_MODE(SDx, 0);
    /* Initialization clock use 400K */
    uint32_t ClockSource;
    if (SDx == SDIO0)
        ClockSource = system_get_peripheral_clock(PER_CLK_SDIOH0);
    
    if (ClockSource)
        __SD_CLOCK_DIV_LOW_8BIT(SDx, ClockSource / 400000 / 2);
    else
        __SD_CLOCK_DIV_LOW_8BIT(SDx, 100);
    /* SDCLK enable */
    __SD_SDCLK_ENABLE(SDx);

    /* Bus open-drain mode */
    __SD_MMC_OD_PP(SDx, 0);
    /* Stop Reset */
    __SD_MMC_EXT_RST(SDx, 0);
    /* Bus voltage select 3.3V */
    __SD_BUS_VOLTAGE_SELECT(SDx, VOLTAGE_3_3);
    /* Bus Power on */
    __SD_BUS_POWER_ON(SDx);
    
    /* All interrupt status enable */
    __SD_INT_STATUS_ALL_ENABLE(SDx);

    /* Init Block Size 512 Byte */
    __SD_SET_BLOCK_SIZE(SDx, 512);
    /* SDMA Max Buffer as 512K Byte */
    __SD_SET_SDMA_BUFF(SDx, 0x7);
    /* Disable burst size */
    __SD_AHB_BURST_SIZE(SDx, 0);
}

/************************************************************************************
 * @fn      SD_SendCmd
 *
 * @brief   Build command register value and send new command.
 *
 * @param   SDx: Pointer to SD register base.
 *          Command: pointer to a SDIO_CmdTypeDef structure that contains the 
 *                   configuration information for the SD command
 */
void SD_SendCmd(struct_SD_t *SDx, SDIO_CmdTypeDef *Command)
{
    uint16_t lu16_Command;

    /* Data0 line is in use */
    while(!(__SD_GET_PRESENT_STATE(SDx) & PreState_DAT0_SIGNAL_MASK));
    /* CMD line is in use */
    while(__SD_GET_PRESENT_STATE(SDx) & PreState_CMD_LINE_MASK);
    /* Data line is in use */
    while(__SD_GET_PRESENT_STATE(SDx) & PreState_DAT_LINE_MASK);

    /* Build register value */
    lu16_Command = Command->CmdIndex | Command->CmdType | Command->DataType | Command->ResponseType;
    /* Set command argument */
    __SD_SET_ARGUMENT1(SDx, Command->Argument);
    /* Send new command */
    __SD_SEND_COMMAND(SDx, lu16_Command);
}

/************************************************************************************
 * @fn      SD_GetCmdResp1
 *
 * @brief   Checks for error conditions for R1 response.
 */
static uint32_t SD_GetCmdResp1(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t lu32_Response;

    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        /* Read response from register */
        lu32_Response = __SD_GET_RESPONSE0(SDx);
        /* response with error */
        if (lu32_Response & RESP1_ERR_ERRORBITS) 
        {
            lu32_ErrState = lu32_Response;
        }
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_GetCmdResp2
 *
 * @brief   Get command reoponse R2.
 */
static uint32_t SD_GetCmdResp2(struct_SD_t *SDx, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        fp32_Response[0] = __SD_GET_RESPONSE0(SDx);
        fp32_Response[1] = __SD_GET_RESPONSE1(SDx);
        fp32_Response[2] = __SD_GET_RESPONSE2(SDx);
        fp32_Response[3] = __SD_GET_RESPONSE3(SDx);
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_GetCmdResp3
 *
 * @brief   Get command reoponse R3.
 */
static uint32_t SD_GetCmdResp3(struct_SD_t *SDx, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        fp32_Response[0] = __SD_GET_RESPONSE0(SDx);
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_GetCmdResp6
 *
 * @brief   Get command reoponse R6.
 */
static uint32_t SD_GetCmdResp6(struct_SD_t *SDx, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        fp32_Response[0] = __SD_GET_RESPONSE0(SDx);

        if (fp32_Response[0] & RCA_ERR_ERRORBITS)
        {
            lu32_ErrState = fp32_Response[0] & RCA_ERR_ERRORBITS;
        }
        else 
        {
            fp32_Response[0] = fp32_Response[0] & 0xFFFF0000;
        }
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_GetCmdResp7
 *
 * @brief   Get command reoponse R7.
 */
static uint32_t SD_GetCmdResp7(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t lu32_Response;
    
    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        /* Read response from register */
        lu32_Response = __SD_GET_RESPONSE0(SDx);
        
        if (lu32_Response == 0x000001AA) 
        {
            lu32_ErrState = INT_NO_ERR;
        }
        else 
        {
            lu32_ErrState = INT_ERR_MASK;
        }
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SetBlockCount
 *
 * @brief   Set block count.
 */
uint32_t SD_CMD_SetBlockCount(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD23 SET_BLOCK_COUNT */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD23_SET_BLOCK_COUNT;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_ReadSingleBlock
 *
 * @brief   Read Single Block.
 */
uint32_t SD_CMD_ReadSingleBlock(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);
    
    /* Send CMD17 READ_SINGLI_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD17_READ_SINGLI_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_ReadMultiBlock
 *
 * @brief   Read Multi Block.
 */
uint32_t SD_CMD_ReadMultiBlock(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Enable */
    __SD_MULTI_BLOCK_ENABLE(SDx);
    /* Multi Block Transfer Count Enbale */
    __SD_BLOCK_COUNT_ENABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);

    /* Send CMD18 READ_MULTIPLE_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD18_READ_MULTIPLE_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_ReadBlock_SDMA
 *
 * @brief   Read Multi Block use SDMA.
 */
uint32_t SD_CMD_ReadBlock_SDMA(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Enable */
    __SD_MULTI_BLOCK_ENABLE(SDx);
    /* Multi Block Transfer Count Enbale */
    __SD_BLOCK_COUNT_ENABLE(SDx);
    /* DMA Enbale */
    __SD_DMA_ENABLE(SDx);
    /* DMA Select SDMA */
    __SD_DMA_SELECT(SDx, 0);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);

    /* Send CMD18 READ_MULTIPLE_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD18_READ_MULTIPLE_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_WriteSingleBlock
 *
 * @brief   Write Single Block.
 */
uint32_t SD_CMD_WriteSingleBlock(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Write */
    __SD_DATA_DIRECTION(SDx, 0);


    /* Send CMD24 WRITE_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD24_WRITE_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_WriteMultiBlock
 *
 * @brief   Write Multi Block.
 */
uint32_t SD_CMD_WriteMultiBlock(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;
    
    /* Multi Block Transfer Enable */
    __SD_MULTI_BLOCK_ENABLE(SDx);
    /* Multi Block Transfer Count Enbale */
    __SD_BLOCK_COUNT_ENABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Write */
    __SD_DATA_DIRECTION(SDx, 0);


    /* Send CMD25 WRITE_MULTIPLE_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD25_WRITE_MULTIPLE_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_WriteBlock_SDMA
 *
 * @brief   Write Multi Block use SDMA.
 */
uint32_t SD_CMD_WriteBlock_SDMA(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Enable */
    __SD_MULTI_BLOCK_ENABLE(SDx);
    /* Multi Block Transfer Count Enbale */
    __SD_BLOCK_COUNT_ENABLE(SDx);
    /* DMA Enbale */
    __SD_DMA_ENABLE(SDx);
    /* DMA Select SDMA */
    __SD_DMA_SELECT(SDx, 0);
    /* Direction: Write */
    __SD_DATA_DIRECTION(SDx, 0);

    /* Send CMD25 WRITE_MULTIPLE_BLOCK */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD25_WRITE_MULTIPLE_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_EraseStartAddr
 *
 * @brief   Sets the address of the first write block to be erase.
 */
uint32_t SD_CMD_EraseStartAddr(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD32 ERASE_WR_BLK_START */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SD_CMD32_ERASE_WR_BLK_START;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);
    
    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_EraseEndAddr
 *
 * @brief   Sets the address of the last write block of the continuous range to be erase.
 */
uint32_t SD_CMD_EraseEndAddr(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD33 ERASE_WR_BLK_END */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SD_CMD33_ERASE_WR_BLK_END;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_Erase
 *
 * @brief   Erase all previously selected write bolcks.
 */
uint32_t SD_CMD_Erase(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD38 REASE */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD38_ERASE;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1b_R5b;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1b */
    lu32_ErrState = SD_GetCmdResp1(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_StopTransfer
 *
 * @brief   Forces the card to stop transmission.
 */
uint32_t SD_CMD_StopTransfer(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD12 SD_CMD12_STOP_TRANSMISSION */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD12_STOP_TRANSMISSION;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1b_R5b;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1b */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_BlockLength
 *
 * @brief   Command sets the block length for all following block command(read,write,lcok).
 */
uint32_t SD_CMD_BlockLength(struct_SD_t *SDx,  uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD16 SET_BLOCKLEN */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD16_SET_BLOCKLEN;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);
        
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SelectCard
 *
 * @brief   command toggles a card between the stand-by and transfer states or between
 *          the programming and disconnect states.
 */
uint32_t SD_CMD_SelectCard(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD7 SELECT_CARD */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD7_SEL_DESEL_CARD;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1b_R5b;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1b */
    lu32_ErrState = SD_GetCmdResp1(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_DeselectCard
 *
 * @brief   command toggles a card between the stand-by and transfer states or between
 *          the programming and disconnect states.
 */
uint32_t SD_CMD_DeselectCard(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD7 DESELECT_CARD */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD7_SEL_DESEL_CARD;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_NO;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1b */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_GoIdleState
 *
 * @brief   Resets all card to idle state.
 */
uint32_t SD_CMD_GoIdleState(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD0 GO_IDLE_STATE */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD0_GO_IDLE_STATE;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_NO;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* wait command complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));
    
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK) 
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SendRelAddr
 *
 * @brief   Ask the card to publish a new relative address(RCA)
 */
uint32_t SD_CMD_SendRelAddr(struct_SD_t *SDx, uint32_t *fp32_RCA)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD3 SEND_RELATIVE_ADDR */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SD_CMD3_SEND_RELATIVE_ADDR;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R6 */
    lu32_ErrState = SD_GetCmdResp6(SDx, fp32_RCA);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_VoltageSwitch
 *
 * @brief   Switch to 1.8V bus signaling level.
 */
uint32_t SD_CMD_VoltageSwitch(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD11 VOLTAGE_SWITCH */
    SD_CmdTpye.Argument     = 0x00000000;
    SD_CmdTpye.CmdIndex     = SD_CMD11_VOLTAGE_SWITCH;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SwitchFunc
 *
 * @brief   Checks switchable function(mode 0) and switch card function(mode 1).
 */
uint32_t SD_CMD_SwitchFunc(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);
    
    /* Send CMD6 SWITCH_FUNC */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SD_CMD6_SWITCH_FUNC;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SendInterfaceCondition
 *
 * @brief   Send SD memory card interface condition, which includes host supply voltage
 *          information and asks the card whether card support voltage.
 */
uint32_t SD_CMD_SendInterfaceCondition(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Argument: 
    - [31:12]: Reserved (shall be set to '0')
    - [11:8]:  Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
    - [7:0]:   Check Pattern (recommended 0xAA) */
  
    /* Send CMD8 SEND_IF_COND */
    SD_CmdTpye.Argument     = 0x000001AA;
    SD_CmdTpye.CmdIndex     = SD_CMD8_SEND_IF_COND;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R7 */
    lu32_ErrState = SD_GetCmdResp7(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_AllSendCID
 *
 * @brief   Ask any card to send the CID number on the CMD line.
 */
uint32_t SD_CMD_AllSendCID(struct_SD_t *SDx, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD2 ALL_SEND_CID */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD2_ALL_SEND_CID;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R2;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R2 */
    lu32_ErrState = SD_GetCmdResp2(SDx, fp32_Response);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SendCSD
 *
 * @brief   Addres card sends its card-specifc data(CSD) on the CMD line. 
 */
uint32_t SD_CMD_SendCSD(struct_SD_t *SDx, uint32_t fu32_Argument, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD9 SEND_CSD */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD9_SEND_CSD;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R2;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R2 */
    lu32_ErrState = SD_GetCmdResp2(SDx, fp32_Response);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_SendStatus
 *
 * @brief   Addres card sends its status register. 
 */
uint32_t SD_CMD_SendStatus(struct_SD_t *SDx, uint32_t fu32_Argument, uint32_t *fp_Resp)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD13 SEND_STATUS */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD13_SEND_STATUS;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */

    /* wait for response or any errors occur */
    while(!(__SD_GET_INT_STATUS(SDx) & (INT_CMD_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(SDx) & INT_ERR_MASK;
    }
    /* Receive the correct response */
    else if (__SD_GET_INT_STATUS(SDx) & INT_CMD_COMPLETE) 
    {
        /* Read response from register */
        *fp_Resp = __SD_GET_RESPONSE0(SDx);
        /* response with error */
        if (*fp_Resp & RESP1_ERR_ERRORBITS) 
        {
            lu32_ErrState = *fp_Resp;
        }
    }

    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(SDx);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_CMD_AppCommand
 *
 * @brief   Indicates to the card the next command is an application specifc command
 *          rather than a standard command. 
 */
uint32_t SD_CMD_AppCommand(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD55 APP_CMD */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SDMMC_CMD55_APP_CMD;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);
    
    return lu32_ErrState;
}


/************************************************************************************
 * @fn      SD_ACMD_SendOperCondition
 *
 * @brief   Sends host capacity support information(HCS) and asks the accessed card
 *          to send its operating condition register(OCR) content in the response
 *          on CMD line.
 */
uint32_t SD_ACMD_SendOperCondition(struct_SD_t *SDx, uint32_t fu32_Argument, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send ACMD41 SD_SEND_OP_COND */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SD_ACMD41_SD_SEND_OP_COND;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R3_R4;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R3 */
    lu32_ErrState = SD_GetCmdResp3(SDx, fp32_Response);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_ACMD_SetBusWidth
 *
 * @brief   Defines the data bus width (00 = 1bit or 10 = 4bit bus) to be used for 
 *          data transfer. The allowed data bus widths are given in SCR register.
 */
uint32_t SD_ACMD_SetBusWidth(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send ACMD6 SET_BUS_WIDTH */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = SD_ACMD6_SET_BUS_WIDTH;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SD_ACMD_SetBusWidth
 *
 * @brief   Reads the SD Configuration Register (SCR).
 */
uint32_t SD_ACMD_SendSCR(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);

    /* Send ACMD51 SEND_SCR */
    SD_CmdTpye.Argument     = 0x00000000;
    SD_CmdTpye.CmdIndex     = SD_ACMD51_SEND_SCR;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMC_CMD_SendOperCondition
 *
 * @brief   Sends host capacity support information and activates the card's 
 *          initialization process
 */
uint32_t MMC_CMD_SendOperCondition(struct_SD_t *SDx, uint32_t fu32_Argument, uint32_t *fp32_Response)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD1 SEND_OP_COND */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = MMC_CMD1_SEND_OP_COND;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R3_R4;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R3 */
    lu32_ErrState = SD_GetCmdResp3(SDx, fp32_Response);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMC_CMD_SetRelAddr
 *
 * @brief   Assigns relative address to the Device
 */
uint32_t MMC_CMD_SetRelAddr(struct_SD_t *SDx, uint32_t fu32_RCA)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Send CMD3 SET_RELATIVE_ADDR */
    SD_CmdTpye.Argument     = fu32_RCA;
    SD_CmdTpye.CmdIndex     = MMC_CMD3_SET_RELATIVE_ADDR;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMC_CMD_SendExtendedCSD
 *
 * @brief   The Device sends its EXT_CSD register as a block of data.
 */
uint32_t MMC_CMD_SendExtendedCSD(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);
    
    /* Send CMD8 SEND_EXT_CSD */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = MMC_CMD8_SEND_EXT_CSD;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMC_CMD_Switch
 *
 * @brief   Switches the mode of operation of the selected Device or 
 *          modifies the EXT_CSD registers
 */
uint32_t MMC_CMD_Switch(struct_SD_t *SDx, uint32_t fu32_Argument)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Argument: 
    - [31:26]: Reserved (shall be set to '0')
    - [25:24]: Access. 00: command set.
                       01: Set bits
                       10: Clear bits
                       11: Write Byte
    - [23:16]: Index
    - [15:8]:  Value
    - [7:3]: Reserved (shall be set to '0')
    - [2:0]: command set */
    
    /* Send CMD6 SWITCH */
    SD_CmdTpye.Argument     = fu32_Argument;
    SD_CmdTpye.CmdIndex     = MMC_CMD6_SWITCH;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = NO_DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMC_CMD_SendTuningBlock
 *
 * @brief   128 clocks of tuning pattern (64 byte in 4bit mode or 128byte in 8 bit 
 *          mode) is sent for HS200 optimal sampling point detection.
 */
uint32_t MMC_CMD_SendTuningBlock(struct_SD_t *SDx)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    SDIO_CmdTypeDef  SD_CmdTpye;

    /* Multi Block Transfer Disable */
    __SD_MULTI_BLOCK_DISABLE(SDx);
    /* Multi Block Transfer Count Disable */
    __SD_BLOCK_COUNT_DISABLE(SDx);
    /* DMA Disable */
    __SD_DMA_DISABLE(SDx);
    /* Direction: Read */
    __SD_DATA_DIRECTION(SDx, 1);

    /* Send CMD21 SEND_TUNING_BLOCK */
    SD_CmdTpye.Argument     = 0;
    SD_CmdTpye.CmdIndex     = MMC_CMD21_SEND_TUNING_BLOCK;
    SD_CmdTpye.CmdType      = CMD_TYPE_NORMAL;
    SD_CmdTpye.DataType     = DATA_PRESENT;
    SD_CmdTpye.ResponseType = RES_R1_R5_R6_R7;

    SD_SendCmd(SDx, &SD_CmdTpye);

    /* Waiting for R1 */
    lu32_ErrState = SD_GetCmdResp1(SDx);

    return lu32_ErrState;
}