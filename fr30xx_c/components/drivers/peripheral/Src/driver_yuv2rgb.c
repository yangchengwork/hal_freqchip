/*
  ******************************************************************************
  * @file    driver_yuv2rgb.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   YUV2RGB module driver.
  *          This file provides firmware functions to manage the YUV2RGB.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      yuv2rgb_init
 *
 * @brief   Initialize YUV2RGB UART according to the specified parameters 
 *          in the struct_YUV2RGBInit_t.
 *
 * @param   hyuv2rgb: YUV2RGB handle.
 */
void yuv2rgb_init(YUV2RGB_HandleTypeDef *hyuv2rgb)
{
    /* YUV2RGB enable */
    __YUV2RGB_ENABLE(hyuv2rgb->YUV2RGBx);
    /* DMA default enable */
    __YUV2RGB_DMA_ENABLE(hyuv2rgb->YUV2RGBx);
    
    /* Set YUV2RGB format, calculate Mode */
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.RGB_FORMAT = hyuv2rgb->Init.RGB_Format;
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.YUV_FORMAT = hyuv2rgb->Init.YUV_Format;
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.YUV_MODE   = hyuv2rgb->Init.YUV_CalculateMode;

    if (hyuv2rgb->Init.YUV_Format == YUV_FORMAT_444)
        hyuv2rgb->YUV2RGBx->FLOW_CTRL.YUV_FLOW_LEVEL = 1;
    else
        hyuv2rgb->YUV2RGBx->FLOW_CTRL.YUV_FLOW_LEVEL = 2;

    hyuv2rgb->YUV2RGBx->FLOW_CTRL.RGB_FLOW_LEVEL = 30;
}

/************************************************************************************
 * @fn      dvp_init
 *
 * @brief   Initialize dvp init according to the specified parameters 
 *          in the struct_DVPInit_t.
 *
 * @param   hyuv2rgb: YUV2RGB handle.
 */
void dvp_init(YUV2RGB_HandleTypeDef *hyuv2rgb)
{
    /* YUV2RGB enable */
    __YUV2RGB_ENABLE(hyuv2rgb->YUV2RGBx);
    /* DMA default enable */
    __YUV2RGB_DMA_ENABLE(hyuv2rgb->YUV2RGBx);
    
    /* Set YUV2RGB format, calculate Mode */
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.RGB_FORMAT = hyuv2rgb->Init.RGB_Format;
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.YUV_FORMAT = hyuv2rgb->Init.YUV_Format;
    hyuv2rgb->YUV2RGBx->YUV2RGB_CFG.YUV_MODE   = hyuv2rgb->Init.YUV_CalculateMode;

    hyuv2rgb->YUV2RGBx->FLOW_CTRL.RGB_FLOW_LEVEL = 16;
    
    /* DVP reset */ 
    __DVP_RESET(hyuv2rgb->YUV2RGBx); 
    /* Set DVP format */   
    hyuv2rgb->YUV2RGBx->DVP_CFG.BUS_WIDTH_SEL = hyuv2rgb->DVP_Init.DVP_Width;
    /* Set DVP format cut */    
    if(hyuv2rgb->DVP_Init.CUT_Enable == CUT_XADDR_EN)
    {
        hyuv2rgb->YUV2RGBx->XADDR_CUT.CHSTR = hyuv2rgb->DVP_Init.CUT_XADDR_Start;
        hyuv2rgb->YUV2RGBx->XADDR_CUT.CHNUM = hyuv2rgb->DVP_Init.CUT_XADDR_length;         
    }
    else if(hyuv2rgb->DVP_Init.CUT_Enable == CUT_YADDR_EN)
    {
        hyuv2rgb->YUV2RGBx->YADDR_CUT.CVSTR = hyuv2rgb->DVP_Init.CUT_YADDR_Start;
        hyuv2rgb->YUV2RGBx->YADDR_CUT.CVNUM = hyuv2rgb->DVP_Init.CUT_YADDR_length;          
    }
    else
    {
        hyuv2rgb->YUV2RGBx->XADDR_CUT.CHSTR = hyuv2rgb->DVP_Init.CUT_XADDR_Start;
        hyuv2rgb->YUV2RGBx->XADDR_CUT.CHNUM = hyuv2rgb->DVP_Init.CUT_XADDR_length;  
        hyuv2rgb->YUV2RGBx->YADDR_CUT.CVSTR = hyuv2rgb->DVP_Init.CUT_YADDR_Start;
        hyuv2rgb->YUV2RGBx->YADDR_CUT.CVNUM = hyuv2rgb->DVP_Init.CUT_YADDR_length;        
    }
    
    /* set VSYNC HSYNC DCLK polarity Active high*/       
    __DVP_SET_VSYNC_POLARITY_HIGH(hyuv2rgb->YUV2RGBx); 
    __DVP_SET_HSYNC_POLARITY_HIGH(hyuv2rgb->YUV2RGBx); 
    /* set DCLK Data are collected along the rising edge*/     
    __DVP_SET_DCLK_POLARITY_HIGH(hyuv2rgb->YUV2RGBx);
    
    /* Enable DVP*/  
    __DVP_ENABLE(hyuv2rgb->YUV2RGBx);    
}

/************************************************************************************
 * @fn      yuv2rgb_convert
 *
 * @brief   YUV to RGB convert in blocking mode.
 *
 * @param   hyuv2rgb: YUV2RGB handle.
 *          YUV_Buffer:  YUV data buffer.
 *          RGB_BUffer:  RGB data buffer.
 *          fu32_Pixels: 
 */
void yuv2rgb_convert(YUV2RGB_HandleTypeDef *hyuv2rgb, void *YUV_Buffer, void *RGB_Buffer, uint32_t fu32_Pixels)
{
    uint32_t lu32_RGBCount = 0;
    uint32_t lu32_YUVCount = 0;

    hyuv2rgb->u_YUVData.p_data = YUV_Buffer;
    hyuv2rgb->u_RGBData.p_data = RGB_Buffer;

    while(true)
    {
        if (!(__YUV2RGB_GET_INT_RAW_STATUS(hyuv2rgb->YUV2RGBx) & RGB_FIFO_EMPTY))
        {
            if (lu32_RGBCount < fu32_Pixels)
            {
                switch (hyuv2rgb->Init.RGB_Format)
                {
                    case RGB_FORMAT_888: *hyuv2rgb->u_RGBData.p_u32++ = hyuv2rgb->YUV2RGBx->RGB_DATA; break;
                    case RGB_FORMAT_565: *hyuv2rgb->u_RGBData.p_u16++ = hyuv2rgb->YUV2RGBx->RGB_DATA; break;
                    case RGB_FORMAT_332: *hyuv2rgb->u_RGBData.p_u8++  = hyuv2rgb->YUV2RGBx->RGB_DATA; break;
                    default:break;
                }
                lu32_RGBCount++;
                if (lu32_RGBCount >= fu32_Pixels)
                    break;
            }
        }

        if (!(__YUV2RGB_GET_INT_RAW_STATUS(hyuv2rgb->YUV2RGBx) & YUV_FIFO_FULL))
        {
            if (lu32_YUVCount < fu32_Pixels)
            {
                switch (hyuv2rgb->Init.YUV_Format)
                {
                    case YUV_FORMAT_444: hyuv2rgb->YUV2RGBx->YUV_DATA = *hyuv2rgb->u_YUVData.p_u32++; break;
                    case YUV_FORMAT_422: hyuv2rgb->YUV2RGBx->YUV_DATA = *hyuv2rgb->u_YUVData.p_u16++; break;
                    default: break;
                }

                lu32_YUVCount++;
            }
        }
    }
}
