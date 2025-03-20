/*
  ******************************************************************************
  * @file    driver_spi_slave.c
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
#include "driver_spi.h"

static void spi_slave_tx_u8(SPI_HandleTypeDef *hspi)
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

static void spi_slave_tx_u16(SPI_HandleTypeDef *hspi)
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

static void spi_slave_tx_u32(SPI_HandleTypeDef *hspi)
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

static void spi_slave_rx_u8(SPI_HandleTypeDef *hspi)
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

static void spi_slave_rx_u16(SPI_HandleTypeDef *hspi)
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

static void spi_slave_rx_u32(SPI_HandleTypeDef *hspi)
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
 * @fn      spi_slave_IRQHandler
 *
 * @brief   Handle SPI interrupt request.
 *
 * @param   hspi: SPI handle.
 */
void spi_slave_IRQHandler(SPI_HandleTypeDef *hspi)
{
    uint8_t frame_size;
    /* Tx FIFO Threshold EMPTY */
    if (__SPI_TxFIFO_EMPTY_INT_STATUS(hspi->SPIx))
    {
        frame_size = hspi->Init.Frame_Size;
        
        if(frame_size <= SPI_FRAME_SIZE_8BIT)
        {
            spi_slave_tx_u8(hspi);
        }
        else if(frame_size <= SPI_FRAME_SIZE_16BIT)
        {
            spi_slave_tx_u16(hspi);
        }
        else
        {
            spi_slave_tx_u32(hspi);
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
        frame_size = hspi->Init.Frame_Size;
        
        if(frame_size <= SPI_FRAME_SIZE_8BIT)
        {
            spi_slave_rx_u8(hspi);
        }
        else if(frame_size <= SPI_FRAME_SIZE_16BIT)
        {
            spi_slave_rx_u16(hspi);
        }
        else
        {
            spi_slave_rx_u32(hspi);
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
 * @fn      spi_slave_init
 *
 * @brief   Initialize the SPI according to the specified parameters in the struct_SPIInit_t
 *
 * @param   hspi: SPI handle.
 */
void spi_slave_init(SPI_HandleTypeDef *hspi)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx);

    /* Work mode */
    hspi->SPIx->CTRL0.SCPOL = hspi->Init.Work_Mode & 0x2 ? 1 : 0;
    hspi->SPIx->CTRL0.SCPH  = hspi->Init.Work_Mode & 0x1 ? 1 : 0;

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

    __SPI_CS_TOGGLE_DISABLE(hspi->SPIx);

    /* CS Set */
    hspi->SPIx->SER  = 0;
}

/************************************************************************************
 * @fn      spi_slave_transmit
 *
 * @brief   Send an amount of data in blocking mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_slave_transmit(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;
    
    p_data.data = fp_Data;    

    /* config Mult Write Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Write);
    
    /* Get current frame size */
    frame_size = hspi->Init.Frame_Size;
    
    /* Frame Size <= 8 */
    if (frame_size <= SPI_FRAME_SIZE_8BIT) 
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
        while(fu32_Size--)
        {
            while(__SPI_IS_TxFIFO_FULL(hspi->SPIx));
            /* write data to tx FIFO */
            hspi->SPIx->DR = *p_data.p_u32++;             
        }    
    }
    
    while(__SPI_IS_BUSY(hspi->SPIx));
}

/************************************************************************************
 * @fn      spi_slave_transmit_IT
 *
 * @brief   Send an amount of data in interrupt mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be sent
 */
