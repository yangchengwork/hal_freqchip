/*
  ******************************************************************************
  * @file    driver_sd_card.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   SD card application HAL module driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/* Private function prototypes -----------------------------------------------*/
static uint32_t __SDCard_PowerON(SD_HandleTypeDef *hsd);
static uint32_t __SDCard_InitCard(SD_HandleTypeDef *hsd);
static void __SDCard_GetCardCSD(SD_HandleTypeDef *hsd);
static void __SDCard_GetCardCID(SD_HandleTypeDef *hsd);
static void __SDCard_GetCardSCR(SD_HandleTypeDef *hsd);

/************************************************************************************
 * @fn      SDCard_IRQHandler
 *
 * @brief   SDCard interrupt handler.
 *
 * @param   hsd: Pointer to SD handle
 */
void SDCard_IRQHandler(SD_HandleTypeDef *hsd)
{
    switch (hsd->CardStatus)
    {
        /* card read, write */
        case CARD_STATUS_READ_BUSY: 
        case CARD_STATUS_WRITE_BUSY:
        {
            /* DMA interrupt */
            if (__SD_GET_INT_STATUS(hsd->SDx) & INT_DMA_INT) 
            {
                __SD_CLR_INT_STATUS(hsd->SDx, INT_DMA_INT);

                /* SDMA system Address alignment increases */
                hsd->AddrAlign += SDMA_ADDR_UNIT;

                /* Update SDMA system Address */
                __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)hsd->AddrAlign);
            }

            /* error */
            if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK) 
            {
                /* clear interrupt status */
                __SD_CLR_ALL_INT_STATUS(hsd->SDx);

                hsd->CardStatus = CARD_STATUS_ERR;
            }

            /* transfer complete */
            if (__SD_GET_INT_STATUS(hsd->SDx) & INT_TRANSFER_COMPLETE)
            {
                /* clear transfer complete statsu */
                __SD_CLR_INT_STATUS(hsd->SDx, INT_TRANSFER_COMPLETE);
                
                hsd->CardStatus = CARD_STATUS_IDLE;
            }
        }break;
        
        default: break; 
    }
}

/************************************************************************************
 * @fn      SDCard_Init
 *
 * @brief   Initializes the SD Card.
 *
 * @param   hsd: Pointer to SD handle
 */
uint32_t SDCard_Init(SD_HandleTypeDef *hsd)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* SD Crad class initialization */
    SD_SDCardClass_Init(hsd->SDx);
    
    for(volatile uint32_t i=0; i<10000; i++);
    
    /* Identify card operating voltage */
    lu32_ErrState = __SDCard_PowerON(hsd);
    if (lu32_ErrState)
    {
        return lu32_ErrState;
    }

    /* Card initialization */
    lu32_ErrState = __SDCard_InitCard(hsd);
    if (lu32_ErrState)
    {
        return lu32_ErrState;
    }

    /* Get the card information from the CID register */
    __SDCard_GetCardCID(hsd);
    /* Get the card information from the CSD register */
    __SDCard_GetCardCSD(hsd);
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_BusWidth_Select
 *
 * @brief   SDCard 1bit/4bit bus width select.
 *
 * @param   hsd: Pointer to SD handle.
 *          fu32_BusWidth: bus width. can select SDIO_BUS_WIDTH_1BIT.
 *                                               SDIO_BUS_WIDTH_4BIT.
 */
