/*
  ******************************************************************************
  * @file    driver_can.c
  * @author  FreqChip Firmware Team
  * @version V1.0.0
  * @date    2022
  * @brief   SD controller HAL module driver.
  *          This file provides firmware functions to manage the 
  *          Controller Area Network (CAN) peripheral
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 FreqChip.
  * All rights reserved.
  ******************************************************************************
*/
#include "fr30xx.h"

/************************************************************************************
 * @fn      can_IRQHandler
 *
 * @brief   Handle CAN interrupt request.
 *
 * @param   hcan: CAN handle.
 */
__WEAK void can_IRQHandler(CAN_HandleTypeDef *hcan)
{
    uint32_t fu32_int_source;

    fu32_int_source = can_get_int_status(hcan);

    /* Rx FIFO0 New Message */
    if (fu32_int_source & INT_RxFIFO0_NEW_MESSAGE)
    {
        can_clear_int_status(hcan, INT_RxFIFO0_NEW_MESSAGE);

        if (hcan->RxFIFO0_New_Message_Callback)
        {
            hcan->RxFIFO0_New_Message_Callback(hcan);
        }
    }
    /* Rx FIFO0 Watermark Reached */
    if (fu32_int_source & INT_RxFIFO0_WATERMARK_REACHED)
    {
        can_clear_int_status(hcan, INT_RxFIFO0_WATERMARK_REACHED);

        if (hcan->RxFIFO0_Watermark_Reached_Callback)
        {
            hcan->RxFIFO0_Watermark_Reached_Callback(hcan);
        }
    }
    /* Rx FIFO0 full */
    if (fu32_int_source & INT_RxFIFO0_FULL)
    {
        can_clear_int_status(hcan, INT_RxFIFO0_FULL);

        if (hcan->RxFIFO0_Full_Callback)
        {
            hcan->RxFIFO0_Full_Callback(hcan);
        }
    }
    /* Rx FIFO0 Message Lost */
    if (fu32_int_source & INT_RxFIFO0_MESSAGE_LOST)
    {
        can_clear_int_status(hcan, INT_RxFIFO0_MESSAGE_LOST);

        if (hcan->RxFIFO0_Message_Lost_Callback)
        {
            hcan->RxFIFO0_Message_Lost_Callback(hcan);
        }
    }


    /* Rx FIFO1 New Message */
    if (fu32_int_source & INT_RxFIFO1_NEW_MESSAGE)
    {
        can_clear_int_status(hcan, INT_RxFIFO1_NEW_MESSAGE);

        if (hcan->RxFIFO1_New_Message_Callback)
        {
            hcan->RxFIFO1_New_Message_Callback(hcan);
        }
    }
    /* Rx FIFO1 Watermark Reached */
    if (fu32_int_source & INT_RxFIFO1_WATERMARK_REACHED)
    {
        can_clear_int_status(hcan, INT_RxFIFO1_WATERMARK_REACHED);

        if (hcan->RxFIFO1_Watermark_Reached_Callback)
        {
            hcan->RxFIFO1_Watermark_Reached_Callback(hcan);
        }
    }
    /* Rx FIFO1 full */
    if (fu32_int_source & INT_RxFIFO1_FULL)
    {
        can_clear_int_status(hcan, INT_RxFIFO1_FULL);

        if (hcan->RxFIFO1_Full_Callback)
        {
            hcan->RxFIFO1_Full_Callback(hcan);
        }
    }
    /* Rx FIFO1 Message Lost */
    if (fu32_int_source & INT_RxFIFO1_MESSAGE_LOST)
    {
        can_clear_int_status(hcan, INT_RxFIFO1_MESSAGE_LOST);

        if (hcan->RxFIFO1_Message_Lost_Callback)
        {
            hcan->RxFIFO1_Message_Lost_Callback(hcan);
        }
    }

    /* Receive High Priority Message */
    if (fu32_int_source & INT_HIGH_PRIORITY_MESSAGE)
    {
        can_clear_int_status(hcan, INT_HIGH_PRIORITY_MESSAGE);

        if (hcan->Receive_High_Priority_Message_Callback)
        {
            hcan->Receive_High_Priority_Message_Callback(hcan);
        }
    }

    /* Transmission Completed */
    if (fu32_int_source & INT_TRANSMISSION_COMPLETED)
    {
        can_clear_int_status(hcan, INT_TRANSMISSION_COMPLETED);

        if (hcan->Transmission_Completed_Callback)
        {
            hcan->Transmission_Completed_Callback(hcan);
        }
    }
    /* Transmission Cancellation Finished */
    if (fu32_int_source & INT_TRANSMISSION_CANCELLATION_FINISHED)
    {
        can_clear_int_status(hcan, INT_TRANSMISSION_CANCELLATION_FINISHED);

        if (hcan->Transmission_Cancellation_Finished_Callback)
        {
            hcan->Transmission_Cancellation_Finished_Callback(hcan);
        }
    }
    /* TxFIFO Empty */
    if (fu32_int_source & INT_TxFIFO_EMPTY)
    {
        can_clear_int_status(hcan, INT_TxFIFO_EMPTY);

        if (hcan->TxFIFO_Empty_Callback)
        {
            hcan->TxFIFO_Empty_Callback(hcan);
        }
    }

#ifdef CAN_TX_EVENT_FUNCTION_EXIST
    /* Tx Event FIFO New Entry */
    if (fu32_int_source & INT_TxEVENT_NEW_ENTRY)
    {
        can_clear_int_status(hcan, INT_TxEVENT_NEW_ENTRY);

        if (hcan->TxEventFIFO_New_Entry_Callback)
        {
            hcan->TxEventFIFO_New_Entry_Callback(hcan);
        }
    }
    /* Tx Event Watermark Reached */
    if (fu32_int_source & INT_TxEVENT_WATERMARK_REACHED)
    {
        can_clear_int_status(hcan, INT_TxEVENT_WATERMARK_REACHED);

        if (hcan->TxEventFIFO_Watermark_Reached_Callback)
        {
            hcan->TxEventFIFO_Watermark_Reached_Callback(hcan);
        }
    }
    /* Tx Event Full */
    if (fu32_int_source & INT_TxEVENT_FULL)
    {
        can_clear_int_status(hcan, INT_TxEVENT_FULL);

        if (hcan->TxEventFIFO_Full_Callback)
        {
            hcan->TxEventFIFO_Full_Callback(hcan);
        }
    }
    /* Tx Event lost */
    if (fu32_int_source & INT_TxEVENT_ELEMENT_LOST)
    {
        can_clear_int_status(hcan, INT_TxEVENT_ELEMENT_LOST);

        if (hcan->TxEventFIFO_Lost_Callback)
        {
            hcan->TxEventFIFO_Lost_Callback(hcan);
        }
    }
#endif

    /* Message Ram Access Failure */
    if (fu32_int_source & INT_MESSAGE_RAM_ACCESS_FAILURE)
    {
        can_clear_int_status(hcan, INT_MESSAGE_RAM_ACCESS_FAILURE);

        if (hcan->Message_Ram_Access_Failure_Callback)
        {
            hcan->Message_Ram_Access_Failure_Callback(hcan);
        }
    }
    /* Timeout Occurred */
    if (fu32_int_source & INT_TIMEOUT_OCCURRED)
    {
        can_clear_int_status(hcan, INT_TIMEOUT_OCCURRED);

        if (hcan->Timeout_Occurred_Callback)
        {
            hcan->Timeout_Occurred_Callback(hcan);
        }
    }
    /* Message Stored To RxBuffer */
    if (fu32_int_source & INT_MESSAGE_STORED_TO_RX_BUFFER)
    {
        can_clear_int_status(hcan, INT_MESSAGE_STORED_TO_RX_BUFFER);

        if (hcan->Message_Stored_To_RxBuffer_Callback)
        {
            hcan->Message_Stored_To_RxBuffer_Callback(hcan);
        }
    }
    /* Bit Error Corrected */
    if (fu32_int_source & INT_BIT_ERROR_CORRECTED)
    {
        can_clear_int_status(hcan, INT_BIT_ERROR_CORRECTED);

        if (hcan->Bit_Error_Corrected_Callback)
        {
            hcan->Bit_Error_Corrected_Callback(hcan);
        }
    }
    /* Bit Error uncorrected */
    if (fu32_int_source & INT_BIT_ERROR_UNCORRECTED)
    {
        can_clear_int_status(hcan, INT_BIT_ERROR_UNCORRECTED);

        if (hcan->Bit_Error_Uncorrected_Callback)
        {
            hcan->Bit_Error_Uncorrected_Callback(hcan);
        }
    }
    /* Error Log Overflow */
    if (fu32_int_source & INT_ERROR_LOG_OVERFLOW)
    {
        can_clear_int_status(hcan, INT_ERROR_LOG_OVERFLOW);

        if (hcan->Error_Log_Overflow_Callback)
        {
            hcan->Error_Log_Overflow_Callback(hcan);
        }
    }
    /* Error Passive */
    if (fu32_int_source & INT_ERROR_PASSIVE)
    {
        can_clear_int_status(hcan, INT_ERROR_PASSIVE);

        if (hcan->Error_Passive_Callback)
        {
            hcan->Error_Passive_Callback(hcan);
        }
    }
    /* Warning */
    if (fu32_int_source & INT_WARNING_STATUS)
    {
        can_clear_int_status(hcan, INT_WARNING_STATUS);

        if (hcan->Warning_Callback)
        {
            hcan->Warning_Callback(hcan);
        }
    }
    /* BUS OFF */
    if (fu32_int_source & INT_BUS_OFF_STATUS)
    {
        can_clear_int_status(hcan, INT_BUS_OFF_STATUS);

        if (hcan->Bus_Off_Callback)
        {
            hcan->Bus_Off_Callback(hcan);
        }
    }
    /* ram wdt */
    if (fu32_int_source & INT_WATCHDOG_INTERRUPT)
    {
        can_clear_int_status(hcan, INT_WATCHDOG_INTERRUPT);

        if (hcan->RAM_WDT_Callback)
        {
            hcan->RAM_WDT_Callback(hcan);
        }
    }
    /* Protocol Error */
    if (fu32_int_source & INT_PROTOCOL_ERROR)
    {
        can_clear_int_status(hcan, INT_PROTOCOL_ERROR);

        if (hcan->Protocol_Error_Callback)
        {
            hcan->Protocol_Error_Callback(hcan);
        }
    }
    /* Protocol Error FD */
    if (fu32_int_source & INT_PROTOCOL_ERROR_FD)
    {
        can_clear_int_status(hcan, INT_PROTOCOL_ERROR_FD);

        if (hcan->Protocol_Error_FD_Callback)
        {
            hcan->Protocol_Error_FD_Callback(hcan);
        }
    }
}

