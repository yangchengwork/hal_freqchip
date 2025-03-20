/*
  ******************************************************************************
  * @file    driver_sd_mmc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   Multi-Media card application HAL module driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/* Private function prototypes -----------------------------------------------*/
static uint32_t __eMMC_PowerON(MMC_HandleTypeDef *hmmc);
static uint32_t __eMMC_InitCard(MMC_HandleTypeDef *hmmc);
static void __MMCCard_GetCardCSD(MMC_HandleTypeDef *hmmc);
static void __MMCCard_GetCardCID(MMC_HandleTypeDef *hmmc);

/************************************************************************************
 * @fn      eMMC_IRQHandler
 *
 * @brief   eMMC interrupt handler.
 *
 * @param   hsd: Pointer to eMMC handle
 */
void eMMC_IRQHandler(MMC_HandleTypeDef *hmmc)
{
    switch (hmmc->eMMCStatus)
    {
        /* card read, write */
        case MMC_STATUS_READ_BUSY: 
        case MMC_STATUS_WRITE_BUSY:
        {
            /* DMA interrupt */
            if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_DMA_INT) 
            {
                __SD_CLR_INT_STATUS(hmmc->SDx, INT_DMA_INT);

                /* SDMA system Address alignment increases */
                hmmc->AddrAlign += SDMA_ADDR_UNIT;

                /* Update SDMA system Address */
                __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)hmmc->AddrAlign);
            }

            /* error */
            if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK) 
            {
                /* clear interrupt status */
                __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

                hmmc->eMMCStatus = MMC_STATUS_ERR;
            }

            /* transfer complete */
            if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_TRANSFER_COMPLETE)
            {
                /* clear transfer complete statsu */
                __SD_CLR_INT_STATUS(hmmc->SDx, INT_TRANSFER_COMPLETE);
                
                hmmc->eMMCStatus = MMC_STATUS_IDLE;
            }
        }break;
        
        default: break; 
    }
}

/************************************************************************************
 * @fn      eMMC_Init
 *
 * @brief   Initializes the Embedded Multi Media Card.
 *
 * @param   hmmc: Pointer to MMC handle
 */
uint32_t eMMC_Init(MMC_HandleTypeDef *hmmc)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    /* eMMC class initialization */
    SD_SDMMCClass_Init(hmmc->SDx);

    /* Identify card operating voltage */
    lu32_ErrState = __eMMC_PowerON(hmmc);
    if (lu32_ErrState)
    {
        return lu32_ErrState;
    }

    /* Card initialization */
    lu32_ErrState = __eMMC_InitCard(hmmc);
    if (lu32_ErrState)
    {
        return lu32_ErrState;
    }

    /* Get the card information from the CID register */
    __MMCCard_GetCardCID(hmmc);
    /* Get the card information from the CSD register */
    __MMCCard_GetCardCSD(hmmc);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_BusWidth_Select
 *
 * @brief   eMMC 1bit/4bit/8bit bus width select.
 *
 * @param   hmmc: Pointer to MMC handle.
 * @param   fu32_BusWidth: bus width. can select MMC_BUS_WIDTH_1BIT.
 *                                               MMC_BUS_WIDTH_4BIT.
 *                                               MMC_BUS_WIDTH_8BIT.
 */
uint32_t eMMC_BusWidth_Select(MMC_HandleTypeDef *hmmc, uint32_t fu32_BusWidth)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t fu32_Argument;

    /* Access */
    fu32_Argument = MMC_CMD6_ACCESS_WRITE_BYTE;
    /* Index */
    fu32_Argument |= MMC_EX_CSD_INDEX_BUS_WIDTH << 16;
    /* Value */
    fu32_Argument |= fu32_BusWidth << 8;

    /* Send CMD6 SWITCH */
    lu32_ErrState = MMC_CMD_Switch(hmmc->SDx, fu32_Argument);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    else 
    {
        /* Data0 line is in use */
        while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_DAT0_SIGNAL_MASK));
        
        /* Host set bus width */
        if (fu32_BusWidth == MMC_BUS_WIDTH_8BIT) 
        {
            /* Enable Bus width 8bit */
            __SD_8BIT_WIDTH_ENABLE(hmmc->SDx);
        }
        else 
        {
            /* Disable Bus width 8bit */
            __SD_8BIT_WIDTH_DISABLE(hmmc->SDx);

            if (fu32_BusWidth == MMC_BUS_WIDTH_4BIT)
                __SD_DATA_WIDTH_4BIT(hmmc->SDx);
            else
                __SD_DATA_WIDTH_1BIT(hmmc->SDx);
        }

        /* record Bus Width */
        hmmc->ExCSDInfo.BUS_WIDTH = fu32_BusWidth;
    }

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_Switch_HS200_Mode
 *
 * @brief   eMMC Switching to HS200 mode
 *
 * @param   hmmc: Pointer to MMC handle.
 */
