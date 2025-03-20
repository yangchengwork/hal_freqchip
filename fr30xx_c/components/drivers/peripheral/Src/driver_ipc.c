/*
  ******************************************************************************
  * @file    driver_ipc.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2021
  * @brief   ipc module driver.
  *          This file provides firmware functions to manage the 
  *          IPC peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      ipc_IRQHandler
 *
 * @brief   Handle IPC interrupt request.
 *
 * @param   hipc: IPC handle.
 */
void ipc_IRQHandler(IPC_HandleTypeDef *hipc)
{
    uint32_t status;
    volatile uint32_t msg;
    
    status = hipc->IPCx->IPC_ISR_STA.Word;
    if (status & IPC_INT_STATUS_CH0_IN) {
        msg = hipc->IPCx->IPC_MSG_IN0;
        if (hipc->RxCallback) {
            hipc->RxCallback(hipc, IPC_CH_0, msg);
        }
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH0_IN;
        hipc->IPCx->IPC_MSG_CLR.CH0_IN = 1;
    }
    if (status & IPC_INT_STATUS_CH1_IN) {
        msg = hipc->IPCx->IPC_MSG_IN1;
        if (hipc->RxCallback) {
            hipc->RxCallback(hipc, IPC_CH_1, msg);
        }
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH1_IN;
        hipc->IPCx->IPC_MSG_CLR.CH1_IN = 1;
    }
    if (status & IPC_INT_STATUS_CH2_IN) {
        msg = hipc->IPCx->IPC_MSG_IN2;
        if (hipc->RxCallback) {
            hipc->RxCallback(hipc, IPC_CH_2, msg);
        }
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH2_IN;
        hipc->IPCx->IPC_MSG_CLR.CH2_IN = 1;
    }
    if (status & IPC_INT_STATUS_CH3_IN) {
        msg = hipc->IPCx->IPC_MSG_IN3;
        if (hipc->RxCallback) {
            hipc->RxCallback(hipc, IPC_CH_3, msg);
        }
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH3_IN;
        hipc->IPCx->IPC_MSG_CLR.CH3_IN = 1;
    }

    status = hipc->IPCx->IPC_ISR_STA.Word;
    if ((status & IPC_INT_STATUS_CH0_OUT)) {
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH0_OUT;
        if (hipc->TxCallback) {
            hipc->TxCallback(hipc, IPC_CH_0);
        }
    }
    if ((status & IPC_INT_STATUS_CH1_OUT)) {
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH1_OUT;
        if (hipc->TxCallback) {
            hipc->TxCallback(hipc, IPC_CH_1);
        }
    }
    if ((status & IPC_INT_STATUS_CH2_OUT)) {
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH2_OUT;
        if (hipc->TxCallback) {
            hipc->TxCallback(hipc, IPC_CH_2);
        }
    }
    if ((status & IPC_INT_STATUS_CH3_OUT)) {
        hipc->IPCx->IPC_ISR_STA.Word = IPC_INT_STATUS_CH3_OUT;
        if (hipc->TxCallback) {
            hipc->TxCallback(hipc, IPC_CH_3);
        }
    }
}

void ipc_init(IPC_HandleTypeDef *hipc)
{
    if (hipc->RxEnableChannels & IPC_CH_0) {
        hipc->IPCx->IPC_CTRL.Bits.CH0_IN_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH0_IN = 1;
    }
    if (hipc->RxEnableChannels & IPC_CH_1) {
        hipc->IPCx->IPC_CTRL.Bits.CH1_IN_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH1_IN = 1;
    }
    if (hipc->RxEnableChannels & IPC_CH_2) {
        hipc->IPCx->IPC_CTRL.Bits.CH2_IN_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH2_IN = 1;
    }
    if (hipc->RxEnableChannels & IPC_CH_3) {
        hipc->IPCx->IPC_CTRL.Bits.CH3_IN_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH3_IN = 1;
    }
    
    if (hipc->TxEnableChannels & IPC_CH_0) {
        hipc->IPCx->IPC_CTRL.Bits.CH0_OUT_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH0_OUT = 1;
    }
    if (hipc->TxEnableChannels & IPC_CH_1) {
        hipc->IPCx->IPC_CTRL.Bits.CH1_OUT_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH1_OUT = 1;
    }
    if (hipc->TxEnableChannels & IPC_CH_2) {
        hipc->IPCx->IPC_CTRL.Bits.CH2_OUT_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH2_OUT = 1;
    }
    if (hipc->TxEnableChannels & IPC_CH_3) {
        hipc->IPCx->IPC_CTRL.Bits.CH3_OUT_EN = 1;
        hipc->IPCx->IPC_ISR_EN.Bits.CH3_OUT = 1;
    }
    
    hipc->TxOngoingChannels = 0;
}

void ipc_msg_send(IPC_HandleTypeDef *hipc, enum_IPC_Chl_Sel_t ch, uint32_t msg)
{
    switch (ch) {
        case IPC_CH_0:
            while (hipc->IPCx->IPC_PENDING.Bits.CH0_OUT);
            hipc->IPCx->IPC_MSG_OUT0 = msg;
            break;
        case IPC_CH_1:
            while (hipc->IPCx->IPC_PENDING.Bits.CH1_OUT);
            hipc->IPCx->IPC_MSG_OUT1 = msg;
            break;
        case IPC_CH_2:
            while (hipc->IPCx->IPC_PENDING.Bits.CH2_OUT);
            hipc->IPCx->IPC_MSG_OUT2 = msg;
            break;
        case IPC_CH_3:
            while (hipc->IPCx->IPC_PENDING.Bits.CH3_OUT);
            hipc->IPCx->IPC_MSG_OUT3 = msg;
            break;
        default:
            return;
    }
    
    hipc->TxOngoingChannels |= ch;
}
