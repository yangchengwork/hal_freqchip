/*
  ******************************************************************************
  * @file    driver_spi_master.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   SPI module driver.
  *          This file provides firmware functions to manage the 
  *          Serial Peripheral Interface (SPI) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"


static void spi_master_tx_u8(SPI_HandleTypeDef *hspi)
{
    while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
    {
        hspi->SPIx->DR = hspi->u_TxData.p_u8[hspi->u32_TxCount++];
        
        if (hspi->u32_TxCount >= hspi->u32_TxSize)
        {
            break;
        }
    }
}    

static void spi_master_tx_u16(SPI_HandleTypeDef *hspi)
{
    while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
    {
        hspi->SPIx->DR = hspi->u_TxData.p_u16[hspi->u32_TxCount++];
        
        if (hspi->u32_TxCount >= hspi->u32_TxSize)
        {
            break;
        }
    }
}

static void spi_master_tx_u32(SPI_HandleTypeDef *hspi)
{
    while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
    {
        hspi->SPIx->DR = hspi->u_TxData.p_u32[hspi->u32_TxCount++];
        
        if (hspi->u32_TxCount >= hspi->u32_TxSize)
        {
            break;
        }
    }
}

static void spi_master_rx_u8(SPI_HandleTypeDef *hspi)
{
    while (!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx)) 
    {
        hspi->u_RxData.p_u8[hspi->u32_RxCount++] = hspi->SPIx->DR;
        
        if (hspi->u32_RxCount >= hspi->u32_RxSize)
        {
            break;
        }
    }
}    

static void spi_master_rx_u16(SPI_HandleTypeDef *hspi)
{
    while (!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx)) 
    {
        hspi->u_RxData.p_u16[hspi->u32_RxCount++] = hspi->SPIx->DR;
        
        if (hspi->u32_RxCount >= hspi->u32_RxSize)
        {
            break;
        }
    }
}

static void spi_master_rx_u32(SPI_HandleTypeDef *hspi)
{
    while (!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx)) 
    {
        hspi->u_RxData.p_u32[hspi->u32_RxCount++] = hspi->SPIx->DR;
        
        if (hspi->u32_RxCount >= hspi->u32_RxSize)
        {
            break;
        }
    }
}

/************************************************************************************
 * @fn      spi_master_IRQHandler
 *
 * @brief   Handle SPI interrupt request.
 *
 * @param   hspi: SPI handle.
 */
void spi_master_IRQHandler(SPI_HandleTypeDef *hspi)
{
    uint8_t frame_size;

    /* Tx FIFO Threshold EMPTY */
    if (__SPI_TxFIFO_EMPTY_INT_STATUS(hspi->SPIx))
    {
        frame_size = __SPI_DATA_FRAME_SIZE_GET(hspi->SPIx);
        
        if(frame_size <= SPI_FRAME_SIZE_8BIT)
        {
            spi_master_tx_u8(hspi);
        }
        else if(frame_size <= SPI_FRAME_SIZE_16BIT)
        {
            spi_master_tx_u16(hspi);
        }
        else
        {
            spi_master_tx_u32(hspi);
        }

        if (hspi->u32_TxCount >= hspi->u32_TxSize) 
        {
            __SPI_TxFIFO_EMPTY_INT_DISABLE(hspi->SPIx);

            hspi->b_TxBusy = false;

            if (hspi->TxCpltCallback != NULL)
            {
                hspi->TxCpltCallback(hspi);
            }
        }
    }
    /* Rx FIFO Threshold FULL */
    else if (__SPI_RxFIFO_FULL_INT_STATUS(hspi->SPIx))
    {
        frame_size = hspi->SPIx->CTRL0.DFS_32;
        
        if(frame_size <= SPI_FRAME_SIZE_8BIT)
        {
            spi_master_rx_u8(hspi);
        }
        else if(frame_size <= SPI_FRAME_SIZE_16BIT)
        {
            spi_master_rx_u16(hspi);
        }
        else
        {
            spi_master_rx_u32(hspi);
        }
        
        if (hspi->u32_RxCount >= hspi->u32_RxSize) 
        {
            __SPI_RxFIFO_FULL_INT_DISABLE(hspi->SPIx);

            hspi->b_RxBusy = false;

            if (hspi->RxCpltCallback != NULL)
            {
                hspi->RxCpltCallback(hspi);
            }
        }
    }
}