/************************************************************************************
 * @fn      can_init
 *
 * @brief   Initializes the CAN peripheral according to the specified 
 *          parameters in the struct_CANInit_t
 *
 * @param   hcan: CAN handle.
 */
void can_init(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    /* Timestamp enable */
    __CAN_SET_TIMESTAMP_SELECT_INTERNAL(hcan->CANx);

    /* Bit Rate Prescaler */
    __CAN_SET_NOMINAL_RATE_PRESCALER(hcan->CANx, hcan->Init.Prescaler);
    /* Data time segment1/segment2 */
    __CAN_SET_NOMINAL_TIME_SEG_1(hcan->CANx, hcan->Init.TimeSeg1);
    __CAN_SET_NOMINAL_TIME_SEG_2(hcan->CANx, hcan->Init.TimeSeg2);
    /* Synchronization Jump Width */
    __CAN_SET_NOMINAL_SYNC_WIDTH(hcan->CANx, hcan->Init.SyncJumpWidth);

    /* default disable Restricted Operation Mode */
    __CAN_RESTRICTED_OPERATION_DISABLE(hcan->CANx);
    /* default disable Bus Monitoring Mode */
    __CAN_BUS_MONITOR_MODE_DISABLE(hcan->CANx);
    /* default disable Test Mode */
    __CAN_TEST_MODE_DISABLE(hcan->CANx);
    /* default enable Automatic retransmission */
    __CAN_AUTOMATIC_RETRANSMISSION_ENABLE(hcan->CANx);
    /* default enable CAN FD Mode  */
    __CAN_FD_MODE_ENABLE(hcan->CANx);
    /* default enable Transmit pause */
    __CAN_TRANSMIT_PAUSE_ENABLE(hcan->CANx);

    if (hcan->Init.DataBit_RateSwitch)
    {
        __CAN_BIT_RATE_SWITCH_ENABLE(hcan->CANx);

        /* Data Bit Rate Prescaler */
        __CAN_SET_DATA_BIT_RATE_PRESCALER(hcan->CANx, hcan->Init.DataBit_Prescaler);
        /* Data Bit time Segment1/segment2 */
        __CAN_SET_DATA_BIT_TIME_SEG_1(hcan->CANx, hcan->Init.DataBit_TimeSeg1);
        __CAN_SET_DATA_BIT_TIME_SEG_2(hcan->CANx, hcan->Init.DataBit_TimeSeg2);
        /* Data Bit Synchronization Jump Width */
        __CAN_SET_DATA_BIT_SYNC_WIDTH(hcan->CANx, hcan->Init.DataBit_SyncJumpWidth);
    }
    else
    {
        __CAN_BIT_RATE_SWITCH_DISABLE(hcan->CANx);
    }

    /* Global Filter Config */
#if (CAN_UNMATCHED_STANDARD_ID_HANDLING == 1)
    __CAN_STANDARD_REJECT_UNMATCHED_FRAME(hcan->CANx);
#elif (CAN_UNMATCHED_STANDARD_ID_HANDLING == 2)
    __CAN_STANDARD_ACCEPT_UNMATCHED_FRAME_TO_FIFO0(hcan->CANx);
#else
    __CAN_STANDARD_ACCEPT_UNMATCHED_FRAME_TO_FIFO1(hcan->CANx);
#endif

#if (CAN_UNMATCHED_EXTENDED_ID_HANDLING == 1)
    __CAN_EXTENDED_REJECT_UNMATCHED_FRAME(hcan->CANx);
#elif (CAN_UNMATCHED_EXTENDED_ID_HANDLING == 2)
    __CAN_EXTENDED_ACCEPT_UNMATCHED_FRAME_TO_FIFO0(hcan->CANx);
#else
    __CAN_EXTENDED_ACCEPT_UNMATCHED_FRAME_TO_FIFO1(hcan->CANx);
#endif

//    __CAN_PROTOCOL_EXCEPTION_HANDLING_ENABLE(hcan->CANx);
    
    /* accept remote frame */
    __CAN_STANDARD_ACCEPT_REMOTE_FRAME(hcan->CANx);
    __CAN_EXTENDED_ACCEPT_REMOTE_FRAME(hcan->CANx);

    /* Tx Buffer Transmission Occurred enable */
    __CAN_Tx_OCCURRED_INT_ENABLE(hcan->CANx, 0xFFFFFFFF);

    /* Int status enable */
    __CAN_INT_LINE_ENABLE(hcan->CANx);
}

