/*
  ******************************************************************************
  * @file    driver_i2s.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   I2S module driver.
  *          This file provides firmware functions to manage the 
  *          Inter - IC Sound (I2S) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

 /************************************************************************************
 * @fn      i2s_IRQHandler
 *
 * @brief   Handle I2S interrupt request.
 *
 * @param   huart: I2S handle.
 */
__WEAK void i2s_IRQHandler(I2S_HandleTypeDef *hi2s)
{
    /* Format 20Bit/24Bit/32bit */
    if (hi2s->Init.DataFormat > I2S_DATA_FORMAT_16BIT) 
    {
        /* TxFIFO half full interrupt enable */
        if (__I2S_INT_IS_ENANLE(hi2s->I2Sx, I2S_TX_FIFOS_ALMOST_EMPTY))
        {
            /* FIFO half full */
            if ((__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_TX_FIFOS_ALMOST_EMPTY)) 
            {
                if (hi2s->TxIntCallback)
                {
                    hi2s->TxIntCallback(hi2s);
                }
                else
                {
                    for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH; i++)
                    {
                        hi2s->I2Sx->DATA_L = 0;
                        hi2s->I2Sx->DATA_R = 0;
                    }
                }
            }
        }
        /* Rx FIFO full interrupt enable */
        if(__I2S_INT_IS_ENANLE(hi2s->I2Sx, I2S_RX_FIFOS_HALF_FULL))
        {
            /* FIFO half full */
            if ((__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_RX_FIFOS_HALF_FULL))
            {
                if (hi2s->RxIntCallback)
                {
                    hi2s->RxIntCallback(hi2s);
                }
                else
                {
                    for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH; i++)
                    {
                        volatile uint32_t l_data = hi2s->I2Sx->DATA_L;
                        volatile uint32_t r_data = hi2s->I2Sx->DATA_R;
                    }
                }
            }    
        }
    }
    /* Format 8Bit/16Bit */
    else
    {
        /* TxFIFO half full interrupt enable */
        if (__I2S_INT_IS_ENANLE(hi2s->I2Sx, I2S_TX_L_FIFO_ALMOST_EMPTY))
        {   
            /* FIFO half full */
            if ((__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_TX_L_FIFO_ALMOST_EMPTY)) 
            {
                if (hi2s->TxIntCallback)
                {
                    hi2s->TxIntCallback(hi2s);
                }
                else
                {
                    for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH; i++)
                    {
                        hi2s->I2Sx->DATA_L = 0;
                    }
                }                  
            }
        }
        /* Rx FIFO full interrupt enable */
        if(__I2S_INT_IS_ENANLE(hi2s->I2Sx, I2S_RX_L_FIFO_HALF_FULL))  
        {        
            /* FIFO half full */
            if ((__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_RX_L_FIFO_HALF_FULL)) 
            {
                if (hi2s->RxIntCallback)
                {
                    hi2s->RxIntCallback(hi2s);
                }
                else
                {
                    for (uint32_t i=0; i<I2S_FIFO_HALF_DEPTH; i++)
                    {
                        volatile uint32_t l_data = hi2s->I2Sx->DATA_L;
                    }
                }
            } 
        }
    }
}

/************************************************************************************
 * @fn      i2s_init
 *
 * @brief   Initialize the I2S peripheral according to the specified 
 *          parameters in the struct_I2SInit_t
 *
 * @param   hi2s: I2S handle.
 */