/************************************************************************************
 * @fn      spi_master_init
 *
 * @brief   Initialize the SPI according to the specified parameters in the struct_SPIInit_t
 *
 * @param   hspi: SPI handle.
 */
void spi_master_init(SPI_HandleTypeDef *hspi)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Work mode */
    hspi->SPIx->CTRL0.SCPOL = hspi->Init.Work_Mode & 0x2 ? 1 : 0;
    hspi->SPIx->CTRL0.SCPH  = hspi->Init.Work_Mode & 0x1 ? 1 : 0;
    /* Frame Size */
    hspi->SPIx->CTRL0.DFS_32 = hspi->Init.Frame_Size;
    /* BaudRate Prescaler */
    hspi->SPIx->BAUDR = hspi->Init.BaudRate_Prescaler;

    /* FIFO Threshold */
    hspi->SPIx->TXFTLR = hspi->Init.TxFIFOEmpty_Threshold;
    hspi->SPIx->RXFTLR = hspi->Init.RxFIFOFull_Threshold;

    /* Disable all interrupt */
    hspi->SPIx->IMR.MSTIM = 0;
    hspi->SPIx->IMR.TXEIM = 0;
    hspi->SPIx->IMR.TXOIM = 0;
    hspi->SPIx->IMR.RXUIM = 0;
    hspi->SPIx->IMR.RXOIM = 0;
    hspi->SPIx->IMR.RXFIM = 0;
    
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_SET_FLOW_SIZE(hspi->SPIx, 62);
    }
    
    __SPI_CS_TOGGLE_DISABLE(hspi->SPIx);

    /* CS Set */
    hspi->SPIx->SER  = 1;
}

/************************************************************************************
 * @fn      spi_master_transmit_X1
 *
 * @brief   Send an amount of data in blocking mode.(Standard mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_master_transmit_X1(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Disable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_DISABLE(hspi->SPIx);
    }

    /* Select RxTx mode */
    __SPI_TMODE_RxTx(hspi->SPIx);
    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    /* Get current frame size */
    frame_size = __SPI_DATA_FRAME_SIZE_GET(hspi->SPIx);
    
    p_data.data = fp_Data;
    
    if(frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u8++;
        }
    }
    else if(frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u16++;
        }
    }
    else
    {
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u32++;
        }
    }
    
    while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
    while(__SPI_IS_BUSY(hspi->SPIx));
}

/************************************************************************************
 * @fn      spi_master_transmit_X1_IT
 *
 * @brief   Send an amount of data in interrupt mode.(Standard mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_master_transmit_X1_IT(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    if (hspi->b_TxBusy)
        return;
        
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Disable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_DISABLE(hspi->SPIx);
    }

    /* Select RxTx mode */
    __SPI_TMODE_RxTx(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    hspi->u32_TxSize        = fu32_Size;
    hspi->u32_TxCount       = 0;
    hspi->u_TxData.p_data   = fp_Data;
    hspi->b_TxBusy          = true;

    /* TxFIFO empty interrupt enable */
    __SPI_TxFIFO_EMPTY_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_master_transmit_X1_DMA
 *
 * @brief   Send an amount of data in DMA mode.(Standard mode)
 *
 * @param   hspi: SPI handle.
 */
void spi_master_transmit_X1_DMA(SPI_HandleTypeDef *hspi)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Disable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_DISABLE(hspi->SPIx);
    }

    /* Clear DMA signals */
    __SPI_DMA_TX_DISABLE(hspi->SPIx);

    /* Select RxTx mode */
    __SPI_TMODE_RxTx(hspi->SPIx);
    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    /* DMA Config */
    __SPI_DMA_TX_ENABLE(hspi->SPIx);
    __SPI_DMA_TX_LEVEL(hspi->SPIx, hspi->Init.TxFIFOEmpty_Threshold);
}