void spi_slave_transmit_IT(SPI_HandleTypeDef *hspi, void*fp_Data, uint32_t fu32_Size)
{
    if (hspi->b_TxBusy)
        return;
    uint8_t frame_size;
    
    /* config Mult Write Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Write);

    hspi->u32_TxSize       = fu32_Size;
    hspi->u32_TxCount      = 0;
    hspi->b_TxBusy         = true;
    hspi->u_TxData.p_data  = fp_Data;
    
    /* Get current frame size */
    frame_size = hspi->Init.Frame_Size;

    if (frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
        {
            hspi->SPIx->DR = hspi->u_TxData.p_u8[hspi->u32_TxCount++];
            
            if (hspi->u32_TxCount >= fu32_Size) 
            {
                hspi->b_TxBusy = false;
                return;
            }            
        }
    }
    else if (frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
        {
            hspi->SPIx->DR = hspi->u_TxData.p_u16[hspi->u32_TxCount++];
            
            if (hspi->u32_TxCount >= fu32_Size) 
            {
                hspi->b_TxBusy = false;
                return;
            }
        }           
    }
    else
    {
        while(!__SPI_IS_TxFIFO_FULL(hspi->SPIx))
        {
            hspi->SPIx->DR = hspi->u_TxData.p_u32[hspi->u32_TxCount++];

            if (hspi->u32_TxCount >= fu32_Size) 
            {
                hspi->b_TxBusy = false;
                return;
            }            
        }             
    }

    /* TxFIFO empty interrupt enable */
    __SPI_TxFIFO_EMPTY_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_slave_transmit_DMA
 *
 * @brief   Send an amount of data in DMA mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 */
void spi_slave_transmit_DMA(SPI_HandleTypeDef *hspi)
{ 
    /* config Mult Write Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Write);

    /* DMA Config */
    __SPI_DMA_TX_ENABLE(hspi->SPIx);
    __SPI_DMA_TX_LEVEL(hspi->SPIx, hspi->Init.TxFIFOEmpty_Threshold);
}

/************************************************************************************
 * @fn      spi_slave_receive
 *
 * @brief   Receive an amount of data in blocking mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be Receive
 */
void spi_slave_receive(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;

    p_data.data = fp_Data;
    
    /* config Mult Read Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Read);
    
    /* Get current frame size */
    frame_size = hspi->Init.Frame_Size;
    
    /* Frame Size <= 8 */
    if (frame_size <= SPI_FRAME_SIZE_8BIT) 
    {
        
        while (fu32_Size) 
        {
            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u8++ = hspi->SPIx->DR;

                fu32_Size--;
            }
        }
    }
    else if (frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while (fu32_Size) 
        {
            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u16++ = hspi->SPIx->DR;
                
                fu32_Size--;
            }
        }
    }
    else
    {
        while (fu32_Size) 
        {
            while(!__SPI_IS_RxFIFO_EMPTY(hspi->SPIx))
            {
                *p_data.p_u32++ = hspi->SPIx->DR;
                
                fu32_Size--;
            }
        }        
    }

    while(__SPI_IS_BUSY(hspi->SPIx));
}

/************************************************************************************
 * @fn      spi_slave_receive_IT
 *
 * @brief   Receive an amount of data in interrupt mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be Receive
 */
void spi_slave_receive_IT(SPI_HandleTypeDef *hspi, void *fp_Data, uint32_t fu32_Size)
{
    if (hspi->b_RxBusy)
        return;

    /* config Mult Read Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Read);

    hspi->u32_RxSize       = fu32_Size;
    hspi->u32_RxCount      = 0;
    hspi->b_RxBusy         = true;
    hspi->u_RxData.p_data  = fp_Data;

    /* RxFIFO full interrupt enable */
    __SPI_RxFIFO_FULL_INT_ENABLE(hspi->SPIx);
}

/************************************************************************************
 * @fn      spi_slave_receive_DMA
 *
 * @brief   Receive an amount of data in interrupt mode.(Standard、Dual and Quad mode)
 *
 * @param   hspi: SPI handle.
 *          fp_Data: pointer to data buffer
 *          fu32_Size: amount of data to be Receive
 */
void spi_slave_receive_DMA(SPI_HandleTypeDef *hspi)
{   
    /* config Mult Read Transfer parameters */    
    spi_slave_MultWireConfig(hspi,Wire_Read);

    /* DMA Config */
    __SPI_DMA_RX_ENABLE(hspi->SPIx);
    __SPI_DMA_RX_LEVEL(hspi->SPIx, hspi->Init.RxFIFOFull_Threshold);
}

/************************************************************************************
 * @fn      spi_slave_MultWireConfig
 *
 * @brief   config Mult Wire Transfer parameters.(DualbQuad mode)
 *
 * @param   hspi: SPI handle.
 */
void spi_slave_MultWireConfig(SPI_HandleTypeDef *hspi, enum_Wire_Type_t fe_type)
{
    /* Disable SPI, reset FIFO */
    __SPI_DISABLE(hspi->SPIx); 
    
    /* slave mode and Frame Size*/
    if (hspi->MultWireParam.Wire_X2X4X8 == Wire_X2)
    {
        __SPI_SLAVE_SET_MODE_X2(hspi->SPIx);  
        
        hspi->SPIx->CTRL0.DFS_32 = (hspi->Init.Frame_Size + 1) / 2 - 1;
    }
    else if (hspi->MultWireParam.Wire_X2X4X8 == Wire_X4)
    {
        __SPI_SLAVE_SET_MODE_X4(hspi->SPIx);  
        
        hspi->SPIx->CTRL0.DFS_32 = (hspi->Init.Frame_Size + 1) / 4 - 1;
    }
    else
    {
        __SPI_SLAVE_SET_MODE_X1(hspi->SPIx); 
        hspi->SPIx->CTRL0.DFS_32 = hspi->Init.Frame_Size;        
    } 

    if (fe_type == Wire_Write) 
    {
        /* slave output enable */
        hspi->SPIx->CTRL0.SLV_OE = 0;
        
        /* Select Only Tx mode */
        __SPI_TMODE_Tx_ONLY(hspi->SPIx);
    } 
    else
    {
        /* slave output disable */
        hspi->SPIx->CTRL0.SLV_OE = 1;
    
        /* Select Only Rx mode */
        __SPI_TMODE_Rx_ONLY(hspi->SPIx);        
    }

    /* Enable SPI */
    __SPI_ENABLE(hspi->SPIx);    
}