void i2s_init(I2S_HandleTypeDef *hi2s)
{
    hi2s->b_TxBusy = false;
    hi2s->b_RxBusy = false;
    
    /* disable all interrupt */
    __I2S_INT_DISABLE(hi2s->I2Sx, I2S_ALL_FIFO_STATUS);

    /* Enable Rx/Tx FIFO */
    __I2S_RxFIFO_EN(hi2s->I2Sx);
    __I2S_TxFIFO_EN(hi2s->I2Sx);

    /* Clear Rx/Tx FIFO */
    __I2S_RxFIFO_CLR(hi2s->I2Sx);
    __I2S_TxFIFO_CLR(hi2s->I2Sx);
    
    /* left and right channels operate simultaneously */
    __I2S_WD_SWAP_HIGH16SIZE_RIGHT(hi2s->I2Sx);
    
    /* config mode */
    hi2s->I2Sx->CTRL0.MSTSLV = hi2s->Init.Mode;

    /* Calculate audio frequency */
    hi2s->I2Sx->FrmDiv.BCLKDIV = hi2s->Init.BCLKDIV - 1;

    /* Calculate channel length */
    hi2s->I2Sx->FrmDiv.FRMDIV = hi2s->Init.ChannelLength - 1;

    /* Data length */
    hi2s->I2Sx->CTRL1.I2S_DATA_LENGTH = hi2s->Init.DataFormat;
    
    switch (hi2s->Init.Standard)
    {
        /* philips */ 
        case I2S_STANDARD_PHILIPS:
        {
            hi2s->I2Sx->CTRL1.I2S_NORMAL = 1;
            
            __I2S_ENABLE(hi2s->I2Sx);
        }break;

        /* MSB */
        case I2S_STANDARD_MSB:
        {  
            hi2s->I2Sx->CTRL1.I2S_NORMAL = 0;
            hi2s->I2Sx->CTRL1.I2S_ADJUST = 0;
            
            __I2S_FRAME_INV_ENABLE(hi2s->I2Sx);
            __I2S_ENABLE(hi2s->I2Sx);
        }break;

        /* LSB */
        case I2S_STANDARD_LSB:
        {
            uint32_t data_length[5] = {8,16,20,24,32};
          
            hi2s->I2Sx->CTRL1.I2S_NORMAL = 0;
            hi2s->I2Sx->CTRL1.I2S_ADJUST = 1;
            hi2s->I2Sx->CTRL1.I2SFBOFF   = hi2s->Init.ChannelLength - data_length[hi2s->Init.DataFormat];

            __I2S_FRAME_INV_ENABLE(hi2s->I2Sx);
            __I2S_ENABLE(hi2s->I2Sx);
        }break;

        /* PCM */
        case I2S_STANDARD_PCM:
        {
            /* Mono or Stereo select*/
            hi2s->I2Sx->PCM_RHYCTRL.PCM_SAMPTYPE    = 0;
            hi2s->I2Sx->PCM_GENCTRL.PCM_MONO_STEREO = 0;
            /* Frame Synchronization signal */
            hi2s->I2Sx->PCM_RHYCTRL.PCM_FSYNCSHP = 0;
            /* Sample size and slots */
            __I2S_PCM_SLOTS_SET(hi2s->I2Sx, 1);
            __I2S_PCM_SAMPLESIZE_16BIT(hi2s->I2Sx);
            /* ENABLE PCM */
            hi2s->I2Sx->PCM_RHYCTRL.PCM_DOUTCFG = 1;
            __I2S_PCM_ENABLE(hi2s->I2Sx);
        }break;
        
        default: break; 
    }
}

/************************************************************************************
 * @fn      i2s_deinit
 *
 * @brief   deinit an ongoing i2s.
 *
 * @param   hi2s: I2S handle.
 */
void i2s_deinit(I2S_HandleTypeDef *hi2s)
{
    __I2S_PCM_DISABLE(hi2s->I2Sx);
    __I2S_DISABLE(hi2s->I2Sx);
    
    hi2s->b_TxBusy = false;
    hi2s->b_RxBusy = false;
}

/************************************************************************************
 * @fn      i2s_transmit_IT
 *
 * @brief   Transmit an I2S data in interrupt mode, user should send data in TxCallback with 
 *          i2s_send_data function.
 *
 * @param   huart: I2S handle.
 */
bool i2s_transmit_IT(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->b_TxBusy)  return false;

    hi2s->b_TxBusy    = true;
    
    __I2S_L_TxFIFO_EMPTY_LEVEL(hi2s->I2Sx, I2S_FIFO_HALF_DEPTH);
    __I2S_R_TxFIFO_EMPTY_LEVEL(hi2s->I2Sx, I2S_FIFO_HALF_DEPTH);

    /* Format 20Bit/24Bit/32bit */
    if (hi2s->Init.DataFormat > I2S_DATA_FORMAT_16BIT)    
    {         
        __I2S_LR_WD_DISABLE(hi2s->I2Sx);
        
        __I2S_INT_ENABLE(hi2s->I2Sx, I2S_TX_FIFOS_ALMOST_EMPTY);
    }
    /* Format 8Bit/16Bit */
    else
    {
        __I2S_LR_WD_ENABLE(hi2s->I2Sx);
        
        __I2S_INT_ENABLE(hi2s->I2Sx, I2S_TX_L_FIFO_ALMOST_EMPTY);    
    }
    
    return true;
}

/************************************************************************************
 * @fn      i2s_receive_IT
 *
 * @brief   Receive an I2S data in interrupt mode. User read received data with function
 *          i2s_read_data in RxCallback.
 *
 * @param   huart: I2S handle.
 */