/************************************************************************************
 * @fn      spi_master_receive_X1
 *
 * @brief   Receive an amount of data in blocking mode.(Standard mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be Receive
 */
void spi_master_receive_X1(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;
    uint32_t RxCount = fu32_Size;
    uint32_t TxCount = fu32_Size;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Enable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_ENABLE(hspi->SPIx);
    } 

    /* Select RxTx mode */
    __SPI_TMODE_RxTx(hspi->SPIx);
    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);
    
    /* Get current frame size */
    frame_size = __SPI_DATA_FRAME_SIZE_GET(hspi->SPIx);
    
    p_data.data = fp_Data;
    
    if(frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        while (RxCount) 
        {
            /* write data to tx FIFO */
            if (!__SPI_IS_TxFIFO_FULL(hspi->SPIx) && TxCount) 
            {
                hspi->SPIx->DR = 0x00;
                TxCount--;
            }

            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u8++ = hspi->SPIx->DR;

                RxCount--;
            }
        }
    }
    else if(frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while (RxCount) 
        {
            /* write data to tx FIFO */
            if (!__SPI_IS_TxFIFO_FULL(hspi->SPIx) && TxCount) 
            {
                hspi->SPIx->DR = 0x00;
                TxCount--;
            }

            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u16++ = hspi->SPIx->DR;

                RxCount--;
            }
        }
    }
    else
    {
        while (RxCount) 
        {
            /* write data to tx FIFO */
            if (!__SPI_IS_TxFIFO_FULL(hspi->SPIx) && TxCount) 
            {
                hspi->SPIx->DR = 0x00;
                TxCount--;
            }

            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u32++ = hspi->SPIx->DR;

                RxCount--;
            }
        }
    }

    while(__SPI_IS_BUSY(hspi->SPIx));
}

/************************************************************************************
 * @fn      spi_master_readflash_X1
 *
 * @brief   ReadFlash mode. First send the command address, And then receive the data,
 *          only used for SPI_FRAME_SIZE_8BIT.
 *
 * @param   hspi: SPI handle.
 *          fp_CMD_ADDR: Command and Address.
 *          fu32_CMDLegnth: Command and Address Length.
 *          fp_Data    : pointer to data buffer.
 *          fu16_Size  : amount of data to be Receive.
 */
