/*
  ******************************************************************************
  * @file    driver_display.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   display module driver.
  *          This file provides firmware functions to manage the 
  *          display controller 8080/6800 RGB SPI peripheral.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/*********************************************************************
 * @fn      display_init
 *
 * @brief   Initialize the display interface according to the specified parameters
 *          in the DISPLAY_HandTypeDef 
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 */
void display_init(DISPLAY_HandTypeDef *hdisplay)
{
    /* SOF RST */ 
    hdisplay->DISPLAYx->INTF_CFG.PARA_RST = 1;
    /* FIFO_RST */
    __DISPLAY_TX_FIFO_RESET(hdisplay->DISPLAYx);

    /* DMA Config */
    hdisplay->DISPLAYx->DMA.DMA_TX_LEVEL = 16;
    hdisplay->DISPLAYx->DMA.DMA_ENABLE   = 1;
    
    switch(hdisplay->Interface_Select)
    {
        case DISPLAY_INTERFACE_PARALLEL:
        {
            /* para clk en */ 
            hdisplay->DISPLAYx->INTF_CFG.PARA_CLKEN = 1;
            /* select parallel interface*/
            hdisplay->DISPLAYx->INTF_CFG.INTF_MODE = 0;
            /* select 8080 or 6800 */
            hdisplay->DISPLAYx->INTF_CFG.MODE = hdisplay->Parallel_Init.ParallelMode;
            /* select 8bit or 16bit */
            hdisplay->DISPLAYx->INTF_CFG.PARA_WIDTH = hdisplay->Parallel_Init.DataBusSelect;
            /* Write Clock DIV */
            hdisplay->DISPLAYx->CRM.WRITE_CLK_DIV = hdisplay->Parallel_Init.WriteClock;
            /* Read Clock DIV */
            hdisplay->DISPLAYx->CRM.READ_CLK_DIV = hdisplay->Parallel_Init.ReadClock;
        }break;
        
        case DISPLAY_INTERFACE_RGB:
        {
            /* RGB clk en */
            hdisplay->DISPLAYx->INTF_CFG.RGB_CLKEN = 1;
            /* select RGB interface*/
            hdisplay->DISPLAYx->INTF_CFG.INTF_MODE = 2;
            
            /* select RGB DCLK LOW level push data*/
            __DISPLAY_RGB_SET_PIXEL_CLK_POLARITY_LOW(hdisplay->DISPLAYx);
            /* set RGB Pixel Clk polarity*/
            __DISPLAY_RGB_SET_PIXEL_CLK_DIV(hdisplay->DISPLAYx,hdisplay->RGB_Init.PixelClock_Prescaler);
            
            /* select RGB Out Format*/
            hdisplay->DISPLAYx->RGB_CFG.RGB_FORAMAT_OUT = hdisplay->RGB_Init.RGB_OUT_FORMAT_SELECT; 
            /* select RGB In Format*/
            hdisplay->DISPLAYx->RGB_CFG.RGB_FORMAT_IN = hdisplay->RGB_Init.RGB_IN_FORMAT_SELECT;  

            /* set VSYNC and HSYNC synchronous pulse width*/
            hdisplay->DISPLAYx->SYNC_PULSE.VSPW = hdisplay->RGB_Init.VSPW - 1;
            hdisplay->DISPLAYx->SYNC_PULSE.HSPW = hdisplay->RGB_Init.HSPW - 1;
            /* set VSYNC and HSYNC resolution */
            hdisplay->DISPLAYx->DISPLAY_PERIOD.HRES = hdisplay->RGB_Init.HRES - 1;  
            hdisplay->DISPLAYx->DISPLAY_PERIOD.VRES = hdisplay->RGB_Init.VRES - 1;   
            /* set VSYNC and HSYNC synchronize the following edge period */
            hdisplay->DISPLAYx->PORCH_PERIOD.HBP = hdisplay->RGB_Init.HBP - 1;  
            hdisplay->DISPLAYx->PORCH_PERIOD.VBP = hdisplay->RGB_Init.VBP - 1;
            /* set VSYNC and HSYNC synchronize the leading edge cycle */
            hdisplay->DISPLAYx->PORCH_PERIOD.HFP = hdisplay->RGB_Init.HFP - 1;
            hdisplay->DISPLAYx->PORCH_PERIOD.VFP = hdisplay->RGB_Init.VFP - 1; 
        }break;
        
        case DISPLAY_INTERFACE_SPI:
        {
            /* SPI clk en */
            hdisplay->DISPLAYx->INTF_CFG.SPI_CLKEN = 1; 
            /* select spi interface*/
            hdisplay->DISPLAYx->INTF_CFG.INTF_MODE = 1; 
            
            /* Disable SPI, reset FIFO */
            __DISPLAY_SPI_DISABLE(hdisplay->DISPLAYx);
            
            /* Work mode */
            hdisplay->DISPLAYx->SPI_CTRL0.SCPOL = hdisplay->SPI_Init.Work_Mode & 0x2 ? 1: 0;
            hdisplay->DISPLAYx->SPI_CTRL0.SCPH  = hdisplay->SPI_Init.Work_Mode & 0x1 ? 1: 0;  
            
            /* Frame Size */
            hdisplay->DISPLAYx->SPI_CTRL0.DFS = hdisplay->SPI_Init.Frame_Size;
            /* BaudRate Prescaler */
            hdisplay->DISPLAYx->SPI_BAUDR = hdisplay->SPI_Init.BaudRate_Prescaler;
            
            /* CS Set */
            hdisplay->DISPLAYx->SPI_SER = 1;
            hdisplay->DISPLAYx->SPI_CTRL0.SS_TGL_EN = 0;
        }break;
        
        default: break;
    }
    /* FIFO_RELEASE */
    __DISPLAY_TX_FIFO_RELEASE(hdisplay->DISPLAYx);
}