/************************************************************************************
 * @fn      can_message_ram_init
 *
 * @brief   initialize  standard ID filter / extended ID filter /
 *                      Tx queue Buffer    /
 *                      Rx FIFO 0/1        / Rx Buffer
 *                      element size, Nums and start address.
 *
 * @param   hcan: CAN handle.
 * 
 * @return  the amount of ram used. Unit byte.
 *          < 0: error code
 */
int32_t can_message_ram_init(CAN_HandleTypeDef *hcan)
{
    uint16_t lu16_AddrOffset; 
    uint16_t lu16_ElementSize;

    if (hcan->RAMConfig.StandardIDFilterNums > 128)
        return CAN_ERR_STANDARD_ID_FILTER_NUMS_OVERFLOW;
    if (hcan->RAMConfig.ExtendedIDFilterNums > 64)
        return CAN_ERR_EXTENDEN_ID_FILTER_NUMS_OVERFLOW;
    if (hcan->RAMConfig.TxFIFOQueueNums + hcan->RAMConfig.TxDedicatedBufferNums > 32)
        return CAN_ERR_TXBUFFER_NUMS_OVERFLOW;
    if (hcan->RAMConfig.RxFIFO0Nums > 64)
        return CAN_ERR_RX_FIFO0_NUMS_OVERFLOW;
    if (hcan->RAMConfig.RxFIFO1Nums > 64)
        return CAN_ERR_RX_FIFO1_NUMS_OVERFLOW;
    if (hcan->RAMConfig.DataBufferSize > CAN_DATA_BUFFER_SIZE_64_BYTE)
        return CAN_ERR_DATA_BUFFER_OVERSIZE;

    /* Set message ram start address */
    __CAN_SET_MESSAGE_RAM_START_ADDR(hcan->CANx, hcan->RAMConfig.StartAddress);

    /* ---------------------------------------------*/
    /* ----- standard ID filter Buffer config ------*/
    /* ---------------------------------------------*/
    /* Calculate Start address */
    lu16_AddrOffset = 0;
    /* Set the number of standard message ID filter element */
    __CAN_SET_STANDARD_ID_FILTER_LIST_NUMS(hcan->CANx, hcan->RAMConfig.StandardIDFilterNums);
    /* Set the start address of standard Message ID filter list */
    __CAN_SET_STANDARD_ID_FILTER_LIST_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

    /* ---------------------------------------------*/
    /* ----- extended ID filter Buffer config ------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset += hcan->RAMConfig.StandardIDFilterNums * 4;
    /* Set the number of extended message ID filter element */
    __CAN_SET_EXTENDED_ID_FILTER_LIST_NUMS(hcan->CANx, hcan->RAMConfig.ExtendedIDFilterNums);
    /* Set the start address of extended Message ID filter list */
    __CAN_SET_EXTENDED_ID_FILTER_LIST_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

    /* ---------------------------------------------*/
    /* ---------- Tx queue Buffer congfig ----------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset += hcan->RAMConfig.ExtendedIDFilterNums * 8;
    /* Tx operation mode selection */
#if (CAN_TX_OPERATION_MODE_SELECTION == 1)
    /* Tx fifo operation */
    __CAN_SET_Tx_FIFO_OPERATION(hcan->CANx);
#else
    /* Tx queue operation */
    __CAN_SET_Tx_QUEUE_OPERATION(hcan->CANx);
#endif

    /* Set the Number of Tx Queue element */
    __CAN_SET_Tx_FIFO_QUEUE_NUMS(hcan->CANx, hcan->RAMConfig.TxFIFOQueueNums);
    __CAN_SET_Tx_BUFFER_NUMS(hcan->CANx, hcan->RAMConfig.TxDedicatedBufferNums);

    /* Set the start address of Tx Queue section in Message RAM */
    __CAN_SET_Tx_BUFFER_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

     /* Tx queue  element size  /
        Rx FIFO0  element size  /
        Rx FIFO1  element size  /
        Rx Buffer element size */
    switch (hcan->RAMConfig.DataBufferSize)
    {
        case CAN_DATA_BUFFER_SIZE_8_BYTE:  lu16_ElementSize = 16; break;
        case CAN_DATA_BUFFER_SIZE_12_BYTE: lu16_ElementSize = 20; break;
        case CAN_DATA_BUFFER_SIZE_16_BYTE: lu16_ElementSize = 24; break;
        case CAN_DATA_BUFFER_SIZE_20_BYTE: lu16_ElementSize = 28; break;
        case CAN_DATA_BUFFER_SIZE_24_BYTE: lu16_ElementSize = 32; break;
        case CAN_DATA_BUFFER_SIZE_32_BYTE: lu16_ElementSize = 40; break;
        case CAN_DATA_BUFFER_SIZE_48_BYTE: lu16_ElementSize = 56; break;
        case CAN_DATA_BUFFER_SIZE_64_BYTE: lu16_ElementSize = 72; break;
        default: break; 
    }
    /* Set Rx FIFO 0/1, Rx Buffer element size */
    __CAN_SET_Rx_BUFFER_ELEMENT_SIZE(hcan->CANx, hcan->RAMConfig.DataBufferSize);
    __CAN_SET_Rx_FIFO0_ELEMENT_SIZE(hcan->CANx,  hcan->RAMConfig.DataBufferSize);
    __CAN_SET_Rx_FIFO1_ELEMENT_SIZE(hcan->CANx,  hcan->RAMConfig.DataBufferSize);
    /* Set Tx Buffer Element Size */
    __CAN_SET_Tx_BUFFER_ELEMENT_SIZE(hcan->CANx, hcan->RAMConfig.DataBufferSize);

    /* Record element size */
    hcan->ElementSize = lu16_ElementSize;
        
    /* ---------------------------------------------*/
    /* ------------- Rx FIFO 0 congfig -------------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset += (hcan->RAMConfig.TxFIFOQueueNums + hcan->RAMConfig.TxDedicatedBufferNums) * lu16_ElementSize;

    /* FIFO 0 blocking/overwrite mode */
#if (CAN_RX_FIFO0_MODE_SELECTION == 1)
    __CAN_SET_Rx_FIFO0_BLOCKING_MODE(hcan->CANx);
#else
    __CAN_SET_Rx_FIFO0_OVERWRITE_MODE(hcan->CANx);
#endif
    /* Set the Number of Rx FIFO0 element */
    __CAN_SET_Rx_FIFO0_NUMS(hcan->CANx, hcan->RAMConfig.RxFIFO0Nums);
    /* Rx FIFO0 almost full threshold */
    __CAN_SET_RX_FIFO0_THRESHOLD(hcan->CANx, CAN_RX_FIFO0_ALMOST_FULL_THRESHOLD);
    /* Set the start address of Rx FIFO 0 section in Message RAM */
    __CAN_SET_Rx_FIFO0_START_ADDRESS(hcan->CANx, lu16_AddrOffset);
    
    /* ---------------------------------------------*/
    /* ------------- Rx FIFO 1 congfig -------------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset += hcan->RAMConfig.RxFIFO0Nums * lu16_ElementSize;

    /* FIFO 1 blocking/overwrite mode */
#if (CAN_RX_FIFO0_MODE_SELECTION == 1)
    __CAN_SET_Rx_FIFO1_BLOCKING_MODE(hcan->CANx);
#else
    __CAN_SET_Rx_FIFO1_OVERWRITE_MODE(hcan->CANx);
#endif
    /* Set the Number of Rx FIFO1 element */
    __CAN_SET_Rx_FIFO1_NUMS(hcan->CANx, hcan->RAMConfig.RxFIFO1Nums);
    /* Rx FIFO1 almost full threshold */
    __CAN_SET_RX_FIFO1_THRESHOLD(hcan->CANx, CAN_RX_FIFO1_ALMOST_FULL_THRESHOLD);
    /* Set the start address of Rx FIFO 1 section in Message RAM */
    __CAN_SET_Rx_FIFO1_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

    /* ---------------------------------------------*/
    /* -------- Rx dedicated buffer congfig --------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset += hcan->RAMConfig.RxFIFO1Nums * lu16_ElementSize;

    /* Set the start address of Rx dedicated buffer section in Message RAM */
    __CAN_SET_Rx_BUFFER_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

#ifdef CAN_TX_EVENT_FUNCTION_EXIST
    /* ---------------------------------------------*/
    /* ---------- Tx Event buffer congfig ----------*/
    /* ---------------------------------------------*/
    /* Calculate offset address */
    lu16_AddrOffset +=  CAN_RX_DEDICATED_BUFFER_NUMS * lu16_ElementSize;

    /* Set the Number of Tx Event element */
    __CAN_SET_Tx_EVENT_NUMS(hcan->CANx, hcan->RAMConfig.TxEventNums);
    /* Set the start address of Tx Event FIFO in Message RAM */
    __CAN_SET_Tx_EVENT_START_ADDRESS(hcan->CANx, lu16_AddrOffset);

    __CAN_SET_TX_EVENT_THRESHOLD(hcan->CANx, CAN_TX_EVENT_ALMOST_FULL_THRESHOLD);
#endif

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);