void spi_master_readflash_X1(SPI_HandleTypeDef *hspi, uint8_t *fp_CMD_ADDR, uint32_t fu32_CMDLegnth, uint8_t *fp_Data, uint16_t fu16_Size)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Enable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_ENABLE(hspi->SPIx);   
    }
    
    /* Select flash read mode */
    __SPI_TMODE_FLASH_READ(hspi->SPIx);

    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* Config receive data size */ 
    hspi->SPIx->CTRL1.NDF = fu16_Size - 1;

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_DISABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_DISABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_DISABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_DISABLE();break;
        default:break;
    }
      
    while (fu32_CMDLegnth--) 
    {
        /* write data to tx FIFO */
        hspi->SPIx->DR = *fp_CMD_ADDR++;
    }

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_ENABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_ENABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_ENABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_ENABLE();break;
        default:break;
    }
    
    while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
    while (__SPI_IS_BUSY(hspi->SPIx)) 
    {
        if (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
        {
            *fp_Data++ = hspi->SPIx->DR;
        }
    }
    
    while (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
    {
        *fp_Data++ = hspi->SPIx->DR;
    }
}

/************************************************************************************
 * @fn      spi_master_readflash_X1_IT
 *
 * @brief   ReadFlash mode. First send the command address, And then receive the data,
 *          only used for SPI_FRAME_SIZE_8BIT.
 *
 * @param   hspi: SPI handle.
 *          fp_CMD_ADDR: Command and Address.
 *          fu32_CMDLegnth: Command and Address Length.
 *          fp_RxData  : pointer to data buffer.
 *          fu16_Size  : amount of data to be Receive.
 */
void spi_master_readflash_X1_IT(SPI_HandleTypeDef *hspi, uint8_t *fp_CMD_ADDR, uint32_t fu32_CMDLegnth, uint8_t *fp_Data, uint16_t fu16_Size)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Enable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_ENABLE(hspi->SPIx);   
    }

    /* Select flash read mode */
    __SPI_TMODE_FLASH_READ(hspi->SPIx);
    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* Config receive data size */ 
    hspi->SPIx->CTRL1.NDF = fu16_Size - 1;

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    hspi->u32_RxSize        = fu16_Size;
    hspi->u32_RxCount       = 0;
    hspi->b_RxBusy          = true;
    hspi->u_RxData.p_data   = (uint8_t *)fp_Data;

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_DISABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_DISABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_DISABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_DISABLE();break;
        default:break;
    }

    while (fu32_CMDLegnth--) 
    {
        /* write data to tx FIFO */
        hspi->SPIx->DR = *fp_CMD_ADDR++;
    }

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_ENABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_ENABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_ENABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_ENABLE();break;
        default:break;
    }

    /* RxFIFO full interrupt enable */
    __SPI_RxFIFO_FULL_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_master_readflash_X1_DMA
 *
 * @brief   ReadFlash mode. First send the command address, And then receive the data,
 *          only used for SPI_FRAME_SIZE_8BIT.
 *
 * @param   hspi: SPI handle.
 *          fp_CMD_ADDR: Command and Address.
 *          fu32_CMDLegnth: Command and Address Length.
 *          fu16_Size  : amount of data to be Receive.
 */
void spi_master_readflash_X1_DMA(SPI_HandleTypeDef *hspi, uint8_t *fp_CMD_ADDR, uint32_t fu32_CMDLegnth, uint16_t fu16_Size)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Enable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
    if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
    {
        __SPI_FLOW_ENABLE(hspi->SPIx);   
    }

    /* Clear DMA signals */
    __SPI_DMA_RX_DISABLE(hspi->SPIx);

    /* Select flash read mode */
    __SPI_TMODE_FLASH_READ(hspi->SPIx);
    /* Select Standard mode */
    __SPI_SET_MODE_X1(hspi->SPIx);

    /* DMA Config */
    __SPI_DMA_RX_LEVEL(hspi->SPIx, hspi->Init.RxFIFOFull_Threshold);
    __SPI_DMA_RX_ENABLE(hspi->SPIx);
    
    /* Config receive data size */ 
    hspi->SPIx->CTRL1.NDF = fu16_Size - 1;

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_DISABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_DISABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_DISABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_DISABLE();break;
        default:break;
    }

    while (fu32_CMDLegnth--) 
    {
        /* write data to tx FIFO */
        hspi->SPIx->DR = *fp_CMD_ADDR++;
    }

    switch ((uint32_t)(hspi->SPIx))
    {
        case SPIM0_BASE:__SYSTEM_SPI_MASTER0_CLK_ENABLE();break;
        case SPIM1_BASE:__SYSTEM_SPI_MASTER1_CLK_ENABLE();break;
        case SPIMX8_0_BASE:__SYSTEM_SPI_MASTER0_X8_CLK_ENABLE();break;
        case SPIMX8_1_BASE:__SYSTEM_SPI_MASTER1_X8_CLK_ENABLE();break;
        default:break;
    }
}

/************************************************************************************
 * @fn      spi_master_transmit_X2X4X8
 *
 * @brief   Send an amount of data in blocking mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_master_transmit_X2X4X8(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Write);

    /* Select Only Tx mode */
    __SPI_TMODE_Tx_ONLY(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);
    
    /* Get current frame size */
    frame_size = __SPI_DATA_FRAME_SIZE_GET(hspi->SPIx);
    
    p_data.data = fp_Data;
    