uint32_t SDCard_BusWidth_Select(SD_HandleTypeDef *hsd, uint32_t fu32_BusWidth)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* SEND CMD55 APP_CMD with RCA as 0 */
    lu32_ErrState = SD_CMD_AppCommand(hsd->SDx, hsd->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    
    /* Send ACMD6 SET_BUS_WIDTH */
    lu32_ErrState = SD_ACMD_SetBusWidth(hsd->SDx, fu32_BusWidth);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    else 
    {
        if (fu32_BusWidth) 
            __SD_DATA_WIDTH_4BIT(hsd->SDx);
        else
            __SD_DATA_WIDTH_1BIT(hsd->SDx);
    }
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_GetCardSCR
 *
 * @brief   Get SDCard SCR information.
 *
 * @param   hsd: Pointer to SD handle.
 */
uint32_t SDCard_GetCardSCR(SD_HandleTypeDef *hsd)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* SEND CMD55 APP_CMD with RCA as 0 */
    lu32_ErrState = SD_CMD_AppCommand(hsd->SDx, hsd->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* recieve SCR Block 8 Byte */
    __SD_SET_BLOCK_SIZE(hsd->SDx, 8);

    /* Send ACMD51 SEND_SCR */
    lu32_ErrState = SD_ACMD_SendSCR(hsd->SDx);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    if (lu32_ErrState == INT_NO_ERR) 
    {
        /* Wait for Buffer Read Ready Int */
        while(!(__SD_GET_INT_STATUS(hsd->SDx) & INT_BUFFER_READ_READY));
        /* Clear Buffer_Read_Ready */
        __SD_CLR_INT_STATUS(hsd->SDx, INT_BUFFER_READ_READY);
        /* Read one block */
        for (int i = 0; i < 2; i++)
        {
            hsd->SCR[i] = __SD_GET_BUFFERDATA(hsd->SDx);
        }

        /* wait for transfer complete or any errors occur */
        while(!(__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));
        /* Any errors occur */
        if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
        {
            return (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK);
        }
        /* clear interrupt status */
        __SD_CLR_ALL_INT_STATUS(hsd->SDx);

        /* Get Card SCR info */
        __SDCard_GetCardSCR(hsd);

        if (hsd->SCRInfo->SD_SPEC == 0)
            hsd->CardInfo.SpecVersion = CARD_PHY_LAYER_SPEC_VERSION_V101;
        else if (hsd->SCRInfo->SD_SPEC == 1)
            hsd->CardInfo.SpecVersion = CARD_PHY_LAYER_SPEC_VERSION_V110;
        else if (hsd->SCRInfo->SD_SPEC == 2 && hsd->SCRInfo->SD_SPEC3 == 0)
            hsd->CardInfo.SpecVersion = CARD_PHY_LAYER_SPEC_VERSION_V200;
        else if (hsd->SCRInfo->SD_SPEC == 2 && hsd->SCRInfo->SD_SPEC3 == 1)
            hsd->CardInfo.SpecVersion = CARD_PHY_LAYER_SPEC_VERSION_V30x;
    }

    /* Fixed Block Size 512 Byte */
    __SD_SET_BLOCK_SIZE(hsd->SDx, 512);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_ReadBolcks
 *
 * @brief   Reads block(s) from a specified address in a card. 
 *          The Data transfer by polling mode.
 *
 * @param   hsd: Pointer to SD handle.
 *          fp_Data: pointer to the received dat buffer.
 *          fu32_BlockAddr: Block Start Address.
 *          fu16_BlockNum: Number of SD blocks to read.
 */
uint32_t SDCard_ReadBolcks(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum)
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    if (fu16_BlockNum == 1) 
    {
        /* SEND CMD17 READ_SINGLI_BLOCK */
        lu32_ErrState = SD_CMD_ReadSingleBlock(hsd->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Wait for Buffer Read Ready Int */
        while(!(__SD_GET_INT_STATUS(hsd->SDx) & INT_BUFFER_READ_READY));
        /* Clear Buffer_Read_Ready */
        __SD_CLR_INT_STATUS(hsd->SDx, INT_BUFFER_READ_READY);
        /* Read one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            *fp_Data++ = __SD_GET_BUFFERDATA(hsd->SDx);
        }
    }
    else 
    {
        /* SEND CMD23 SET_BLOCK_COUNT */
        lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Set block count */
        __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);

        /* SEND CMD18 READ_MULTIPLE_BLOCK */
        lu32_ErrState = SD_CMD_ReadMultiBlock(hsd->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        while (fu16_BlockNum--)
        {
            /* Wait for Buffer_Read_Ready */
            while(!(__SD_GET_INT_STATUS(hsd->SDx) & INT_BUFFER_READ_READY));
            /* Clear Buffer_Read_Ready */
            __SD_CLR_INT_STATUS(hsd->SDx, INT_BUFFER_READ_READY);
            /* Read one block */
            for (i = 0; i < BLOCKSIZE / 4; i++)
            {
                *fp_Data++ = __SD_GET_BUFFERDATA(hsd->SDx);
            }
        }
    }
    
    /* wait for transfer complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hsd->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_WriteBolcks
 *
 * @brief   Write block(s) from a specified address in a card. 
 *          The Data transfer by polling mode.
 *
 * @param   hsd: Pointer to SD handle.
 *          fp_Data: pointer to the received dat buffer.
 *          fu32_BlockAddr: Block Address.
 *          fu16_BlockNum: Number of SD blocks to write.
 */
uint32_t SDCard_WriteBolcks(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum)
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;
    
    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    if (fu16_BlockNum == 1) 
    {
        /* SEND CMD24 WRITE_BLOCK */
        lu32_ErrState = SD_CMD_WriteSingleBlock(hsd->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;

        /* Wait for buffer write enable Int */
        while(!(__SD_GET_PRESENT_STATE(hsd->SDx) & PreState_BUFFER_WRITE_EN_MASK));
        /* write one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            __SD_SET_BUFFERDATA(hsd->SDx, *fp_Data++);
        }
    }
    else 
    {
        /* SEND CMD23 SET_BLOCK_COUNT */
        lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
        if (lu32_ErrState)
            return lu32_ErrState;
        
        /* Set block count */
        __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);
        
        /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
        lu32_ErrState = SD_CMD_WriteMultiBlock(hsd->SDx, fu32_BlockAddr);
        if (lu32_ErrState)
            return lu32_ErrState;
        
        /* First block transfer check the PresentState register */
        /* Wait for Buffer_Write_Enable */
        while(!(__SD_GET_PRESENT_STATE(hsd->SDx) & PreState_BUFFER_WRITE_EN_MASK));
        /* write one block */
        for (i = 0; i < BLOCKSIZE / 4; i++)
        {
            __SD_SET_BUFFERDATA(hsd->SDx, *fp_Data++);
        }
        fu16_BlockNum--;
        /* Other block transfer check the StatusInt register */
        while(fu16_BlockNum--)
        {
            /* Wait for Buffer_Write_Ready */
            while(!(__SD_GET_INT_STATUS(hsd->SDx) & INT_BUFFER_WRITE_READY));
            /* Clear Buffer_Write_Ready */
            __SD_CLR_INT_STATUS(hsd->SDx, INT_BUFFER_WRITE_READY);
            /* write one block */
            for (i = 0; i < BLOCKSIZE / 4; i++)
            {
                __SD_SET_BUFFERDATA(hsd->SDx, *fp_Data++);
            }
        }
    }

    /* wait for transfer complete or any errors occur */
    while(!(__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hsd->SDx);
        
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_ReadBolcks_SDMA
 *
 * @brief   Reads block(s) from a specified address in a card. 
 *          The Data transfer by SDMA(Simple DMA) mode.
 *
 * @param   hsd: Pointer to SD handle.
 *          fp_Data: pointer to the received dat buffer.
 *          fu32_BlockAddr: Block Start Address.
 *          fu16_BlockNum: Number of SD blocks to read.
 */
uint32_t SDCard_ReadBolcks_SDMA(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    bool lb_TrfComplete = true;
    uint32_t lu32_Addr = (uint32_t)fp_Data;

    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);

    /* SEND CMD18 READ_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_ReadBlock_SDMA(hsd->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
        
    /* Address alignment */
    lu32_Addr &= SDMA_ADDR_ALIGN_MASK;

    /* wait for transfer complete or any errors occur */
    while (lb_TrfComplete)
    {
        if (__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)) 
        {
            lb_TrfComplete = false;
        }

        if (__SD_GET_INT_STATUS(hsd->SDx) & INT_DMA_INT) 
        {
            __SD_CLR_INT_STATUS(hsd->SDx, INT_DMA_INT);

            /* SDMA system Address alignment increases */
            lu32_Addr += SDMA_ADDR_UNIT;

            /* Update SDMA system Address */
            __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)lu32_Addr);
        }
    }

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hsd->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_WriteBolcks_SDMA
 *
 * @brief   Write block(s) from a specified address in a card. 
 *          The Data transfer by SDMA(Simple DMA) mode.
 *
 * @param   hsd: Pointer to SD handle.
 *          fp_Data: pointer to the received dat buffer.
 *          fu32_BlockAddr: Block Address.
 *          fu16_BlockNum: Number of SD blocks to write.
 */
uint32_t SDCard_WriteBolcks_SDMA(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    bool lb_TrfComplete = true;
    uint32_t lu32_Addr = (uint32_t)fp_Data;

    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);

    /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_WriteBlock_SDMA(hsd->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
        
    /* Address alignment */
    lu32_Addr &= SDMA_ADDR_ALIGN_MASK;

    /* wait for transfer complete or any errors occur */
    while (lb_TrfComplete)
    {
        if (__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)) 
        {
            lb_TrfComplete = false;
        }

        if (__SD_GET_INT_STATUS(hsd->SDx) & INT_DMA_INT) 
        {
            __SD_CLR_INT_STATUS(hsd->SDx, INT_DMA_INT);
            
            /* SDMA system Address alignment increases */
            lu32_Addr += SDMA_ADDR_UNIT;
            
            /* Update SDMA system Address */
            __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)lu32_Addr);
        }
    }

    /* Any errors occur */
    if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
    {
        lu32_ErrState = __SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK;
    }
    /* clear interrupt status */
    __SD_CLR_ALL_INT_STATUS(hsd->SDx);

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_ReadBolcks_SDMA_IT
 *
 * @brief   Reads block(s) from a specified address in a card. 
 *          The Data transfer by SDMA(Simple DMA) mode with interrupt.
 *
 * @param   hsd: Pointer to SD handle.
 *          fp_Data: pointer to the received dat buffer.
 *          fu32_BlockAddr: Block Start Address.
 *          fu16_BlockNum: Number of SD blocks to read.
 */