#ifdef CAN_TX_EVENT_FUNCTION_EXIST
    /* Calculate the amount of ram used */
    lu16_AddrOffset += hcan->RAMConfig.TxEventNums * 8;
#else
    /* Calculate offset address */
    lu16_AddrOffset +=  CAN_RX_DEDICATED_BUFFER_NUMS * lu16_ElementSize;
#endif

    return lu16_AddrOffset;
}

/************************************************************************************
 * @fn      can_ram_watch_dog_config
 *
 * @brief   ram watch dog config.
 *
 * @param   fu8_InitValue: Start value of the Message RAM Watchdog Counter.
 *                         if the value config '00' the counter is disabled.
 */
void can_ram_watch_dog_config(CAN_HandleTypeDef *hcan, uint8_t fu8_InitValue)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* set ram watchdog initial value */
    __CAM_SET_RAM_WATCHDOG_INITIAL_VALUE(hcan->CANx, fu8_InitValue);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_get_ram_watch_dog_value
 *
 * @brief   get ram watch dog counter value.
 */
uint8_t can_get_ram_watch_dog_value(CAN_HandleTypeDef *hcan)
{
    return __CAM_GET_RAM_WATCHDOG_VALUE(hcan->CANx);
}

/************************************************************************************
 * @fn      can_timestamp_Prescaler_config
 *
 * @brief   Configures the timestamp unit in multiples of CAN bit times 1 ~ 256.
 *
 * @param   fu8_prescaler: prescaler value can be 1 ~ 256.
 */
void can_timestamp_prescaler_config(CAN_HandleTypeDef *hcan, uint16_t fu16_prescaler)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* set timestamp prescaler */
    __CAN_SET_TIMESTAMP_PRESCALER(hcan->CANx, fu16_prescaler);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_timestamp_counter_enable/disable
 *
 * @brief   timestamp counter enable/disable.
 */
void can_timestamp_counter_enable(CAN_HandleTypeDef *hcan)
{
    __CAN_SET_TIMESTAMP_SELECT_INTERNAL(hcan->CANx);
}
void can_timestamp_counter_disable(CAN_HandleTypeDef *hcan)
{
    __CAN_SET_TIMESTAMP_SELECT_BYPASS(hcan->CANx);
}

/************************************************************************************
 * @fn      can_timestamp_counter_reset
 *
 * @brief   timestamp counter reset 0.
 */