/*********************************************************************
 * @fn      display_Parallel_write_cmd
 *
 * @brief   Sending command
 * 
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fp8_CMD : command data
 */
void display_Parallel_write_cmd(DISPLAY_HandTypeDef *hdisplay, uint8_t fp8_CMD)
{
    /* Write, CMD */
    __DISPLAY_PARALLEL_WR_CMD(hdisplay->DISPLAYx, fp8_CMD);
    
    /* wait bus idle */
    while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 
}

/*********************************************************************
 * @fn      display_Parallel_write_param
 *
 * @brief   Sending parameter.
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fu16_Data : parameter. Can be 8 bit or 16 bit, depend on BUS bit.
 */
void display_Parallel_write_param(DISPLAY_HandTypeDef *hdisplay, uint16_t fu16_Data)
{   
    /* Write, param */
    __DISPLAY_PARALLEL_WR_PARAM(hdisplay->DISPLAYx, fu16_Data);
    
    /* wait bus idle */
    while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx));   
}

/*********************************************************************
 * @fn      display_Parallel_write_data
 *
 * @brief   Sending data or parameters
 * 
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fp32_WriteBuffer : Write data buffer
 * @param   fu32_WriteNum    : transmit number.
 *                             1. select DATA_BUS_8_BIT,  1 count sent 1 byte
 *                             2. select DATA_BUS_16_BIT, 1 count sent 2 byte
 */
