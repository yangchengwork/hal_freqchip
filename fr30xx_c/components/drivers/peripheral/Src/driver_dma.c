/*
  ******************************************************************************
  * @file    driver_dma.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2024
  * @brief   DMA module driver.
  *          This file provides firmware functions to manage the 
  *          Direct Memory Access (DMA) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/*********************************************************************
 * @fn      dma_init
 *
 * @brief   Initialize the DMA according to the specified parameters
 *          in the dma_InitParameter_t 
 *
 * @param   hdma : pointer to a DMA_HandleTypeDef structure that contains
 *                 the configuration information for DMA module
 *          
 * @return  None.
 */
void dma_init(DMA_HandleTypeDef *hdma)
{
    struct_DMA_t *DMA = hdma->DMAx;
    
    /* FIFO_MODE_1 */
    DMA->Channels[hdma->Channel].CFG2.FIFO_MODE = 1;

    /* Clear Linked List Pointer Register */
    DMA->Channels[hdma->Channel].LLP.LOC = 0;
    DMA->Channels[hdma->Channel].LLP.LMS = 0;

    /* Interrupt Enable */
    DMA->Channels[hdma->Channel].CTL1.INT_EN = 1;
    
    /* Bus Master Selection */
    DMA->Channels[hdma->Channel].CTL1.SMS = hdma->Init.Source_Master_Sel;
    DMA->Channels[hdma->Channel].CTL1.DMS = hdma->Init.Desination_Master_Sel;

    /* Transfer Width */
    DMA->Channels[hdma->Channel].CTL1.SRC_TR_WIDTH = hdma->Init.Source_Width;
    DMA->Channels[hdma->Channel].CTL1.DST_TR_WIDTH = hdma->Init.Desination_Width;
    
    /* Burst Length */
    DMA->Channels[hdma->Channel].CTL1.DEST_MSIZE = hdma->Init.Desination_Burst_Len;
    DMA->Channels[hdma->Channel].CTL1.SRC_MSIZE  = hdma->Init.Source_Burst_Len;

    /* Address Increment */
    DMA->Channels[hdma->Channel].CTL1.SINC = hdma->Init.Source_Inc;
    DMA->Channels[hdma->Channel].CTL1.DINC = hdma->Init.Desination_Inc;

    /* DMA Data Flow */
    DMA->Channels[hdma->Channel].CTL1.TT_FC = hdma->Init.Data_Flow;

    /* Set Peripheral Request ID */
    if (hdma->Init.Data_Flow == DMA_M2P_DMAC) 
    {
        DMA->Channels[hdma->Channel].CFG2.DEST_PER = hdma->Init.Request_ID;
    }
    else if (hdma->Init.Data_Flow == DMA_P2M_DMAC) 
    {
        DMA->Channels[hdma->Channel].CFG2.SRC_PER = hdma->Init.Request_ID;
    }

    /* Hardware handshaking interface */
    DMA->Channels[hdma->Channel].CFG1.HS_SEL_SRC = 0;
    DMA->Channels[hdma->Channel].CFG1.HS_SEL_DST = 0;
	
    /* AHB dmac Enabled */
    DMA->Misc_Reg.DmaCfgReg.DMA_EN = 1;
}

/*********************************************************************
 * @fn      dma_start
 *
 * @brief   DMA transfer start. 
 *
 * @param   hdma : pointer to a DMA_HandleTypeDef structure that contains
 *                 the configuration information for DMA module
 *          SrcAddr:  source address
 *          DstAddr:  desination address
 *          Size:     transfer size (This parameter can be a 12-bit Size)
 * @return  None.
 */
void dma_start(DMA_HandleTypeDef *hdma, uint32_t SrcAddr, uint32_t DstAddr, uint32_t Size)
{
    struct_DMA_t *DMA = hdma->DMAx;
    
    /* Set source address and desination address */
    DMA->Channels[hdma->Channel].SAR = SrcAddr;
    DMA->Channels[hdma->Channel].DAR = DstAddr;
    
    /* Set Transfer Size */
    DMA->Channels[hdma->Channel].CTL2.BLOCK_TS = Size;

    /* Enable the channel */
    DMA->Misc_Reg.ChEnReg |= (1 << hdma->Channel | (1 << (hdma->Channel + 8)));
}

/*********************************************************************
 * @fn      dma_start_IT
 *
 * @brief   DMA transfer start and enable interrupt
 *
 * @param   hdma : pointer to a DMA_HandleTypeDef structure that contains
 *                 the configuration information for DMA module
 *          SrcAddr:  source address
 *          DstAddr:  desination address
 *          Size:     transfer size (This parameter can be a 12-bit Size)
 * @return  None.
 */