void can_timestamp_counter_reset(CAN_HandleTypeDef *hcan)
{
    __CAN_SET_TIMESTAMP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_timestamp_counter_reset
 *
 * @brief   get timestamp counter value.
 */
uint16_t can_get_timestamp_counter(CAN_HandleTypeDef *hcan)
{
    return __CAN_GET_TIMESTAMP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_get_transmit_error_counter
 * @fn      can_get_receive_error_counter
 *
 * @brief   get transmit/receive error  counter value.
 */
uint8_t can_get_transmit_error_counter(CAN_HandleTypeDef *hcan)
{
    return __CAN_GET_TRANSMIT_ERROR_COUNTER(hcan->CANx);
}
uint8_t can_get_receive_error_counter(CAN_HandleTypeDef *hcan)
{
    return __CAN_GET_RECEIVE_ERROR_COUNTER(hcan->CANx);
}

/************************************************************************************
 * @fn      can_add_tx_message
 *
 * @brief   Add a message to the Tx queue/fifo.
 *
 * @param   hcan: CAN handle.
 *          fstr_TxHeader: Tx message header.
 *          Data: Data buffer pointer.
 * @return  lu32_PutIndex > 0: Put index in Tx queue. 
 *          < 0: error code
 */
int32_t can_add_tx_message(CAN_HandleTypeDef *hcan, struct_CANTxHeaderDef_t fstr_TxHeader, uint8_t *Data)
{
    uint32_t i;

    struct_CanTxElement_t *CanTxElement;
    uint32_t lu32_Address;
    uint32_t lu32_PutIndex;
    
    /* Tx queue full */
    if (__CAN_IS_TxFIFO_QUEUE_FULL(hcan->CANx))
        return CAN_ERR_TXFIFO_FULL;

    /* the start address of Tx Queue section in Message RAM */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_Tx_BUFFER_START_ADDRESS(hcan->CANx);

    /* Calculate element offset address */
    lu32_PutIndex = __CAN_GET_TxFIFO_QUEUE_PUT_INDEX(hcan->CANx);
    lu32_Address += lu32_PutIndex * hcan->ElementSize;

    /* write param/data to message ram */
    CanTxElement = (struct_CanTxElement_t *)lu32_Address;

    /* Frame Mode, classcial or FD */
    CanTxElement->FrameCFG.FDF = fstr_TxHeader.FormatMode;
    /* Frame type, data or remote */
    CanTxElement->FrameCFG.XTD = fstr_TxHeader.IdType;

    if (fstr_TxHeader.IdType == CAN_ID_STANDARD) 
        CanTxElement->FrameCFG.ID = fstr_TxHeader.Identifier << 18;
    else
        CanTxElement->FrameCFG.ID = fstr_TxHeader.Identifier;

    /* Data frame */
    if (fstr_TxHeader.FrameType == CAN_DATA_FRAME) 
    {
        CanTxElement->FrameCFG.DLC = fstr_TxHeader.DLC;
        CanTxElement->FrameCFG.RTR = CAN_DATA_FRAME;

        uint32_t TxLength;

        switch (fstr_TxHeader.DLC)
        {
            case CAN_FD_DLC9_DATA_LENGTH_12BYTE:  TxLength = 12; break;
            case CAN_FD_DLC10_DATA_LENGTH_16BYTE: TxLength = 16; break;
            case CAN_FD_DLC11_DATA_LENGTH_20BYTE: TxLength = 20; break;
            case CAN_FD_DLC12_DATA_LENGTH_24BYTE: TxLength = 24; break;
            case CAN_FD_DLC13_DATA_LENGTH_32BYTE: TxLength = 32; break;
            case CAN_FD_DLC14_DATA_LENGTH_48BYTE: TxLength = 48; break;
            case CAN_FD_DLC15_DATA_LENGTH_64BYTE: TxLength = 64; break;

            default: TxLength = fstr_TxHeader.DLC; break;
        }
        for (i = 0; i < TxLength; i++)
        {
            CanTxElement->Data[i] = Data[i];
        }
    }
    /* Remote frame */
    else 
    {
        CanTxElement->FrameCFG.DLC = 0;
        CanTxElement->FrameCFG.RTR = CAN_REMOTE_FRAM;
    }
    /* Bit Rate Switch */
    CanTxElement->FrameCFG.BRS = fstr_TxHeader.BitRateSwitch;

    /* Add request */
    __CAN_ADD_Tx_REQUEST(hcan->CANx, 1 << lu32_PutIndex);

    return lu32_PutIndex;
}

/************************************************************************************
 * @fn      can_add_tx_message_to_txbuffer
 *
 * @brief   Add a message to the Tx buffer.
 *
 * @param   hcan: CAN handle.
 *          fu32_TxBufferIndex: tx buffer index.
 *          fstr_TxHeader: Tx message header.
 *          Data: Data buffer pointer.
 * @return  result > 0: Put index in Tx buffer. 
 *          < 0: error code
 */
int32_t can_add_tx_message_to_txbuffer(CAN_HandleTypeDef *hcan, uint32_t fu32_TxBufferIndex, struct_CANTxHeaderDef_t fstr_TxHeader, uint8_t *Data)
{
    uint32_t i;

    struct_CanTxElement_t *CanTxElement;
    uint32_t lu32_Address;

    /* Tx buffer in use */
    if (can_is_tx_message_pending(hcan, fu32_TxBufferIndex))
        return CAN_ERR_TXBUFFER_IN_USE;

    /* the start address of Tx buffer section in Message RAM */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_Tx_BUFFER_START_ADDRESS(hcan->CANx);

    /* Calculate element offset address */
    lu32_Address += fu32_TxBufferIndex * hcan->ElementSize;

    /* write param/data to message ram */
    CanTxElement = (struct_CanTxElement_t *)lu32_Address;

    /* Frame Mode, classcial or FD */
    CanTxElement->FrameCFG.FDF = fstr_TxHeader.FormatMode;
    /* Frame type, data or remote */
    CanTxElement->FrameCFG.XTD = fstr_TxHeader.IdType;

    if (fstr_TxHeader.IdType == CAN_ID_STANDARD) 
        CanTxElement->FrameCFG.ID = fstr_TxHeader.Identifier << 18;
    else
        CanTxElement->FrameCFG.ID = fstr_TxHeader.Identifier;

    /* Data frame */
    if (fstr_TxHeader.FrameType == CAN_DATA_FRAME) 
    {
        CanTxElement->FrameCFG.DLC = fstr_TxHeader.DLC;
        CanTxElement->FrameCFG.RTR = CAN_DATA_FRAME;

        uint32_t TxLength;

        switch (fstr_TxHeader.DLC)
        {
            case CAN_FD_DLC9_DATA_LENGTH_12BYTE:  TxLength = 12; break;
            case CAN_FD_DLC10_DATA_LENGTH_16BYTE: TxLength = 16; break;
            case CAN_FD_DLC11_DATA_LENGTH_20BYTE: TxLength = 20; break;
            case CAN_FD_DLC12_DATA_LENGTH_24BYTE: TxLength = 24; break;
            case CAN_FD_DLC13_DATA_LENGTH_32BYTE: TxLength = 32; break;
            case CAN_FD_DLC14_DATA_LENGTH_48BYTE: TxLength = 48; break;
            case CAN_FD_DLC15_DATA_LENGTH_64BYTE: TxLength = 64; break;

            default: TxLength = fstr_TxHeader.DLC; break;
        }
        for (i = 0; i < TxLength; i++)
        {
            CanTxElement->Data[i] = Data[i];
        }
    }
    /* Remote frame */
    else 
    {
        CanTxElement->FrameCFG.DLC = 0;
        CanTxElement->FrameCFG.RTR = CAN_REMOTE_FRAM;
    }
    /* Bit Rate Switch */
    CanTxElement->FrameCFG.BRS = fstr_TxHeader.BitRateSwitch;

    /* Add request */
    __CAN_ADD_Tx_REQUEST(hcan->CANx, 1 << fu32_TxBufferIndex);

    return fu32_TxBufferIndex;
}

/************************************************************************************
 * @fn      can_is_tx_message_pending
 *
 * @brief   Check if a transmission request is pending.
 *
 * @param   hcan: CAN handle.
 *          fu32_PutIndex: tx message index in Tx queue. 
 * @return  true:  tx message pending.
 *          false: tx message not pending.
 */
bool can_is_tx_message_pending(CAN_HandleTypeDef *hcan, uint32_t fu32_PutIndex)
{
    bool lb_status;

    return lb_status = (__CAN_GET_Tx_REQUEST_PENDING(hcan->CANx) & (1 << fu32_PutIndex)) ? true : false;
}

/************************************************************************************
 * @fn      can_abort_tx_message
 *
 * @brief   abort a message from the Tx queue.
 *
 * @param   hcan: CAN handle.
 *          fu32_PutIndex: tx message index in Tx queue. 
 */
void can_abort_tx_message(CAN_HandleTypeDef *hcan, uint32_t fu32_PutIndex)
{
    /* Tx Buffer Cancellation Request */
    __CAN_CANCELLATION_Tx_REQUEST(hcan->CANx, 1 << fu32_PutIndex);
}

/************************************************************************************
 * @fn      can_add_standard_filter
 *
 * @brief   Added standard filter config.
 *
 * @param   hcan: CAN handle.
 *          fstr_FilterCfg: standard filter config param.
 *          fu32_Index: filter number index.
 */
void can_add_standard_filter(CAN_HandleTypeDef *hcan, struct_FilterCfg_t fstr_FilterCfg, uint32_t fu32_Index)
{
    uint32_t lu32_Address;

    struct_StdFilterElement_t *StdFilterElement;

    /* Set the start address of standard Message ID filter list */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_STANDARD_ID_FILTER_LIST_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += fu32_Index * 4;

    /* write param/data to message ram */
    StdFilterElement = (struct_StdFilterElement_t *)lu32_Address;

    StdFilterElement->SFT   = fstr_FilterCfg.FilterType;
    StdFilterElement->SFEC  = fstr_FilterCfg.ProcessMode;
    StdFilterElement->SFID1 = fstr_FilterCfg.FilterID_1;
    StdFilterElement->SFID2 = fstr_FilterCfg.FilterID_2;

    if (fstr_FilterCfg.ProcessMode == FILTER_PROCESS_STORE_IN_RxBUFFER) 
    {
        StdFilterElement->SFID2 = fstr_FilterCfg.RxBufferIndex;

        if (fstr_FilterCfg.RxBufferIndex < 32)
            hcan->RxBufferUsed_L |= 1 << fstr_FilterCfg.RxBufferIndex;
        else
            hcan->RxBufferUsed_H |= 1 << (fstr_FilterCfg.RxBufferIndex - 32);
    }
}

/************************************************************************************
 * @fn      can_add_extended_filter
 *
 * @brief   Added extended filter config.
 *
 * @param   hcan: CAN handle.
 *          fstr_FilterCfg: extended filter config param.
 *          fu32_Index: filter number index.
 */
void can_add_extended_filter(CAN_HandleTypeDef *hcan, struct_FilterCfg_t fstr_FilterCfg, uint32_t fu32_Index)
{
    uint32_t lu32_Address;
    
    struct_ExtFilterElement_t *ExtFilterElement;
    
    /* Set the start address of extended Message ID filter list */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_EXTENDED_ID_FILTER_LIST_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += fu32_Index * 8;
    
    /* write param/data to message ram */
    ExtFilterElement = (struct_ExtFilterElement_t *)lu32_Address;

    ExtFilterElement->EFT   = fstr_FilterCfg.FilterType;
    ExtFilterElement->EFEC  = fstr_FilterCfg.ProcessMode;
    ExtFilterElement->EFID1 = fstr_FilterCfg.FilterID_1;
    ExtFilterElement->EFID2 = fstr_FilterCfg.FilterID_2;

    if (fstr_FilterCfg.ProcessMode == FILTER_PROCESS_STORE_IN_RxBUFFER) 
    {
        ExtFilterElement->EFID2 = fstr_FilterCfg.RxBufferIndex;

        if (fstr_FilterCfg.RxBufferIndex < 32)
            hcan->RxBufferUsed_L |= 1 << fstr_FilterCfg.RxBufferIndex;
        else
            hcan->RxBufferUsed_H |= 1 << (fstr_FilterCfg.RxBufferIndex - 32);
    }
}

/************************************************************************************
 * @fn      can_remove_standard_filter
 *
 * @brief   Remove standard filter config.
 *
 * @param   hcan: CAN handle.
 *          fu32_Index: filter number index.
 */
void can_remove_standard_filter(CAN_HandleTypeDef *hcan, uint32_t fu32_Index)
{
    uint32_t lu32_Address;

    struct_StdFilterElement_t *StdFilterElement;

    /* Set the start address of standard Message ID filter list */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_STANDARD_ID_FILTER_LIST_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += fu32_Index * 4;

    /* write param/data to message ram */
    StdFilterElement = (struct_StdFilterElement_t *)lu32_Address;

    /* Disable selected element */
    if (StdFilterElement->SFEC == FILTER_PROCESS_STORE_IN_RxBUFFER) 
    {
        if (StdFilterElement->SFID2 < 32)
            hcan->RxBufferUsed_L &= ~(1 << StdFilterElement->SFID2);
        else
            hcan->RxBufferUsed_H &= ~(1 << (StdFilterElement->SFID2 - 32));
    }

    StdFilterElement->SFT  = CAN_FILTER_DISABLE;
    StdFilterElement->SFEC = FILTER_PROCESS_DISABLE;
}

/************************************************************************************
 * @fn      can_remove_extended_filter
 *
 * @brief   Remove extended filter config.
 *
 * @param   hcan: CAN handle.
 *          fu32_Index: filter number index.
 */
void can_remove_extended_filter(CAN_HandleTypeDef *hcan, uint32_t fu32_Index)
{
    uint32_t lu32_Address;

    struct_ExtFilterElement_t *ExtFilterElement;

    /* Set the start address of extended Message ID filter list */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_EXTENDED_ID_FILTER_LIST_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += fu32_Index * 8;
    
    /* write param/data to message ram */
    ExtFilterElement = (struct_ExtFilterElement_t *)lu32_Address;

    /* Disable selected element */
    if (ExtFilterElement->EFEC == FILTER_PROCESS_STORE_IN_RxBUFFER) 
    {
        if (ExtFilterElement->EFID2 < 32)
            hcan->RxBufferUsed_L &= ~(1 << ExtFilterElement->EFID2);
        else
            hcan->RxBufferUsed_H &= ~(1 << (ExtFilterElement->EFID2 - 32));
    }

    ExtFilterElement->EFEC = FILTER_PROCESS_DISABLE;
}

/************************************************************************************
 * @fn      can_get_rxbuffer_message
 *
 * @brief   Get an CAN frame from the Rx buffer.
 *
 * @param   hcan: CAN handle.
 *          fu32_RxBufferIndex: The message index in the RxBuffer.
 */
void can_get_rxbuffer_message(CAN_HandleTypeDef *hcan, uint32_t fu32_RxBufferIndex, struct_CANRxHeaderDef_t *RxHeader, uint8_t *Data)
{
    uint32_t lu32_Address;

    struct_CanRxElement_t *CanRxElement;

    /* Get the start address of Rx dedicated buffer section in Message RAM */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_Rx_BUFFER_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += fu32_RxBufferIndex * hcan->ElementSize;
    
    /* Read message param/data from RxBuffer ram */
    CanRxElement = (struct_CanRxElement_t *)lu32_Address;

    RxHeader->IdType     = CanRxElement->FrameCFG.XTD ? CAN_ID_EXTENDED : CAN_ID_STANDARD;
    RxHeader->FrameType  = CanRxElement->FrameCFG.RTR ? CAN_REMOTE_FRAM : CAN_DATA_FRAME;
    RxHeader->FormatMode = CanRxElement->FrameCFG.FDF ? CAN_FD_FRAME    : CAN_CLASSICAL_FRAME;

    if (RxHeader->IdType == CAN_ID_STANDARD)
        RxHeader->Identifier = CanRxElement->FrameCFG.ID >> 18;
    else
        RxHeader->Identifier = CanRxElement->FrameCFG.ID;

    if (RxHeader->FrameType == CAN_DATA_FRAME) 
    {
        switch (CanRxElement->FrameCFG.DLC)
        {
            case CAN_FD_DLC9_DATA_LENGTH_12BYTE:  RxHeader->DLC = 12; break;
            case CAN_FD_DLC10_DATA_LENGTH_16BYTE: RxHeader->DLC = 16; break;
            case CAN_FD_DLC11_DATA_LENGTH_20BYTE: RxHeader->DLC = 20; break;
            case CAN_FD_DLC12_DATA_LENGTH_24BYTE: RxHeader->DLC = 24; break;
            case CAN_FD_DLC13_DATA_LENGTH_32BYTE: RxHeader->DLC = 32; break;
            case CAN_FD_DLC14_DATA_LENGTH_48BYTE: RxHeader->DLC = 48; break;
            case CAN_FD_DLC15_DATA_LENGTH_64BYTE: RxHeader->DLC = 64; break;

            default: RxHeader->DLC = CanRxElement->FrameCFG.DLC; break;
        }

        for (int i = 0; i < RxHeader->DLC; i++)
        {
            Data[i] = CanRxElement->Data[i];
        }
    }
    RxHeader->BitRateSwitch = CanRxElement->FrameCFG.BRS;

    RxHeader->Timestamp = CanRxElement->FrameCFG.RXTS;

    if (CanRxElement->FrameCFG.ANMF == 0) 
    {
        RxHeader->FilterMatchIndex = CanRxElement->FrameCFG.FIDX;
    }

   if (fu32_RxBufferIndex < 32)
       __CAN_CLEAR_NEWDATA1_FLAGS(hcan->CANx, (1 << fu32_RxBufferIndex));
   else
       __CAN_CLEAR_NEWDATA2_FLAGS(hcan->CANx, (1 << (fu32_RxBufferIndex - 32)));
}

/************************************************************************************
 * @fn      can_get_rxfifo0_message
 *
 * @brief   Get an CAN frame from the Rx FIFO 0.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxfifo0_message(CAN_HandleTypeDef *hcan, struct_CANRxHeaderDef_t *RxHeader, uint8_t *Data)
{
    uint32_t lu32_Address, lu32_GetIndex;

    struct_CanRxElement_t *CanRxElement;

    /* Rx FIFO0 get index */
    lu32_GetIndex = __CAN_GET_Rx_FIFO0_GET_INDEX(hcan->CANx);

    /* Get the start address of Rx FIFO 0 section in Message RAM */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_Rx_FIFO0_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += lu32_GetIndex * hcan->ElementSize;

    /* Read message param/data from RxBuffer ram */
    CanRxElement = (struct_CanRxElement_t *)lu32_Address;

    RxHeader->IdType     = CanRxElement->FrameCFG.XTD ? CAN_ID_EXTENDED : CAN_ID_STANDARD;
    RxHeader->FrameType  = CanRxElement->FrameCFG.RTR ? CAN_REMOTE_FRAM : CAN_DATA_FRAME;
    RxHeader->FormatMode = CanRxElement->FrameCFG.FDF ? CAN_FD_FRAME    : CAN_CLASSICAL_FRAME;
    
    if (RxHeader->IdType == CAN_ID_STANDARD)
        RxHeader->Identifier = CanRxElement->FrameCFG.ID >> 18;
    else
        RxHeader->Identifier = CanRxElement->FrameCFG.ID;

    if (RxHeader->FrameType == CAN_DATA_FRAME) 
    {
        switch (CanRxElement->FrameCFG.DLC)
        {
            case CAN_FD_DLC9_DATA_LENGTH_12BYTE:  RxHeader->DLC = 12; break;
            case CAN_FD_DLC10_DATA_LENGTH_16BYTE: RxHeader->DLC = 16; break;
            case CAN_FD_DLC11_DATA_LENGTH_20BYTE: RxHeader->DLC = 20; break;
            case CAN_FD_DLC12_DATA_LENGTH_24BYTE: RxHeader->DLC = 24; break;
            case CAN_FD_DLC13_DATA_LENGTH_32BYTE: RxHeader->DLC = 32; break;
            case CAN_FD_DLC14_DATA_LENGTH_48BYTE: RxHeader->DLC = 48; break;
            case CAN_FD_DLC15_DATA_LENGTH_64BYTE: RxHeader->DLC = 64; break;

            default: RxHeader->DLC = CanRxElement->FrameCFG.DLC; break;
        }

        for (int i = 0; i < RxHeader->DLC; i++)
        {
            Data[i] = CanRxElement->Data[i];
        }
    }
    RxHeader->BitRateSwitch = CanRxElement->FrameCFG.BRS;

    RxHeader->Timestamp = CanRxElement->FrameCFG.RXTS;

    if (CanRxElement->FrameCFG.ANMF == 0) 
    {
        RxHeader->FilterMatchIndex = CanRxElement->FrameCFG.FIDX;
    }

    /* Read message ack */
    __CAN_SET_Rx_FIFO0_ACKNOWLEDGE(hcan->CANx, lu32_GetIndex);
    
    return lu32_GetIndex;
}

/************************************************************************************
 * @fn      can_get_rxfifo1_message
 *
 * @brief   Get an CAN frame from the Rx FIFO 1.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxfifo1_message(CAN_HandleTypeDef *hcan, struct_CANRxHeaderDef_t *RxHeader, uint8_t *Data)
{
    uint32_t lu32_Address, lu32_GetIndex;

    struct_CanRxElement_t *CanRxElement;

    /* Rx FIFO1 get index */
    lu32_GetIndex = __CAN_GET_Rx_FIFO1_GET_INDEX(hcan->CANx);

    /* Get the start address of Rx FIFO 1 section in Message RAM */
    lu32_Address  = __CAN_GET_MESSAGE_RAM_START_ADDR(hcan->CANx);
    lu32_Address += __CAN_GET_Rx_FIFO1_START_ADDRESS(hcan->CANx);
    /* Calculate element offset address */
    lu32_Address += lu32_GetIndex * hcan->ElementSize;

    /* Read message param/data from RxBuffer ram */
    CanRxElement = (struct_CanRxElement_t *)lu32_Address;

    RxHeader->IdType     = CanRxElement->FrameCFG.XTD ? CAN_ID_EXTENDED : CAN_ID_STANDARD;
    RxHeader->FrameType  = CanRxElement->FrameCFG.RTR ? CAN_REMOTE_FRAM : CAN_DATA_FRAME;
    RxHeader->FormatMode = CanRxElement->FrameCFG.FDF ? CAN_FD_FRAME    : CAN_CLASSICAL_FRAME;

    if (RxHeader->IdType == CAN_ID_STANDARD)
        RxHeader->Identifier = CanRxElement->FrameCFG.ID >> 18;
    else
        RxHeader->Identifier = CanRxElement->FrameCFG.ID;

    if (RxHeader->FrameType == CAN_DATA_FRAME) 
    {
        switch (CanRxElement->FrameCFG.DLC)
        {
            case CAN_FD_DLC9_DATA_LENGTH_12BYTE:  RxHeader->DLC = 12; break;
            case CAN_FD_DLC10_DATA_LENGTH_16BYTE: RxHeader->DLC = 16; break;
            case CAN_FD_DLC11_DATA_LENGTH_20BYTE: RxHeader->DLC = 20; break;
            case CAN_FD_DLC12_DATA_LENGTH_24BYTE: RxHeader->DLC = 24; break;
            case CAN_FD_DLC13_DATA_LENGTH_32BYTE: RxHeader->DLC = 32; break;
            case CAN_FD_DLC14_DATA_LENGTH_48BYTE: RxHeader->DLC = 48; break;
            case CAN_FD_DLC15_DATA_LENGTH_64BYTE: RxHeader->DLC = 64; break;

            default: RxHeader->DLC = CanRxElement->FrameCFG.DLC; break;
        }

        for (int i = 0; i < RxHeader->DLC; i++)
        {
            Data[i] = CanRxElement->Data[i];
        }
    }
    RxHeader->BitRateSwitch = CanRxElement->FrameCFG.BRS;

    RxHeader->Timestamp = CanRxElement->FrameCFG.RXTS;

    if (CanRxElement->FrameCFG.ANMF == 0) 
    {
        RxHeader->FilterMatchIndex = CanRxElement->FrameCFG.FIDX;
    }
    
    /* Read message ack */
    __CAN_SET_Rx_FIFO1_ACKNOWLEDGE(hcan->CANx, lu32_GetIndex);

    return lu32_GetIndex;
}