void display_Parallel_write_data(DISPLAY_HandTypeDef *hdisplay, uint32_t *fp32_WriteBuffer, uint32_t fu32_WriteNum)
{
    uint32_t i;
    uint32_t lu32_Num;

    hdisplay->DISPLAYx->DATA_WR_LEN = fu32_WriteNum;
    
    /* 8 bit bus */
    if (hdisplay->DISPLAYx->INTF_CFG.PARA_WIDTH == DATA_BUS_8_BIT) 
    {
        lu32_Num = fu32_WriteNum / 4;
        
        if (fu32_WriteNum % 4) 
        {
            lu32_Num++;
        }
    }
    /* 16 bit bus */
    else 
    {
        lu32_Num = fu32_WriteNum / 2;
        
        if (fu32_WriteNum % 2) 
        {
            lu32_Num++;
        }
    }
    
    while(lu32_Num >= DISPLAY_FIFO_DEPTH)
    {
        uint32_t u32_l = DISPLAY_FIFO_DEPTH;
            
        while(u32_l--)
        {
            hdisplay->DISPLAYx->TX_FIFO = *fp32_WriteBuffer++;
        }
        
        lu32_Num -= DISPLAY_FIFO_DEPTH;  
            
        while(!(__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_EMPTY));   
    }
    while(lu32_Num--)
    {
        hdisplay->DISPLAYx->TX_FIFO = *fp32_WriteBuffer++;
    }
    /* wait bus idle */
    while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 
}

/*********************************************************************
 * @fn      display_Parallel_read_data_8bit
 *
 * @brief   read data. select DATA_BUS_8_BIT, 1 count receive 1 byte.
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fu8_Param      : read Param
 * @param   fp8_ReadBuffer : read data buffer.
 * @param   fu32_ReadNum   : receive number.
 */
void display_Parallel_read_data_8bit(DISPLAY_HandTypeDef *hdisplay, uint8_t fu8_Param, uint8_t *fp8_ReadBuffer, uint32_t fu32_ReadNum)
{
    uint32_t i;   

    __DISPLAY_PARALLEL_WR_PARAM(hdisplay->DISPLAYx, fu8_Param); 
    
    /* wait bus idle */
    while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 
    
    for (i = 0; i < fu32_ReadNum; i++)
    {
        /* Read  REQ*/
        __DISPLAY_PARALLEL_RD_REQ(hdisplay->DISPLAYx);
        /* wait bus idle */
        while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 

        fp8_ReadBuffer[i] = hdisplay->DISPLAYx->DAT_RD;
    }
}

/*********************************************************************
 * @fn      display_Parallel_read_data_16bit
 *
 * @brief   read data. select DATA_BUS_16_BIT, 1 count receive 2 byte.
 * 
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fu8_Param       : read Param
 * @param   fp16_ReadBuffer : read data buffer.
 * @param   fu32_ReadNum    : receive number.
 */
void display_Parallel_read_data_16bit(DISPLAY_HandTypeDef *hdisplay, uint8_t fu8_Param, uint16_t *fp16_ReadBuffer, uint32_t fu32_ReadNum)
{
    uint32_t i;
    
    __DISPLAY_PARALLEL_WR_PARAM(hdisplay->DISPLAYx, fu8_Param);
    
    /* wait bus idle */
    while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 
    
    for (i = 0; i < fu32_ReadNum; i++)
    {
        /* Read  REQ*/
        __DISPLAY_PARALLEL_RD_REQ(hdisplay->DISPLAYx);
        /* wait bus idle */
        while(__DISPLAY_IS_BUS_BUSY(hdisplay->DISPLAYx)); 

        fp16_ReadBuffer[i] = hdisplay->DISPLAYx->DAT_RD;
    }
}

/*********************************************************************
 * @fn      display_spi_transmit_x1
 *
 * @brief   Send an amount of data in blocking mode.(Standard mode)
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fp_Data  : pointer to data buffer
 * @param   fu32_Size: amount of data to be sent
 */