bool i2s_receive_IT(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->b_RxBusy)  return false;

    hi2s->b_RxBusy    = true;
    
    __I2S_L_RxFIFO_FULL_LEVEL(hi2s->I2Sx, 16);
    __I2S_R_RxFIFO_FULL_LEVEL(hi2s->I2Sx, 16);
    
    /* Format 20Bit/24Bit/32bit */
    if (hi2s->Init.DataFormat > I2S_DATA_FORMAT_16BIT)
    {       
        __I2S_LR_WD_DISABLE(hi2s->I2Sx);
        __I2S_INT_ENABLE(hi2s->I2Sx, I2S_RX_FIFOS_HALF_FULL);   
    }
    /* Format 8Bit/16Bit */    
    else
    {
        __I2S_LR_WD_ENABLE(hi2s->I2Sx);
        __I2S_INT_ENABLE(hi2s->I2Sx, I2S_RX_L_FIFO_HALF_FULL);   
    }
    
    return true;
}

/************************************************************************************
 * @fn      i2s_read_data
 *
 * @brief   Read data from RX FIFO. One sample contains left and right channel data. When
 *          DataFormat is not more than 16-bits, one sample takes 32-bits, left channel data
 *          is stored in high 16-bits and right channel data is stored in low 16-bits. When
 *          DataFormat is more than 16-bits, one sample takes 2 words.
 *
 * @param   huart: I2S handle.
 *          buffer: buffer used to store received data.
 *          samples: How many samples expected to receive.
 *
 * @return  How many samples read from RX FIFO
 */
uint32_t i2s_read_data(I2S_HandleTypeDef *hi2s, uint32_t *buffer, uint32_t samples)
{
    uint32_t count = 0;

    if (hi2s->Init.DataFormat > I2S_DATA_FORMAT_16BIT)
    {
        while ((!(__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_RX_FIFOS_EMPTY))
            && (count < samples))
        {
            buffer[count] = hi2s->I2Sx->DATA_L;
            buffer[count] = hi2s->I2Sx->DATA_R;
            count++;
        }
    }
    else
    {
        __I2S_LR_WD_ENABLE(hi2s->I2Sx);

        while ((!(__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_RX_L_FIFO_EMPTY))
            && (count < samples))
        {
            buffer[count] = hi2s->I2Sx->DATA_L;
            count++;
        }
    }
    
    return count;
}

/************************************************************************************
 * @fn      i2s_send_data
 *
 * @brief   write data to TX FIFO. One sample contains left and right channel data. When
 *          DataFormat is not more than 16-bits, one sample takes 32-bits, left channel data
 *          is stored in high 16-bits and right channel data is stored in low 16-bits. When
 *          DataFormat is more than 16-bits, one sample takes 2 words.
 *
 * @param   huart: I2S handle.
 *          buffer: buffer used to store sending data.
 *          samples: How many samples expected to send.
 *
 * @return  How many samples sent to TX FIFO
 */
uint32_t i2s_send_data(I2S_HandleTypeDef *hi2s, uint32_t *buffer, uint32_t samples)
{
    uint32_t count = 0;

    if (hi2s->Init.DataFormat > I2S_DATA_FORMAT_16BIT)
    {
        while ((!(__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_TX_FIFOS_FULL))
            && (count < samples))
        {
            hi2s->I2Sx->DATA_L = buffer[count];
            hi2s->I2Sx->DATA_R = buffer[count];
            count++;
        }
    }
    else
    {
        __I2S_LR_WD_ENABLE(hi2s->I2Sx);

        while ((!(__I2S_GET_INT_STATUS(hi2s->I2Sx) & I2S_TX_L_FIFO_FULL))
            && (count < samples))
        {
            hi2s->I2Sx->DATA_L = buffer[count];
            count++;
        }
    }
    
    return count;
}

/************************************************************************************
 * @fn      i2s_transmit_DMA
 *
 * @brief   Transmit an amount of data in DMA mode.
 */
void i2s_transmit_DMA(I2S_HandleTypeDef *hi2s)
{
    hi2s->I2Sx->DMA_CFG.DMACR_L_TX = 1;
    hi2s->I2Sx->DMA_CFG.DMACR_R_TX = 1;    
    /* Tx empty dma level */
    __I2S_TX_DMA_EMPTY_LEVEL(hi2s->I2Sx, 16); 
}

/************************************************************************************
 * @fn      i2s_receive_DMA
 *
 * @brief   Receive an amount of data in DMA mode.
 */
void i2s_receive_DMA(I2S_HandleTypeDef *hi2s)
{
    hi2s->I2Sx->DMA_CFG.DMACR_L_RX = 1;
    hi2s->I2Sx->DMA_CFG.DMACR_R_RX = 1;    
    /* Rx empty dma level */
    __I2S_RX_DMA_EMPTY_LEVEL(hi2s->I2Sx, 16);   
}