uint32_t SDCard_ReadBolcks_SDMA_IT(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    if (hsd->CardStatus != CARD_STATUS_IDLE) 
        return E4_CARD_BUSY;

    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);

    /* Address alignment */
    hsd->AddrAlign = (uint32_t)fp_Data & SDMA_ADDR_ALIGN_MASK;

    /* Enable error/transfer complete/dma */
    __SD_INT_ENABLE(hsd->SDx, INT_DMA_INT | INT_TRANSFER_COMPLETE | INT_ERR_MASK);

    /* SEND CMD18 READ_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_ReadBlock_SDMA(hsd->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
    else 
        hsd->CardStatus = CARD_STATUS_READ_BUSY;

    return lu32_ErrState;
}

/************************************************************************************
* @fn      SDCard_WriteBolcks_SDMA_IT
*
* @brief   Write block(s) from a specified address in a card. 
*          The Data transfer by SDMA(Simple DMA) mode with interrupt.
*
* @param   hsd: Pointer to SD handle.
*          fp_Data: pointer to the received dat buffer.
*          fu32_BlockAddr: Block Address.
*          fu16_BlockNum: Number of SD blocks to write.
*/
uint32_t SDCard_WriteBolcks_SDMA_IT(SD_HandleTypeDef *hsd, uint32_t *fp_Data, uint32_t fu32_BlockAddr, uint16_t fu16_BlockNum) 
{
    int i;
    uint32_t lu32_ErrState = INT_NO_ERR;

    if (hsd->CardStatus != CARD_STATUS_IDLE) 
        return E4_CARD_BUSY;
        
    if (fu16_BlockNum == 0)
        return E1_NUM_ERR;

    /* SEND CMD23 SET_BLOCK_COUNT */
    lu32_ErrState = SD_CMD_SetBlockCount(hsd->SDx, fu16_BlockNum);
    if (lu32_ErrState)
        return lu32_ErrState;

    /* set SDMA system Address */
    __SD_SET_SDMA_SYSTEM_ADDR(hsd->SDx, (uint32_t)fp_Data);
    /* Set block count */
    __SD_SET_BLOCK_COUNT(hsd->SDx, fu16_BlockNum);
    
    /* Address alignment */
    hsd->AddrAlign = (uint32_t)fp_Data & SDMA_ADDR_ALIGN_MASK;

    /* Enable error/transfer complete/dma */
    __SD_INT_ENABLE(hsd->SDx, INT_DMA_INT | INT_TRANSFER_COMPLETE | INT_ERR_MASK);

    /* SEND CMD25 WRITE_MULTIPLE_BLOCK */
    lu32_ErrState = SD_CMD_WriteBlock_SDMA(hsd->SDx, fu32_BlockAddr);
    if (lu32_ErrState)
        return lu32_ErrState;
    else 
        hsd->CardStatus = CARD_STATUS_WRITE_BUSY;

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_Erase
 *
 * @brief   Write block(s) from a specified address in a card. 
 *          The Data transfer by polling mode.
 *
 * @param   hsd: Pointer to SD handle.
 *          BlockStartAdd: Start Block address
 *          BlockEndAdd:   End Block address
 */
uint32_t SDCard_Erase(SD_HandleTypeDef *hsd, uint32_t BlockStartAddr, uint32_t BlockEndAddr)
{
    uint32_t lu32_ErrState = INT_NO_ERR;

    /* CMD32: ERASE_WR_BLK_START */
    lu32_ErrState = SD_CMD_EraseStartAddr(hsd->SDx, BlockStartAddr);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    
    /* CMD33: ERASE_WR_BLK_END */
    lu32_ErrState = SD_CMD_EraseEndAddr(hsd->SDx, BlockEndAddr);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    
    /* CMD38: ERASE */
    lu32_ErrState = SD_CMD_Erase(hsd->SDx);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }
    
    return lu32_ErrState;
}