void display_spi_transmit_x1(DISPLAY_HandTypeDef *hdisplay, void *fp_Data, uint32_t fu32_Size)
{
    uint8_t frame_size;
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;

    /* Disable SPI, reset FIFO */
    __DISPLAY_SPI_DISABLE(hdisplay->DISPLAYx);
    
    /* Select Standard Dual, Quad and Octal mode*/
    __DISPLAY_SPI_SET_MODE_X1(hdisplay->DISPLAYx);

    /* Enable SPI */
    __DISPLAY_SPI_ENABLE(hdisplay->DISPLAYx);

    /* Get current frame size */
    frame_size = __DISPLAY_SPI_DATA_FRAME_SIZE_GET(hdisplay->DISPLAYx);

    p_data.data = fp_Data;
    
    if(frame_size <= SPI_FRAME_SIZE_8BIT)
    {
        while (fu32_Size--) 
        {
            while((__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_FULL));
            
            hdisplay->DISPLAYx->TX_FIFO = *p_data.p_u8++;
        }
    }
    else if(frame_size <= SPI_FRAME_SIZE_16BIT)
    {
        while (fu32_Size--) 
        {
            while((__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_FULL));
            
            hdisplay->DISPLAYx->TX_FIFO = *p_data.p_u16++;
        }
    }
    else
    {
        while (fu32_Size--) 
        {
            while((__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_FULL));
            
            hdisplay->DISPLAYx->TX_FIFO = *p_data.p_u32++;
        }
    }
    
    while(!(__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_EMPTY));
}

/************************************************************************************
 * @fn      display_spi_transmit_X2X4X8_DMA
 *
 * @brief   Send an amount of data in DMA mode.(Dual?Quad and Octal mode)
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 */
void display_spi_transmit_X2X4X8_DMA(DISPLAY_HandTypeDef *hdisplay)
{
    /* Disable SPI, reset FIFO */
    __DISPLAY_SPI_DISABLE(hdisplay->DISPLAYx);

    /* Frame Size */
    __DISPLAY_SPI_DATA_FRAME_SIZE(hdisplay->DISPLAYx, hdisplay->SPI_Init.Frame_Size);    
    
    /* Select Standard Dual, Quad and Octal mode*/
    __DISPLAY_SPI_SET_MODE_X2X4X8(hdisplay->DISPLAYx, hdisplay->SPI_Init.Wire_X2X4X8);
    
    hdisplay->DISPLAYx->SPI_CTRL1.SPI_INST = 0;
    hdisplay->DISPLAYx->SPI_CTRL1.TRANS_TYPE = INST_ADDR_XX;
    hdisplay->DISPLAYx->SPI_CTRL1.SPI_ADDR = 0;

    /* Enable SPI */
    __DISPLAY_SPI_ENABLE(hdisplay->DISPLAYx);
}

/*********************************************************************
 * @fn      display_rgb_write_data
 *
 * @brief   Sending data or parameters
 *
 * @param   hdisplay   : pointer to a DISPLAY_HandTypeDef structure that contains
 *                       the configuration information for 8080/6800/RGB/SPI module
 * @param   fp32_WriteBuffer : Write data buffer
 * @param   fu32_WriteNum    : transmit number.
 */
void display_rgb_write_data(DISPLAY_HandTypeDef *hdisplay, void *fp32_WriteBuffer, uint32_t fu32_WriteNum)
{   
    union {
        void *data;
        uint8_t *p_u8;
        uint16_t *p_u16;
        uint32_t *p_u32;
    } p_data;
    
    p_data.data = fp32_WriteBuffer;

    //Reset RGB
    __DISPLAY_RGB_DISABLE(hdisplay->DISPLAYx);      
    __DISPLAY_RGB_ENABLE(hdisplay->DISPLAYx);  
    
    if(hdisplay->RGB_Init.RGB_OUT_FORMAT_SELECT == RGB565_OUT)
    {
        while(fu32_WriteNum--)
        {
            while((__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_FULL));
            
            hdisplay->DISPLAYx->TX_FIFO = *p_data.p_u16++;
        }        
    }
    else
    {
        while(fu32_WriteNum--)
        {
            while((__DISPLAY_INT_STATUS(hdisplay->DISPLAYx)&INT_TXFIFO_FULL));
            
            hdisplay->DISPLAYx->TX_FIFO = *p_data.p_u32++;
        } 
    }
    
    while(!(__DISPLAY_INT_STATUS(hdisplay->DISPLAYx) & INT_TXFIFO_EMPTY));    
}