uint32_t eMMC_Switch_HS200_Mode(MMC_HandleTypeDef *hmmc)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t fu32_Argument;

    /* HS200 mode must use 4 or 8 Bus Width*/
    if (hmmc->ExCSDInfo.BUS_WIDTH == MMC_BUS_WIDTH_1BIT)
        return ERR_HS200_BUS_WIDTH_ERR;
    /* Check whether the device supports HS200 */
    if ((hmmc->ExCSDInfo.DEVICE_TYPE & MMC_DEVICE_TPYE_HS200_1_8V) == 0)
        return ERR_HS200_NOT_SUPPORTED;

    /* Access */
    fu32_Argument = MMC_CMD6_ACCESS_WRITE_BYTE;
    /* Index */
    fu32_Argument |= MMC_EX_CSD_INDEX_HS_TIMING << 16;
    /* Value */
    fu32_Argument |= MMC_HS200_TIMING << 8;

    /* Send CMD6 SWITCH */
    lu32_ErrState = MMC_CMD_Switch(hmmc->SDx, fu32_Argument);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    else
    {
        /* Data0 line is in use */
        while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_DAT0_SIGNAL_MASK));

        /* Host switch 1.8V */
        __SD_1_8V_ENABLE(hmmc->SDx, 1);
        /* DDR50 */
        __SD_SDCLK_DISABLE(hmmc->SDx);
        __SD_UHS_MODE(hmmc->SDx, (SPEED_SDR12 & 0xF));
        __SD_SDCLK_ENABLE(hmmc->SDx);
    }

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_Switch_DDR_Mode
 *
 * @brief   eMMC Switching to dual data rate(DDR) mode.
 *
 * @param   hmmc: Pointer to MMC handle.
 * @param   fu32_DDRBusWidth: DDR bus width. can select MMC_BUS_WIDTH_4BIT_DDR.
 *                                                      MMC_BUS_WIDTH_8BIT_DDR.
 */
uint32_t eMMC_Switch_DDR_Mode(MMC_HandleTypeDef *hmmc, uint32_t fu32_DDRBusWidth)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t fu32_Argument;

    /* Check whether the device supports DDR */
    if ((hmmc->ExCSDInfo.DEVICE_TYPE & MMC_DEVICE_TPYE_HIGH_SPEED_DUAL_1_8V_3V) == 0)
        return ERR_DDR_NOT_SUPPORTED;

    /* Access */
    fu32_Argument = MMC_CMD6_ACCESS_WRITE_BYTE;
    /* Index */
    fu32_Argument |= MMC_EX_CSD_INDEX_HS_TIMING << 16;
    /* Value */
    fu32_Argument |= MMC_HIGH_SPEED_TIMING << 8;

    /* Send CMD6 SWITCH */
    lu32_ErrState = MMC_CMD_Switch(hmmc->SDx, fu32_Argument);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* Data0 line is in use */
    while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_DAT0_SIGNAL_MASK));

    /* Access */
    fu32_Argument = MMC_CMD6_ACCESS_WRITE_BYTE;
    /* Index */
    fu32_Argument |= MMC_EX_CSD_INDEX_BUS_WIDTH << 16;
    /* Value */
    fu32_Argument |= fu32_DDRBusWidth << 8;

    /* Send CMD6 SWITCH */
    lu32_ErrState = MMC_CMD_Switch(hmmc->SDx, fu32_Argument);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    else
    {
        /* Data0 line is in use */
        while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_DAT0_SIGNAL_MASK));

        /* Host set bus width */
        if (fu32_DDRBusWidth == MMC_BUS_WIDTH_8BIT_DDR) 
        {
            /* Enable Bus width 8bit */
            __SD_8BIT_WIDTH_ENABLE(hmmc->SDx);
        }
        else 
        {
            /* Disable Bus width 8bit */
            __SD_8BIT_WIDTH_DISABLE(hmmc->SDx);
            /* Select  Bus width 4bit */
            __SD_DATA_WIDTH_4BIT(hmmc->SDx);
        }

        /* Host switch 1.8V */
        __SD_1_8V_ENABLE(hmmc->SDx, 1);
        /* DDR50 */
        __SD_SDCLK_DISABLE(hmmc->SDx);
        __SD_UHS_MODE(hmmc->SDx, (SPEED_DDR50 & 0xF));
        __SD_SDCLK_ENABLE(hmmc->SDx);

        /* record Bus Width */
        hmmc->ExCSDInfo.BUS_WIDTH = fu32_DDRBusWidth;
    }

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_Execute_Tuning_Sequence
 *
 * @brief   eMMC Execute Tuning Sequence for HS200
 *
 * @param   hmmc: Pointer to MMC handle.
 */