/************************************************************************************
 * @fn      SDCard_Get_Block_count
 *
 * @brief   Get block counter of mounted SD Card.
 *
 * @param   hsd: Pointer to SD handle.
 *
 * @return  Block count.
 */
uint32_t SDCard_Get_Block_count(SD_HandleTypeDef *hsd)
{
    if (hsd) {
        return hsd->CardInfo.MemoryCapacity / 512;
    }
    else {
        return 0;
    }
}

/************************************************************************************
 * @fn      SDCard_GetCardCSD
 *
 * @brief   Get the card information from the CSD register.
 */
static void __SDCard_GetCardCSD(SD_HandleTypeDef *hsd)
{
    hsd->CSDInfo = (SD_CardCSDTypeDef *)hsd->CSD;

    /* Card memory capacity, unit KByte */
    hsd->CardInfo.MemoryCapacity = (hsd->CSDInfo->C_SIZE + 1) * 512;
    /* Card command class */
    hsd->CardInfo.Class = hsd->CSDInfo->CCC;
} 

/************************************************************************************
 * @fn      SDCard_GetCardCID
 *
 * @brief   Get the card information from the CID register.
 */
static void __SDCard_GetCardCID(SD_HandleTypeDef *hsd)
{
    hsd->CIDInfo = (SD_CardCIDTypeDef *)hsd->CID;
}