/************************************************************************************
 * @fn      can_get_rxfifo0_fill_level
 *
 * @brief   Get number of elements stored in Rx FIFO 0, range 0 to 64.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxfifo0_fill_level(CAN_HandleTypeDef *hcan)
{
    return __CAN_GET_Rx_FIFO0_FILL_LEVEL(hcan->CANx);
}

/************************************************************************************
 * @fn      can_get_rxfifo1_fill_level
 *
 * @brief   Get number of elements stored in Rx FIFO 0, range 0 to 64.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxfifo1_fill_level(CAN_HandleTypeDef *hcan)
{
    return __CAN_GET_Rx_FIFO1_FILL_LEVEL(hcan->CANx);
}

/************************************************************************************
 * @fn      can_get_rxbuffer_0_31_status
 *
 * @brief   get rx buffer new data 0~31 status.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxbuffer_0_31_status(CAN_HandleTypeDef *hcan)
{
    return hcan->CANx->NewData1;
}

/************************************************************************************
 * @fn      can_get_rxbuffer_32_63_status
 *
 * @brief   get rx buffer new data 32~63 status.
 *
 * @param   hcan: CAN handle.
 */
uint32_t can_get_rxbuffer_32_63_status(CAN_HandleTypeDef *hcan)
{
    return hcan->CANx->NewData2;
}