uint32_t eMMC_Execute_Tuning_Sequence(MMC_HandleTypeDef *hmmc)
{
    int i, retry;
    uint32_t lu32_TampValue;
    uint32_t lu32_PatternLength;
    uint32_t lu32_PassCount, lu32_FristPoint, lu32_LastPoint;
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    if (hmmc->ExCSDInfo.BUS_WIDTH == MMC_BUS_WIDTH_4BIT)
        lu32_PatternLength = 64;
    else if (hmmc->ExCSDInfo.BUS_WIDTH == MMC_BUS_WIDTH_8BIT)
        lu32_PatternLength = 128;
    else
        return ERR_TUNING_BUS_WIDTH_ERR;

    #define EMMC_TUNING_CONSECUTIVE_SAMPLING_PASS_LIMIT    (4)

    /* tuning circuit reset */
    __SD_SAMPLING_CLOCK_SELECT(hmmc->SDx, 1);

    /* Set Pattern Block Size Byte */
    __SD_SET_BLOCK_SIZE(hmmc->SDx, lu32_PatternLength);

    /* consecutive pass count reset */
    lu32_PassCount = 0;

    for (retry = 0; retry < 32; retry++)
    {
        /* manual sampling delay set */
        __SD_MANUAL_SAMPLING_SET(hmmc->SDx, retry);

        /* Send CMD21 SEND_TUNING_BLOCK */
        lu32_ErrState = MMC_CMD_SendTuningBlock(hmmc->SDx);
        if (lu32_ErrState) 
        {
            /* Software Reset For DAT Line */
            /* Software Reset For CMD Line */
            __SD_RST_CMD_LINE(hmmc->SDx);
            __SD_RST_DAT_LINE(hmmc->SDx);

            lu32_LastPoint = retry - 1;

            if (lu32_PassCount >= EMMC_TUNING_CONSECUTIVE_SAMPLING_PASS_LIMIT)
            {
                /* Calculate the optimal value */
                lu32_TampValue = (lu32_FristPoint + lu32_LastPoint) / 2;
                /* manual sampling delay set */
                __SD_MANUAL_SAMPLING_SET(hmmc->SDx, lu32_TampValue);
                /* tuning success */
                break;
            }
            else
            {
                /* Error. consecutive pass count reset */
                lu32_PassCount  = 0;
                lu32_FristPoint = 0;
                lu32_LastPoint  = 0;
                continue;
            }
        }
        /* Wait for Buffer_Read_Ready */
        while(!(__SD_GET_INT_STATUS(hmmc->SDx) & INT_BUFFER_READ_READY));
        /* Clear Buffer_Read_Ready */
        __SD_CLR_INT_STATUS(hmmc->SDx, INT_BUFFER_READ_READY);
        /* Read pattern block */
        for (int k = 0; k < lu32_PatternLength / 4; k++)
        {
            lu32_TampValue = __SD_GET_BUFFERDATA(hmmc->SDx);
        }
        /* check error */
        if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
        {
            lu32_LastPoint = retry - 1;

            /* clear interrupt status */
            __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

            if (lu32_PassCount >= EMMC_TUNING_CONSECUTIVE_SAMPLING_PASS_LIMIT)
            {
                /* Calculate the optimal value */
                lu32_TampValue = (lu32_FristPoint + lu32_LastPoint) / 2;
                /* manual sampling delay set */
                __SD_MANUAL_SAMPLING_SET(hmmc->SDx, lu32_TampValue);
                /* tuning success */
                break;
            }
            else
            {
                /* Error. consecutive pass count reset */
                lu32_PassCount  = 0;
                lu32_FristPoint = 0;
                lu32_LastPoint  = 0;
                continue;
            }
        }
        if (lu32_PassCount == 0)
            lu32_FristPoint = retry;
        lu32_PassCount++;
    }

    if (retry >= 32)
    {
        lu32_ErrState = ERR_TUNING_ERR;
    }

    /* Init Block Size 512 Byte */
    __SD_SET_BLOCK_SIZE(hmmc->SDx, 512);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_ClockDiv_Updata
 *
 * @brief   Change the  eMMC clock frequency division.
 *
 * @param   hmmc: Pointer to MMC handle.
 * @param   fu8_ClockDiv: Clock division.
 */
void eMMC_ClockDiv_Updata(MMC_HandleTypeDef *hmmc, uint8_t fu8_ClockDiv)
{
    /* Update bus clock speed */
    __SD_CLOCK_DIV_LOW_8BIT(hmmc->SDx, (fu8_ClockDiv / 2));
}

/************************************************************************************
 * @fn      eMMC_ReadBolcks
 *
 * @brief   Reads block(s) from a specified address in a eMMC. 
 *          The Data transfer by polling mode.
 *
 * @param   hmmc: Pointer to eMMC handle.
 * @param   fp_Data: pointer to the received dat buffer.
 * @param   fu32_BlockAddr: Block Start Address.
 * @param   fu16_BlockNum: Number of eMMC blocks to read.
 */
uint32_t eMMC_ReadBolcks(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum)
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    if (fu16_BlockNum == 1) 
    {
        /* SEND CMD17 READ_SINGLI_BLOCK */
        lu32_ErrState = MMC_CMD_ReadSingleBlock(hmmc->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Wait for Buffer Read Ready Int */
        while(!(__SD_GET_INT_STATUS(hmmc->SDx) & INT_BUFFER_READ_READY));
        /* Clear Buffer_Read_Ready */
        __SD_CLR_INT_STATUS(hmmc->SDx, INT_BUFFER_READ_READY);
        /* Read one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            *fp_Data++ = __SD_GET_BUFFERDATA(hmmc->SDx);
        }
    }
    else 
    {
        /* SEND CMD23 SET_BLOCK_COUNT */
        lu32_ErrState = MMC_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Set block count */
        __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);

        /* SEND CMD18 READ_MULTIPLE_BLOCK */
        lu32_ErrState = MMC_CMD_ReadMultiBlock(hmmc->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        while (fu16_BlockNum--)
        {
            /* Wait for Buffer_Read_Ready */
            while(!(__SD_GET_INT_STATUS(hmmc->SDx) & INT_BUFFER_READ_READY));
            /* Clear Buffer_Read_Ready */
            __SD_CLR_INT_STATUS(hmmc->SDx, INT_BUFFER_READ_READY);
            /* Read one block */
            for (i = 0; i < BLOCKSIZE / 4; i++)
            {
                *fp_Data++ = __SD_GET_BUFFERDATA(hmmc->SDx);
            }
        }
    }

    /* wait for transfer complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(hmmc->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_WriteBolcks
 *
 * @brief   Write block(s) from a specified address in a eMMC. 
 *          The Data transfer by polling mode.
 *
 * @param   hmmc: Pointer to eMMC handle.
 * @param   fp_Data: pointer to the received dat buffer.
 * @param   fu32_BlockAddr: Block Start Address.
 * @param   fu16_BlockNum: Number of eMMC blocks to write.
 */
uint32_t eMMC_WriteBolcks(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum)
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    if (fu16_BlockNum == 1) 
    {
        /* SEND CMD24 WRITE_BLOCK */
        lu32_ErrState = MMC_CMD_WriteSingleBlock(hmmc->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Wait for buffer write enable Int */
        while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_BUFFER_WRITE_EN_MASK));
        /* write one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            __SD_SET_BUFFERDATA(hmmc->SDx, *fp_Data++);
        }
    }
    else 
    {
        /* SEND CMD23 SET_BLOCK_COUNT */
        lu32_ErrState = MMC_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Set block count */
        __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);

        /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
        lu32_ErrState = MMC_CMD_WriteMultiBlock(hmmc->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* First block transfer check the PresentState register */
        /* Wait for Buffer_Write_Enable */
        while(!(__SD_GET_PRESENT_STATE(hmmc->SDx) & PreState_BUFFER_WRITE_EN_MASK));
        /* write one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            __SD_SET_BUFFERDATA(hmmc->SDx, *fp_Data++);
        }
        fu16_BlockNum--;
        /* Other block transfer check the StatusInt register */
        while(fu16_BlockNum--)
        {
            /* Wait for Buffer_Write_Ready */
            while(!(__SD_GET_INT_STATUS(hmmc->SDx) & INT_BUFFER_WRITE_READY));
            /* Clear Buffer_Write_Ready */
            __SD_CLR_INT_STATUS(hmmc->SDx, INT_BUFFER_WRITE_READY);
            /* write one block */
            for (i = 0; i < BLOCKSIZE / 4; i++)
            {
                __SD_SET_BUFFERDATA(hmmc->SDx, *fp_Data++);
            }
        }
    }

    /* wait for transfer complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(hmmc->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hmmc->SDx);
        
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_ReadBolcks_SDMA
 *
 * @brief   Reads block(s) from a specified address in a eMMC. 
 *          The Data transfer by SDMA(Simple DMA) mode.
 *
 * @param   hmmc: Pointer to eMMC handle.
 * @param   fp_Data: pointer to the received dat buffer.
 * @param   fu32_BlockAddr: Block Start Address.
 * @param   fu16_BlockNum: Number of eMMC blocks to read.
 */
uint32_t eMMC_ReadBolcks_SDMA(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    bool lb_TrfComplete = true;
    uint32_t lu32_Addr = (uint32_t)fp_Data;

    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = MMC_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);

    /* SEND CMD18 READ_MULTIPLE_BLOCK */
    lu32_ErrState = MMC_CMD_ReadBlock_SDMA(hmmc->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* Address alignment */
    lu32_Addr &= SDMA_ADDR_ALIGN_MASK;

    /* wait for transfer complete or any errors occur */
    while (lb_TrfComplete)
    {
        if (__SD_GET_INT_STATUS(hmmc->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)) 
        {
            lb_TrfComplete = false;
        }

        if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_DMA_INT) 
        {
            __SD_CLR_INT_STATUS(hmmc->SDx, INT_DMA_INT);
            
            /* SDMA system Address alignment increases */
            lu32_Addr += SDMA_ADDR_UNIT;
            
            /* Update SDMA system Address */
            __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)lu32_Addr);
        }
    }

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_WriteBolcks_SDMA
 *
 * @brief   Write block(s) from a specified address in a eMMC. 
 *          The Data transfer by SDMA(Simple DMA) mode.
 *
 * @param   hmmc: Pointer to eMMC handle.
 * @param   fp_Data: pointer to the received dat buffer.
 * @param   fu32_BlockAddr: Block Start Address.
 * @param   fu16_BlockNum: Number of eMMC blocks to write.
 */