/************************************************************************************
 * @fn      SDCard_GetCardSCR
 *
 * @brief   Get the card information from the SCR register.
 */
static void __SDCard_GetCardSCR(SD_HandleTypeDef *hsd)
{
    uint8_t *Point = (uint8_t *)hsd->SCR;
    uint32_t SCRBuffer = (Point[0] << 24) | (Point[1] << 16) | (Point[2] << 8) | Point[3];
    hsd->SCR[0] = SCRBuffer;

    hsd->SCRInfo = (SD_CardSCRTypeDef *)hsd->SCR;
}

/************************************************************************************
 * @fn      SD_PowerON
 *
 * @brief   Enquires cards about their operating voltage and configures clock ontrols.
 */
static uint32_t __SDCard_PowerON(SD_HandleTypeDef *hsd)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t lu32_Response;
    bool lb_BusyStatus = false;

    /* CMD0: GO_IDLE_STATE */
    lu32_ErrState = SD_CMD_GoIdleState(hsd->SDx);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD8: SEND_IF_COND: Command available only on V2.0 cards */
    lu32_ErrState = SD_CMD_SendInterfaceCondition(hsd->SDx);
    if (lu32_ErrState) 
    {
        hsd->CardInfo.CardVersion = CARD_VER_1_X;

        return lu32_ErrState;
    }
    else 
    {
        hsd->CardInfo.CardVersion = CARD_VER_2_X;
        
        /* signaling 3.3V */
        if (hsd->Init.SpeedMode & SIGNALING_3_3V_MASK) 
        {
            while (lb_BusyStatus == false) 
            {
                /* Send CMD55 APP_CMD with RCA as 0 */
                lu32_ErrState = SD_CMD_AppCommand(hsd->SDx, 0);
                if (lu32_ErrState) 
                {
                    return lu32_ErrState;
                }

                /* Send ACMD41 SD_SEND_OP_COND */
                lu32_ErrState = SD_ACMD_SendOperCondition(hsd->SDx, ACMD41_ARG_HCS|ACMD41_ARG_VOLTAGE_WINDOW_32_33, &hsd->OCR);
                if (lu32_ErrState) 
                {
                    return lu32_ErrState;
                }

                lb_BusyStatus = hsd->OCR & OCR_BUSY ? true : false;
            }

            if (hsd->OCR & OCR_CCS) 
            {
                hsd->CardInfo.CardType = CARD_TYPE_SDHC_SDXC;
            } 
            else 
            {
                hsd->CardInfo.CardType = CARD_TYPE_SDSC;
            } 
        }
        /* signaling 1.8V */
        else 
        {
            while (lb_BusyStatus == false) 
            {
                /* SEND CMD55 APP_CMD with RCA as 0 */
                lu32_ErrState = SD_CMD_AppCommand(hsd->SDx, 0);
                if (lu32_ErrState) 
                {
                    return lu32_ErrState;
                }

                /* Send ACMD41 SD_SEND_OP_COND */
                lu32_ErrState = SD_ACMD_SendOperCondition(hsd->SDx, ACMD41_ARG_HCS|ACMD41_ARG_S18R|ACMD41_ARG_VOLTAGE_WINDOW_32_33, &hsd->OCR);
                if (lu32_ErrState) 
                {
                    return lu32_ErrState;
                }

                lb_BusyStatus = hsd->OCR & OCR_BUSY ? true : false;
            }

            /* Card Capacity Status */
            if (hsd->OCR & OCR_CCS) 
            {
                hsd->CardInfo.CardType = CARD_TYPE_SDHC_SDXC;
            } 
            else 
            {
                hsd->CardInfo.CardType = CARD_TYPE_SDSC;
            } 

            /* switching to 1.8V Accepted */
            if (hsd->OCR & OCR_S18A) 
            {
                /* SEND CMD11 VOLTAGE_SWITCH */
                lu32_ErrState = SD_CMD_VoltageSwitch(hsd->SDx);

                if (lu32_ErrState == INT_NO_ERR) 
                {
                    /* Data0 line is in use */
                    while(!(__SD_GET_PRESENT_STATE(hsd->SDx) & PreState_DAT0_SIGNAL_MASK));

                    /* Host switch 1.8V */
                    __SD_1_8V_ENABLE(hsd->SDx, 1);
                    /* Host default select SDR12 */
                    __SD_SDCLK_DISABLE(hsd->SDx);
                    __SD_UHS_MODE(hsd->SDx, 0);
                    __SD_SDCLK_ENABLE(hsd->SDx);


                    /* Wait to Card ready */
                    uint32_t lu32_Timeout = 100;

                    while (lu32_Timeout--)
                    {
                        if (!SD_CMD_SendStatus(hsd->SDx, hsd->RCA, &lu32_Response)) 
                        {
                            break;
                        }
                    }
                }
            }
            else 
            {
                lu32_ErrState = E2_1_8V_ERR;
            }
        }
    }

    return lu32_ErrState;
}