void dma_start_IT(DMA_HandleTypeDef *hdma, uint32_t SrcAddr, uint32_t DstAddr, uint32_t Size)
{
    struct_DMA_t *DMA = hdma->DMAx;
    
    /* Set source address and desination address */
    DMA->Channels[hdma->Channel].SAR = SrcAddr;
    DMA->Channels[hdma->Channel].DAR = DstAddr;
    
    /* Set Transfer Size */
    DMA->Channels[hdma->Channel].CTL2.BLOCK_TS = Size;

    /* Clear Transfer complete status */
    dma_clear_tfr_Status(hdma);
    /* channel Transfer complete interrupt enable */
    dma_tfr_interrupt_enable(hdma);

    /* Enable the channel */
    DMA->Misc_Reg.ChEnReg |= (1 << hdma->Channel | (1 << (hdma->Channel + 8)));
}

/*********************************************************************
 * @fn      dma_linked_list_init
 *
 * @brief   Initialize the linked list parameters
 *
 * @param   link : The list structure that needs to be initialized
 *          param: Initialization parameter
 * @return  None
 */
void dma_linked_list_init(DMA_LLI_InitTypeDef *link, dma_LinkParameter_t *param)
{
    link->SrcAddr           = param->SrcAddr;
    link->DstAddr           = param->DstAddr;
    
    link->Next = ((DMA_LLI_InitTypeDef *)(param->NextLink | param->Linked_Master_Sel));        

    link->CTL1.INT_EN       = 1;
    link->CTL1.SRC_TR_WIDTH = param->Source_Width;
    link->CTL1.DST_TR_WIDTH = param->Desination_Width;
    link->CTL1.SMS          = param->Source_Master_Sel;
    link->CTL1.DMS          = param->Desination_Master_Sel;
    
    link->CTL1.SINC         = param->Source_Inc;
    link->CTL1.DINC         = param->Desination_Inc;
    link->CTL1.TT_FC        = param->Data_Flow;
    link->CTL1.DEST_MSIZE   = param->Desination_Burst_Len;
    link->CTL1.SRC_MSIZE    = param->Source_Burst_Len;
    link->CTL1.SRC_GATHER_EN  = param->gather_enable;
    link->CTL1.DST_SCATTER_EN = param->scatter_enable;
    link->CTL2.BLOCK_TS       = param->Size;

    /* Block chaining using Linked List is enabled on the Source side */
    /* Block chaining using Linked List is enabled on the Destination side */
    link->CTL1.LLP_DST_EN = 1;
    link->CTL1.LLP_SRC_EN = 1;
}

/*********************************************************************
 * @fn      dma_linked_list_start. 
 *
 * @brief   DMA linked list transfer start. 
 *
 * @param   link    : The first address of the linked list
 *          param   : Initialization parameter
 *          hdma    : DMA handle
 *
 * @return  None
 */
void dma_linked_list_start(DMA_HandleTypeDef *hdma, DMA_LLI_InitTypeDef *link, dma_LinkParameter_t *param)
{
    struct_DMA_t *DMA = hdma->DMAx;
    uint32_t Channel = hdma->Channel;

    /* Set Peripheral Request ID */
    if (link->CTL1.TT_FC == DMA_M2P_DMAC) 
    {
        DMA->Channels[Channel].CFG2.DEST_PER = param->Request_ID;
    }
    else if (link->CTL1.TT_FC == DMA_P2M_DMAC) 
    {
        DMA->Channels[Channel].CFG2.SRC_PER = param->Request_ID;
    }

    /* Hardware handshaking interface */
    DMA->Channels[Channel].CFG1.HS_SEL_SRC = 0;
    DMA->Channels[Channel].CFG1.HS_SEL_DST = 0;

    /* Block chaining using Linked List is enabled on the Source side */
    /* Block chaining using Linked List is enabled on the Destination side */
    DMA->Channels[Channel].CTL1.LLP_DST_EN = link->CTL1.LLP_DST_EN;
    DMA->Channels[Channel].CTL1.LLP_SRC_EN = link->CTL1.LLP_SRC_EN;
    
    DMA->Channels[Channel].LLP.LMS =  param->Linked_Master_Sel;    
    DMA->Channels[Channel].LLP.LOC = ((uint32_t)link) >> 2;

    /* AHB dmac Enabled */
    DMA->Misc_Reg.DmaCfgReg.DMA_EN = 1;

    DMA->Misc_Reg.ChEnReg |= (1 << Channel | (1 << (Channel + 8)));
}

/*********************************************************************
 * @fn      dma_linked_list_start_IT 
 *
 * @brief   DMA linked list transfer start and enable interrupt
 *
 * @param   link    : The first address of the linked list
 *          param   : Initialization parameter
 *          hdma    : DMA handle
 *
 * @return  None
 */