uint32_t eMMC_WriteBolcks_SDMA(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    bool lb_TrfComplete = true;
    uint32_t lu32_Addr = (uint32_t)fp_Data;

    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = MMC_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);

    /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
    lu32_ErrState = MMC_CMD_WriteBlock_SDMA(hmmc->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
        
    /* Address alignment */
    lu32_Addr &= SDMA_ADDR_ALIGN_MASK;

    /* wait for transfer complete or any errors occur */
    while (lb_TrfComplete)
    {
        if (__SD_GET_INT_STATUS(hmmc->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)) 
        {
            lb_TrfComplete = false;
        }

        if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_DMA_INT) 
        {
            __SD_CLR_INT_STATUS(hmmc->SDx, INT_DMA_INT);
            
            /* SDMA system Address alignment increases */
            lu32_Addr += SDMA_ADDR_UNIT;
            
            /* Update SDMA system Address */
            __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)lu32_Addr);
        }
    }

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      eMMC_ReadBolcks_SDMA_IT
 *
 * @brief   Reads block(s) from a specified address in a eMMC. 
 *          The Data transfer by SDMA(Simple DMA) mode with interrupt.
 *
 * @param   hmmc: Pointer to eMMC handle.
 * @param   fp_Data: pointer to the received dat buffer.
 * @param   fu32_BlockAddr: Block Start Address.
 * @param   fu16_BlockNum: Number of eMMC blocks to read.
 */