/************************************************************************************
 * @fn      __SDCard_InitCard
 *
 * @brief   Initializes the sd card.
 */
static uint32_t __SDCard_InitCard(SD_HandleTypeDef *hsd)
{
    uint32_t lu32_ErrState = INT_NO_ERR;
    uint32_t lu32_Response;
    uint32_t CMD6_Data[16];

    /* CMD2: ALL_SEND_CID */
    lu32_ErrState = SD_CMD_AllSendCID(hsd->SDx, hsd->CID);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD3: SEND_RELATIVE_ADDR */
    lu32_ErrState = SD_CMD_SendRelAddr(hsd->SDx, &hsd->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD9: SEND_CSD */
    lu32_ErrState = SD_CMD_SendCSD(hsd->SDx, hsd->RCA, hsd->CSD);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* CMD7: SEL_DESEL_CARD */
    lu32_ErrState = SD_CMD_SelectCard(hsd->SDx, hsd->RCA);
    if (lu32_ErrState) 
    {
        return lu32_ErrState;
    }

    /* Wait to enter transfer state */
    uint32_t lu32_Timeout = 100;

    while (lu32_Timeout--)
    {
        /* CMD13: SEND_STATUS */
        lu32_ErrState = SD_CMD_SendStatus(hsd->SDx, hsd->RCA, &lu32_Response);
        if (lu32_ErrState)
        {
            return lu32_ErrState;
        }
        if (lu32_Response & CARD_CURRENT_STATE_TRAN)
            break;
    }

    /* Switch card function */
    if (hsd->Init.SpeedMode != SPEED_DS) 
    {
        /* CMD6: SWITCH_FUNC */
        lu32_ErrState = SD_CMD_SwitchFunc(hsd->SDx, 0x80FFFFF0 | hsd->Init.SpeedMode);
        if (lu32_ErrState)
        {
            return lu32_ErrState;
        }

        if (lu32_ErrState == INT_NO_ERR) 
        {
            /* Wait for Buffer Read Ready Int */
            while(!(__SD_GET_INT_STATUS(hsd->SDx) & INT_BUFFER_READ_READY));
            /* Clear Buffer_Read_Ready */
            __SD_CLR_INT_STATUS(hsd->SDx, INT_BUFFER_READ_READY);
            /* Read one block */
            for (int i = 0; i < 16; i++)
            {
                CMD6_Data[i] = __SD_GET_BUFFERDATA(hsd->SDx);
            }

            /* wait for transfer complete or any errors occur */
            while(!(__SD_GET_INT_STATUS(hsd->SDx) & (INT_TRANSFER_COMPLETE | INT_ERR_MASK)));
            /* Any errors occur */
            if (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK)
            {
                return (__SD_GET_INT_STATUS(hsd->SDx) & INT_ERR_MASK);
            }
            /* clear interrupt status */
            __SD_CLR_ALL_INT_STATUS(hsd->SDx);


            /* Check Card */
            /* Card Not support selected Speed */
            if ((CMD6_Data[4] & 0xF) == 0xF)
            {
                lu32_ErrState = E3_SPEED_ERR;
                
                return lu32_ErrState;
            }
            /* Update Speed mode */
            else 
            {
                if (hsd->Init.SpeedMode == SPEED_HS)
                {
                    /* HIGH Speed mode */
                    __SD_SPEED_MODE(hsd->SDx, 1);
                }
                else 
                {
                    /* SDR25/SDR50/SDR104/DDR50 */
                    __SD_SDCLK_DISABLE(hsd->SDx);
                    __SD_UHS_MODE(hsd->SDx, (hsd->Init.SpeedMode & 0xF));
                    __SD_SDCLK_ENABLE(hsd->SDx);
                }
            }
        }
    }

    /* Update bus clock speed */
    __SD_CLOCK_DIV_LOW_8BIT(hsd->SDx, (hsd->Init.ClockDiv / 2));
    /* Fixed Block Size 512 Byte */
    __SD_SET_BLOCK_SIZE(hsd->SDx, 512);

    return lu32_ErrState;
}