/************************************************************************************
 * @fn      can_enter_test_mode
 *
 * @brief   enter test mode.
 *
 * @param   hcan: CAN handle.
 * @param   fe_TestMode: test mode select.
 */
void can_enter_test_mode(CAN_HandleTypeDef *hcan, enum_CAN_TestMode_t fe_TestMode)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    /* set test mode */
    if (fe_TestMode == CAN_INTERNAL_LOOP_BACK_TEST_MODE)
    {
        __CAN_TEST_MODE_ENABLE(hcan->CANx);
        __CAN_BUS_MONITOR_MODE_ENABLE(hcan->CANx);
        __CAN_TEST_MODE_LOOP_BACK_MODE_ENABLE(hcan->CANx);
    }
    else
    {
        __CAN_TEST_MODE_ENABLE(hcan->CANx);
        __CAN_BUS_MONITOR_MODE_DISABLE(hcan->CANx);
        __CAN_TEST_MODE_LOOP_BACK_MODE_ENABLE(hcan->CANx);
    }

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
/************************************************************************************
 * @fn      can_quit_test_mode
 *
 * @brief   quit test mode.
 *
 * @param   hcan: CAN handle.
 */
void can_quit_test_mode(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* disable test mode */
    __CAN_TEST_MODE_DISABLE(hcan->CANx);
    __CAN_BUS_MONITOR_MODE_DISABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_enter_bus_monitoring_mode
 * 
 * @brief   enter bus monitoring mode.
 * 
 * @param   hcan: CAN handle.
 */
void can_enter_bus_monitoring_mode(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* bus monitoring mode enable */
    __CAN_BUS_MONITOR_MODE_ENABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
/************************************************************************************
 * @fn      can_quit_bus_monitoring_mode
 * 
 * @brief   quit bus monitoring mode.
 * 
 * @param   hcan: CAN handle.
 */
void can_quit_bus_monitoring_mode(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* bus monitoring mode disable */
    __CAN_BUS_MONITOR_MODE_DISABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_enter_restricted_operation_mode
 * 
 * @brief   enter restricted operation mode.
 * 
 * @param   hcan: CAN handle.
 */
void can_enter_restricted_operation_mode(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* restricted operation mode enable */
    __CAN_RESTRICTED_OPERATION_ENABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
/************************************************************************************
 * @fn      can_quit_restricted_operation_mode
 * 
 * @brief   quit restricted operation mode.
 * 
 * @param   hcan: CAN handle.
 */
void can_quit_restricted_operation_mode(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* restricted operation mode disable */
    __CAN_RESTRICTED_OPERATION_DISABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_fd_transmitter_delay_compensation_enable
 * 
 * @brief   transmitter delay compensation enable. And config configuration parameter.
 * 
 * @param   hcan: CAN handle.
 * @param   fu16_TDCF: Transmitter Delay Compensation Filter Window Length.
 * @param   fu32_TDCO: Transmitter Delay Compensation Offset. 
 */
void can_fd_transmitter_delay_compensation_enable(CAN_HandleTypeDef *hcan, uint16_t fu16_TDCF, uint16_t fu32_TDCO)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* transmitter delay compensation enable */
    __CAN_TRANSMITTER_DELAY_COMPENSATION_ENABLE(hcan->CANx);
    /* Transmitter Delay Compensation Filter Window Length */
    __CAN_SET_TRANSMITTER_DELAY_COMPENSATION_OFFSET(hcan->CANx, fu32_TDCO);
    /* Transmitter Delay Compensation Offset */
    __CAN_SET_TRANSMITTER_DELAY_COMPENSATION_FILTER_WINDOW_LENGTH(hcan->CANx, fu16_TDCF);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
void can_fd_transmitter_delay_compensation_disable(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* transmitter delay compensation disable */
    __CAN_TRANSMITTER_DELAY_COMPENSATION_DISABLE(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @brief      can fd frame format according to ISO11898-1
 * @brief      can fd frame format according to Bosch CAN FD Specification V1.0
 */
void can_fd_frame_format_according_to_ISO11898_1(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* can fd frame format according to ISO11898-1 */
    __CAN_FD_FRAME_FORMAT_ISO11898_1(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
void can_fd_frame_format_according_to_Bosch(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);
    /* can fd frame format according to Bosch CAN FD Specification V1.0 */
    __CAN_FD_FRAME_FORMAT_BOSCH_CAN_FD_SPECV1(hcan->CANx);
    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_timeout_counter_config
 * 
 * @brief   timeout counter config.
 * 
 * @param   hcan: CAN handle.
 */
void can_timeout_counter_config(CAN_HandleTypeDef *hcan, uint16_t fu16_InitValue, enum_CAN_Timeout_select_t fe_TOSelect)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    /* timeout counter set initial value */
    /* timeout counter set mode */
    __CAN_TIMEOUT_COUNTER_INITIAL_VALUE(hcan->CANx, fu16_InitValue);
    __CAN_TIMEOUT_COUNTER_SET_MODE(hcan->CANx, fe_TOSelect);

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
void can_timeout_counter_enable(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    __CAN_TIMEOUT_COUNTER_ENABLE(hcan->CANx);

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
void can_timeout_counter_disable(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    __CAN_TIMEOUT_COUNTER_DISABLE(hcan->CANx);

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}
void can_timeout_counter_restart(CAN_HandleTypeDef *hcan)
{
    /* Initialization start */
    __CAN_INIT_START(hcan->CANx);
    __CAN_CHANGE_ENABLE(hcan->CANx);

    __CAN_TIMEOUT_COUNTER_SET_MODE(hcan->CANx, __CAN_TIMEOUT_COUNTER_GET_MODE(hcan->CANx));

    /* Initialization stop */
    __CAN_CHANGE_DISABLE(hcan->CANx);
    __CAN_INIT_STOP(hcan->CANx);
}

/************************************************************************************
 * @fn      can_int_select_line
 *
 * @brief   can interrupt select line0/line1.
 *          default all interrupt sources use line0.
 */
void can_int_select_line(CAN_HandleTypeDef *hcan, uint32_t fu32_INT_Index, enum_CAN_INT_LINEx_t fe_line)
{
    if (fe_line)
        __CAN_INT_SELECT_LINE1(hcan->CANx, fu32_INT_Index);
    else
        __CAN_INT_SELECT_LINE0(hcan->CANx, fu32_INT_Index);
}

/************************************************************************************
 * @fn      can_int_enable
 *
 * @brief   can interrupt enable.
 */
void can_int_enable(CAN_HandleTypeDef *hcan, uint32_t fu32_INT_Index)
{
    __CAN_INT_ENABLE(hcan->CANx, fu32_INT_Index);
}

/************************************************************************************
 * @fn      can_int_disable
 *
 * @brief   can interrupt disable.
 */
void can_int_disable(CAN_HandleTypeDef *hcan, uint32_t fu32_INT_Index)
{
    __CAN_INT_DISABLE(hcan->CANx, fu32_INT_Index);
}

/************************************************************************************
 * @fn      can_get_int_status
 *
 * @brief   get interrupt status.
 */
uint32_t can_get_int_status(CAN_HandleTypeDef *hcan)
{
    return __CAN_INT_GET_STATUS(hcan->CANx);
}

/************************************************************************************
 * @fn      can_clear_int_status
 *
 * @brief   clear interrupt status.
 */
void can_clear_int_status(CAN_HandleTypeDef *hcan, uint32_t fu32_INT_Index)
{
    __CAN_INT_CLEAR(hcan->CANx, fu32_INT_Index);
}