void dma_linked_list_start_IT(DMA_HandleTypeDef *hdma, DMA_LLI_InitTypeDef *link, dma_LinkParameter_t *param)
{
    struct_DMA_t *DMA = hdma->DMAx;
    uint32_t Channel = hdma->Channel;

    /* Set Peripheral Request ID */
    if (link->CTL1.TT_FC == DMA_M2P_DMAC) 
    {
        DMA->Channels[Channel].CFG2.DEST_PER = param->Request_ID;
    }
    else if (link->CTL1.TT_FC == DMA_P2M_DMAC) 
    {
        DMA->Channels[Channel].CFG2.SRC_PER = param->Request_ID;
    }

    /* Hardware handshaking interface */
    DMA->Channels[Channel].CFG1.HS_SEL_SRC = 0;
    DMA->Channels[Channel].CFG1.HS_SEL_DST = 0;

    DMA->Channels[Channel].CTL1.LLP_DST_EN = link->CTL1.LLP_DST_EN;
    DMA->Channels[Channel].CTL1.LLP_SRC_EN = link->CTL1.LLP_SRC_EN;
    
    DMA->Channels[Channel].LLP.LMS =  param->Linked_Master_Sel;      
    DMA->Channels[Channel].LLP.LOC = ((uint32_t)link) >> 2;

    /* Clear Transfer complete status */
    dma_clear_tfr_Status(hdma);
    /* channel Transfer complete interrupt enable */
    dma_tfr_interrupt_enable(hdma);
    
    /* AHB dmac Enabled */
    DMA->Misc_Reg.DmaCfgReg.DMA_EN = 1;

    DMA->Misc_Reg.ChEnReg |= (1 << Channel | (1 << (Channel + 8)));
}

/*********************************************************************
 * @fn      dma_tfr_interrupt_enable
 *
 * @brief   channel transfer complete interrupt enable
 *
 * @param   hdma : DMA handle 
 *
 * @return  None
 */
void dma_tfr_interrupt_enable(DMA_HandleTypeDef *hdma)
{
    hdma->DMAx->Int_Reg.MaskTfr = (1 << (hdma->Channel)) | (1 << ((hdma->Channel) + 8));
}

/*********************************************************************
 * @fn      dma_tfr_interrupt_disable
 *
 * @brief   channel transfer complete interrupt disable
 *
 * @param   hdma : DMA handle  
 *
 * @return  None
 */
void dma_tfr_interrupt_disable(DMA_HandleTypeDef *hdma)
{
    hdma->DMAx->Int_Reg.MaskTfr = (1 << ((hdma->Channel) + 8));
}

/*********************************************************************
 * @fn      dma_get_tfr_Status
 *
 * @brief   Get channel transfer complete status
 *
 * @param   hdma : DMA handle 
 *
 * @return  true:  channel Transfer complete
 *          false: Not
 */
bool dma_get_tfr_Status(DMA_HandleTypeDef *hdma)
{
    /* Check channel Transfer complete */
    if (hdma->DMAx->Int_Reg.RawTfr & (1 << (hdma->Channel))) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

/*********************************************************************
 * @fn      dma_clear_tfr_Status
 *
 * @brief   clear channel transfer complete status
 *
 * @param   hdma : DMA handle 
 *
 * @return  None
 */
void dma_clear_tfr_Status(DMA_HandleTypeDef *hdma)
{
    /* Clear channel Transfer complete Status */
    hdma->DMAx->Int_Reg.ClearTfr = 1 << (hdma->Channel);
}

/*********************************************************************
 * @fn      dma_tfr_interrupt_enable
 *
 * @brief   channel transfer complete interrupt enable
 *
 * @param   hdma : DMA handle 
 *
 * @return  None
 */
void dma_error_interrupt_enable(DMA_HandleTypeDef *hdma)
{
    hdma->DMAx->Int_Reg.MaskErr = (1 << (hdma->Channel)) | (1 << ((hdma->Channel) + 8));
}

/*********************************************************************
 * @fn      dma_tfr_interrupt_disable
 *
 * @brief   channel transfer complete interrupt disable
 *
 * @param   hdma : DMA handle  
 *
 * @return  None
 */
void dma_error_interrupt_disable(DMA_HandleTypeDef *hdma)
{
    hdma->DMAx->Int_Reg.MaskErr = (1 << ((hdma->Channel) + 8));
}

/*********************************************************************
 * @fn      dma_get_tfr_Status
 *
 * @brief   Get channel transfer complete status
 *
 * @param   hdma : DMA handle 
 *
 * @return  true:  channel Transfer complete
 *          false: Not
 */
bool dma_get_error_Status(DMA_HandleTypeDef *hdma)
{
    /* Check channel Transfer complete */
    if (hdma->DMAx->Int_Reg.RawErr & (1 << (hdma->Channel))) 
    {
        return true;
    }
    else 
    {
        return false;
    }
}

/*********************************************************************
 * @fn      dma_clear_tfr_Status
 *
 * @brief   clear channel transfer complete status
 *
 * @param   hdma : DMA handle 
 *
 * @return  None
 */
void dma_clear_error_Status(DMA_HandleTypeDef *hdma)
{
    /* Clear channel Transfer complete Status */
    hdma->DMAx->Int_Reg.ClearErr = 1 << (hdma->Channel);
}