uint32_t eMMC_ReadBolcks_SDMA_IT(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    if (hmmc->eMMCStatus != MMC_STATUS_IDLE) 
        return ERR_EMMC_BUSY;

    if (fu16_BlockNum == 0)
        return ERR_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);

    /* Address alignment */
    hmmc->AddrAlign = (uint32_t)fp_Data & SDMA_ADDR_ALIGN_MASK;

    /* Enable error/transfer complete/dma */
    __SD_INT_ENABLE(hmmc->SDx, INT_DMA_INT | INT_TRANSFER_COMPLETE | INT_ERR_MASK);

    /* SEND CMD18 READ_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_ReadBlock_SDMA(hmmc->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
    else 
        hmmc->eMMCStatus = MMC_STATUS_READ_BUSY;

    return lu32_ErrState;
}

/************************************************************************************
* @fn      eMMC_WriteBolcks_SDMA_IT
*
* @brief   Write block(s) from a specified address in a eMMC. 
*          The Data transfer by SDMA(Simple DMA) mode with interrupt.
*
* @param   hsd: Pointer to eMMC handle.
* @param   fp_Data: pointer to the received dat buffer.
* @param   fu32_BlockAddr: Block Address.
* @param   fu16_BlockNum: Number of eMMC blocks to write.
*/
uint32_t eMMC_WriteBolcks_SDMA_IT(MMC_HandleTypeDef *hmmc, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    if (hmmc->eMMCStatus != MMC_STATUS_IDLE) 
        return ERR_EMMC_BUSY;
        
    if (fu16_BlockNum == 0)
        return ERR_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hmmc->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hmmc->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hmmc->SDx, fu16_BlockNum);
    
    /* Address alignment */
    hmmc->AddrAlign = (uint32_t)fp_Data & SDMA_ADDR_ALIGN_MASK;

    /* Enable error/transfer complete/dma */
    __SD_INT_ENABLE(hmmc->SDx, INT_DMA_INT | INT_TRANSFER_COMPLETE | INT_ERR_MASK);

    /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_WriteBlock_SDMA(hmmc->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
    else 
        hmmc->eMMCStatus = MMC_STATUS_WRITE_BUSY;

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      MMCCard_GetCardCSD
 *
 * @brief   Get the card information from the CSD register.
 */
static void __MMCCard_GetCardCSD(MMC_HandleTypeDef *hmmc)
{
    hmmc->CSDInfo = (MMC_CardCSDTypeDef *)hmmc->CSD;
    
    /* Card memory capacity, unit KByte */
    hmmc->CardInfo.MemoryCapacity = hmmc->ExCSDInfo.SEC_COUNT / 2;
    /* Card command class */
    hmmc->CardInfo.Class = hmmc->CSDInfo->CCC;
}

/************************************************************************************
 * @fn      __MMCCard_GetCardCID
 *
 * @brief   Get the card information from the CID register.
 */
static void __MMCCard_GetCardCID(MMC_HandleTypeDef *hmmc)
{
    hmmc->CIDInfo = (MMC_CardCIDTypeDef *)hmmc->CID;
}

/************************************************************************************
 * @fn      __eMMC_PowerON
 *
 * @brief   Enquires cards about their operating voltage and configures clock ontrols.
 */
static uint32_t __eMMC_PowerON(MMC_HandleTypeDef *hmmc)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    bool lb_BusyStatus = false;
    
    /* CMD0: GO_IDLE_STATE */
    lu32_ErrState = MMC_CMD_GoIdleState(hmmc->SDx);
    if(lu32_ErrState)
    {
        return lu32_ErrState;
    }

    while (lb_BusyStatus == false)
    {
        /* SEND CMD1 SEND_OP_COND with eMMC_DUAL_VOLTAGE_RANGE(0xC0FF8080U) as argument */
        lu32_ErrState = MMC_CMD_SendOperCondition(hmmc->SDx, eMMC_DUAL_VOLTAGE_RANGE, &hmmc->OCR);
        if(lu32_ErrState)
        {
            return lu32_ErrState;
        }

        lb_BusyStatus = hmmc->OCR & OCR_BUSY ? true : false;
    }

    hmmc->CardInfo.CardType = 0;

    /* After power routine and command returns valid voltage */
    if (hmmc->OCR & eMMC_DUAL_VOLTAGE_CARD) 
    {
        /* Voltage range of the card is within 1.70V ~ 1.95V or 2.7V ~ 3.6V */
        hmmc->CardInfo.CardType = eMMC_DUAL_VOLTAGE_CARD;
    }

    if (hmmc->OCR & eMMC_CAPACITY_HIGHER_2G) 
    {
        /* Capacity > 2G, sector mode */
        hmmc->CardInfo.CardType |= eMMC_CAPACITY_HIGHER_2G;
    }

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      __eMMC_InitCard
 *
 * @brief   Initializes the eMMC.
 */
static uint32_t __eMMC_InitCard(MMC_HandleTypeDef *hmmc)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t lu32_Response, lu32_TempValue;

    /* CMD2: ALL_SEND_CID */
    lu32_ErrState = MMC_CMD_AllSendCID(hmmc->SDx, hmmc->CID);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD3: SET_RELATIVE_ADDR */
    hmmc->RCA = 0xF1170000;
    lu32_ErrState = MMC_CMD_SetRelAddr(hmmc->SDx, hmmc->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* After CMD3, Bus select push-pull mode */
    __SD_MMC_OD_PP(hmmc->SDx, 1);

    /* CMD9: SEND_CSD */
    lu32_ErrState = MMC_CMD_SendCSD(hmmc->SDx, hmmc->RCA, hmmc->CSD);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD7: SEL_DESEL_CARD */
    lu32_ErrState = MMC_CMD_SelectCard(hmmc->SDx, hmmc->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* Wait to enter transfer state */
    uint32_t lu32_Timeout = 100;

    while (lu32_Timeout--)
    {
        /* CMD13: SEND_STATUS */
        lu32_ErrState = MMC_CMD_SendStatus(hmmc->SDx, hmmc->RCA, &lu32_Response);
        if (lu32_ErrState)
        {
            return lu32_ErrState;
        }
        if (lu32_Response & CARD_CURRENT_STATE_TRAN)
            break;
    }

    /* CMD8: SEND_EXT_CSD */
    lu32_ErrState = MMC_CMD_SendExtendedCSD(hmmc->SDx);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* Wait for Buffer Read Ready Int */
    while(!(__SD_GET_INT_STATUS(hmmc->SDx) & INT_BUFFER_READ_READY));
    /* Clear Buffer_Read_Ready */
    __SD_CLR_INT_STATUS(hmmc->SDx, INT_BUFFER_READ_READY);
    /* Read one block */
    for (int i = 0; i < BLOCKSIZE / 4; i++)
    {
        lu32_TempValue = __SD_GET_BUFFERDATA(hmmc->SDx);

        /* Extract Parameter */
        switch (i)
        {
            case MMC_EX_CSD_INDEX_SEC_COUNT0 / 4: 
            {   /* Byte 212,213,214,215 */
                hmmc->ExCSDInfo.SEC_COUNT = lu32_TempValue;
            }break;

            case MMC_EX_CSD_INDEX_BUS_WIDTH / 4:
            {   /* Byte 183 */
                hmmc->ExCSDInfo.BUS_WIDTH = lu32_TempValue >> 24 & 0xFF;
            }break;

            case MMC_EX_CSD_INDEX_HS_TIMING / 4:
            {   /* Byte 185 */
                hmmc->ExCSDInfo.HS_TIMING = lu32_TempValue >> 8 & 0xFF;
            }break;

            case MMC_EX_CSD_INDEX_EXT_CSD_REV / 4:
            {   /* Byte 192 */
                hmmc->ExCSDInfo.EXT_CSD_REV = lu32_TempValue & 0xFF;
            }break;

            case MMC_EX_CSD_INDEX_DEVICE_TYPE / 4:
            {   /* Byte 196 */
                hmmc->ExCSDInfo.DEVICE_TYPE = lu32_TempValue & 0xFF;
            }break;

            default: break; 
        }
    }

    /* wait for transfer complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(hmmc->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hmmc->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hmmc->SDx);

    /* Update bus clock speed */
    __SD_CLOCK_DIV_LOW_8BIT(hmmc->SDx, (hmmc->Init.ClockDiv / 2));

    return lu32_ErrState;
}