//#define SPI_FIFO_DEPTH      128
    if(frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        #ifdef SPI_FIFO_DEPTH
        while(fu32_Size >= SPI_FIFO_DEPTH)
        {
            uint32_t u32_l = SPI_FIFO_DEPTH;
            while (u32_l--)
            {
                /* write data to tx FIFO */
                hspi->SPIx->DR = *p_data.p_u8++;
            }
            fu32_Size -= SPI_FIFO_DEPTH;
            while(__SPI_IS_TxFIFO_EMPTY(hspi->SPIx) == 0);
        }
        while (fu32_Size--) 
        {
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u8++;
        }
        #else
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u8++;
        }
        #endif
    }
    else if(frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        #ifdef SPI_FIFO_DEPTH
        while(fu32_Size >= SPI_FIFO_DEPTH)
        {
            uint32_t u32_l = SPI_FIFO_DEPTH;
            while (u32_l--) 
            {
                /* write data to tx FIFO */
                hspi->SPIx->DR = *p_data.p_u16++;
            }
            fu32_Size -= SPI_FIFO_DEPTH;
            while(__SPI_IS_TxFIFO_EMPTY(hspi->SPIx) == 0);
        }
        while (fu32_Size--) 
        {
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u16++;
        }
        #else
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u16++;
        }
        #endif
    }
    else
    {
        #ifdef SPI_FIFO_DEPTH
        while(fu32_Size >= SPI_FIFO_DEPTH)
        {
            uint32_t u32_l = SPI_FIFO_DEPTH;
            while (u32_l--)
            {
                /* write data to tx FIFO */
                hspi->SPIx->DR = *p_data.p_u32++;
            }
            fu32_Size -= SPI_FIFO_DEPTH;
            while(__SPI_IS_TxFIFO_EMPTY(hspi->SPIx) == 0);
        }
        while (fu32_Size--) 
        {
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u32++;
        }
        #else
        while (fu32_Size--) 
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u32++;
        }
        #endif
    }
    while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
    while(__SPI_IS_BUSY(hspi->SPIx));
}

/************************************************************************************
 * @fn      spi_master_transmit_X2X4X8_IT
 *
 * @brief   Send an amount of data in interrupt mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_master_transmit_X2X4X8_IT(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t *lp8_Data;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Write);

    /* Select Only Tx mode */
    __SPI_TMODE_Tx_ONLY(hspi->SPIx);
    
    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    hspi->u32_TxSize        = fu32_Size;
    hspi->u32_TxCount       = 0;
    hspi->b_TxBusy          = true;
    hspi->u_TxData.p_data   = (uint8_t *)fp_Data;

    /* TxFIFO empty interrupt enable */
    __SPI_TxFIFO_EMPTY_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_master_transmit_X2X4X8_IT
 *
 * @brief   Send an amount of data in DMA mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 */
void spi_master_transmit_X2X4X8_DMA(SPI_HandleTypeDef *hspi)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Clear DMA signals */
    __SPI_DMA_TX_DISABLE(hspi->SPIx);

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Write);

    /* Select Only Tx mode */
    __SPI_TMODE_Tx_ONLY(hspi->SPIx);

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);

    /* DMA Config */
    __SPI_DMA_TX_ENABLE(hspi->SPIx);
    __SPI_DMA_TX_LEVEL(hspi->SPIx, hspi->Init.TxFIFOEmpty_Threshold);
}

/************************************************************************************
 * @fn      spi_master_receive_X2X4X8
 *
 * @brief   Receive an amount of data in blocking mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu16_Size: amount of data to be Receive
 */
void spi_master_receive_X2X4X8(SPI_HandleTypeDef *hspi, void *fp_Data, uint16_t fu16_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Config receive data size */ 
    hspi->u32_RxSize = fu16_Size - 1;

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Read);
    
    /* Get current frame size */
    frame_size = __SPI_DATA_FRAME_SIZE_GET(hspi->SPIx);
    
    p_data.data = fp_Data;
    
    if(frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
        while (__SPI_IS_BUSY(hspi->SPIx)) 
        {
            if (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
            {
                *p_data.p_u8++ = hspi->SPIx->DR;
            }
        }

        while (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
        {
            *p_data.p_u8++ = hspi->SPIx->DR;
        }
    }
    else if(frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
        while (__SPI_IS_BUSY(hspi->SPIx)) 
        {
            if (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
            {
                *p_data.p_u16++ = hspi->SPIx->DR;
            }
        }

        while (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
        {
            *p_data.p_u16++ = hspi->SPIx->DR;
        }
    }
    else
    {
        while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
        while (__SPI_IS_BUSY(hspi->SPIx)) 
        {
            if (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
            {
                *p_data.p_u32++ = hspi->SPIx->DR;
            }
        }

        while (__SPI_IS_RxFIFO_NOT_EMPTY(hspi->SPIx)) 
        {
            *p_data.p_u32++ = hspi->SPIx->DR;
        }
    }
}

/************************************************************************************
 * @fn      spi_master_receive_X2X4X8_IT
 *
 * @brief   Receive an amount of data in interrupt mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu16_Size: amount of data to be Receive
 */
void spi_master_receive_X2X4X8_IT(SPI_HandleTypeDef *hspi, void *fp_Data, uint16_t fu16_Size)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Config receive data size */ 
    hspi->u32_RxSize = fu16_Size - 1;

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Read);

    hspi->u32_RxSize        = fu16_Size;
    hspi->u32_RxCount       = 0;
    hspi->u_RxData.p_data   = fp_Data;
    hspi->b_RxBusy          = true;

    /* RxFIFO full interrupt enable */
    __SPI_RxFIFO_FULL_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_master_receive_X2X4X8_DMA
 *
 * @brief   Receive an amount of data in DMA mode.(Dual、Quad and Octal mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu16_Size: amount of data to be Receive
 */
void spi_master_receive_X2X4X8_DMA(SPI_HandleTypeDef *hspi, uint16_t fu16_Size)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Clear DMA signals */
    __SPI_DMA_RX_DISABLE(hspi->SPIx);

    /* DMA Config */
    __SPI_DMA_RX_LEVEL(hspi->SPIx, hspi->Init.RxFIFOFull_Threshold);
    __SPI_DMA_RX_ENABLE(hspi->SPIx);
   
    /* Config receive data size */ 
    hspi->u32_RxSize = fu16_Size - 1;

    /* config Mult Wire Transfer parameters */
    spi_master_MultWireConfig(hspi, Wire_Read);
}

/************************************************************************************
 * @fn      spi_master_MultWireConfig
 *
 * @brief   config Mult Wire Transfer parameters.(DualbQuad mode)
 *
 * @param   hspi: SPI handle.
 */
void spi_master_MultWireConfig(SPI_HandleTypeDef *hspi, enum_Wire_Type_t fe_type)
{
    uint8_t lu8_Buffer[8];
    uint8_t lu8_Count = 0, i = 0;

    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    if (fe_type == Wire_Write) 
    {
        /* Disable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
        if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
        {
            __SPI_FLOW_DISABLE(hspi->SPIx);
        }

        /* Frame Size */
        __SPI_DATA_FRAME_SIZE(hspi->SPIx, SPI_FRAME_SIZE_8BIT);
        /* Select RxTx mode */
        __SPI_TMODE_RxTx(hspi->SPIx);
        /* Select Standard mode */
        __SPI_SET_MODE_X1(hspi->SPIx);

        __SPI_ENABLE(hspi->SPIx);

        if (hspi->MultWireParam.InstructLength == INST_16BIT) 
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Instruct >> 8 & 0xFF;
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Instruct & 0xFF;
        }
        else if (hspi->MultWireParam.InstructLength == INST_8BIT) 
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Instruct & 0xFF;
        }

        if (hspi->MultWireParam.AddressLength >= ADDR_32BIT)
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Address >> 24 & 0xFF;
        }
        if (hspi->MultWireParam.AddressLength >= ADDR_24BIT) 
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Address >> 16 & 0xFF;
        }
        if (hspi->MultWireParam.AddressLength >= ADDR_16BIT) 
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Address >> 8 & 0xFF;
        }
        if (hspi->MultWireParam.AddressLength >= ADDR_8BIT) 
        {
            lu8_Buffer[lu8_Count++] = hspi->MultWireParam.Address & 0xFF;
        }

        while (lu8_Count--) 
        {
            /* write data to tx FIFO */
            hspi->SPIx->DR = lu8_Buffer[i++];
        }

        while(!__SPI_IS_TxFIFO_EMPTY(hspi->SPIx));
        while(__SPI_IS_BUSY(hspi->SPIx));

        __SPI_DISABLE(hspi->SPIx);

        /* Frame Size */
        __SPI_DATA_FRAME_SIZE(hspi->SPIx, hspi->Init.Frame_Size);

        /* Select Dual, Quad and Octal mode*/
        __SPI_SET_MODE_X2X4X8(hspi->SPIx, hspi->MultWireParam.Wire_X2X4X8);
        
        hspi->SPIx->CTRL2.INST_L      = 0;
        hspi->SPIx->CTRL2.WAIT_CYCLES = 0;
        hspi->SPIx->CTRL2.TRANS_TYPE  = INST_ADDR_XX;
        hspi->SPIx->CTRL2.ADDR_L      = 0;
    }
    else 
    {
        /* Enable SPI FLOW only SPIMX8_0 and SPIMX8_1 */
        if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
        {
            __SPI_FLOW_ENABLE(hspi->SPIx);   
        }

        /* Select Dual, Quad and Octal mode*/
        __SPI_SET_MODE_X2X4X8(hspi->SPIx, hspi->MultWireParam.Wire_X2X4X8);

        /* Select Only Rx mode */
        __SPI_TMODE_Rx_ONLY(hspi->SPIx);
    
        /* config Transfer TypebInstruct LengthbAddress Length */
        hspi->SPIx->CTRL2.TRANS_TYPE  = INST_ADDR_X1;
        hspi->SPIx->CTRL2.INST_L      = hspi->MultWireParam.InstructLength;
        hspi->SPIx->CTRL2.ADDR_L      = hspi->MultWireParam.AddressLength;
        hspi->SPIx->CTRL2.WAIT_CYCLES = hspi->MultWireParam.ReceiveWaitCycles;

        hspi->SPIx->CTRL1.NDF = hspi->u32_RxSize;

        /* Enable SPI */
        __SPI_ENABLE(hspi->SPIx);
        /* write Instruct */
        if (hspi->MultWireParam.InstructLength > INST_0BIT) 
        {
            hspi->SPIx->DR = hspi->MultWireParam.Instruct;
        } 
        
        /* write address */        
        if(hspi->SPIx == SPIMX8_0 || hspi->SPIx == SPIMX8_1)
        {
            hspi->SPIx->DR = hspi->MultWireParam.Address;
        }
        else
        {
            if (hspi->MultWireParam.AddressLength > ADDR_24BIT) 
            {
                hspi->SPIx->DR = (hspi->MultWireParam.Address >> 16) & 0xFFFF;
                hspi->SPIx->DR =  hspi->MultWireParam.Address & 0xFFFF;
            }
            else if (hspi->MultWireParam.AddressLength > ADDR_16BIT) 
            {
                hspi->SPIx->DR = (hspi->MultWireParam.Address >> 8) & 0xFFFF;
                hspi->SPIx->DR =  hspi->MultWireParam.Address & 0xFF;
            }
            else if (hspi->MultWireParam.AddressLength > ADDR_0BIT)
            {
                hspi->SPIx->DR = hspi->MultWireParam.Address & 0xFFFF;
            }               
        }
    }
}
